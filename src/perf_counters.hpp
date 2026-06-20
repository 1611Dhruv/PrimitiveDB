#ifndef PERF_COUNTERS_HPP
#define PERF_COUNTERS_HPP

// Counter core: a thin RAII wrapper over perf_event_open(2). No file I/O, no
// global state -- safe to include in any number of translation units. The CSV
// logging harness that uses these counters lives in perf_measure.hpp.

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <linux/perf_event.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

static constexpr uint32_t PERF_WALL_TIME = 0xFFFFFFFFu;
static constexpr uint64_t WALL_TIME_ID = ~0ull;
namespace measure_perf {
struct Event {
  uint32_t type;
  uint64_t config;
  const char *name;
};

struct PerfResult {
  std::string name;
  uint64_t measure;
};

class Perf {
public:
  Perf(const std::vector<Event> &events) : _leader_fd(-1) {
    perf_event_attr att;

    size_t N = events.size();
    for (size_t i = 0; i < N; i++) {
      const auto &event = events[i];

      if (event.type == PERF_WALL_TIME) {
        _wall_time = true;
        _metric_reg[WALL_TIME_ID] = event.name;
        continue;
      }

      memset(&att, 0, sizeof(att));
      att.type = event.type;
      att.config = event.config;
      int fd = perf_open(&att, _leader_fd);
      if (fd < 0) {
        std::string err_str(strerror(errno));

        while (!_event_fds.empty()) {
          close(_event_fds.back());
          _event_fds.pop_back();
        }

        throw std::invalid_argument(build_error(err_str, event));
      } else {
        if (_leader_fd < 0)
          _leader_fd = fd;
        _event_fds.push_back(fd);

        uint64_t id;
        ioctl(fd, PERF_EVENT_IOC_ID, &id);
        _metric_reg[id] = event.name;
      }
    }
  }

  ~Perf() {
    while (!_event_fds.empty()) {
      close(_event_fds.back());
      _event_fds.pop_back();
    }
  }

  void start() {
    if (_wall_time)
      _t0 = std::chrono::steady_clock::now();
    ioctl(_leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(_leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }

  std::vector<PerfResult> stop() {
    ioctl(_leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    auto _t1 = std::chrono::steady_clock::now();

    size_t needed = 1 + 2 * _event_fds.size();
    std::vector<uint64_t> buffer(needed);
    if (read(_leader_fd, buffer.data(), needed * sizeof(uint64_t)) < 0) {
      throw std::runtime_error("Unable to read perf output");
    }

    if (_wall_time) {
      buffer.push_back(
          (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(_t1 -
                                                                         _t0)
              .count());
      buffer.push_back(WALL_TIME_ID);
    }
    GroupRead *gr = reinterpret_cast<GroupRead *>(buffer.data());
    if (_wall_time) {
      gr->nr++;
    }

    std::vector<PerfResult> res;
    for (size_t i = 0; i < gr->nr; i++) {
      res.push_back({_metric_reg[gr->values[i].id], gr->values[i].value});
    }
    return res;
  }

  std::vector<std::string> getMetricNames() const {
    std::vector<std::string> names;
    for (const auto &[_, v] : _metric_reg) {
      names.push_back(v);
    }
    return names;
  }

  // Delete Copy for simplicity
  Perf(const Perf &other) = delete;
  Perf operator=(const Perf &other) = delete;

private:
  struct GroupRead {
    uint64_t nr;
    struct {
      uint64_t value;
      uint64_t id;
    } values[];
  };

  inline long perf_open(perf_event_attr *a, int leader_fd) {
    a->size = sizeof(*a);
    a->disabled = (leader_fd == -1) ? 1 : 0;
    a->exclude_kernel = 1;
    a->exclude_hv = 1;
    a->read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    return syscall(__NR_perf_event_open, a, /*pid=*/0, /*cpu=*/-1, leader_fd,
                   /*flags=*/0);
  }

  std::string build_error(const std::string &err_str, const Event &e) const {
    std::stringstream ss;
    ss << "Failed to open this event " << e.type << " " << e.config
       << "\n err: " << err_str << "\n";
    return ss.str();
  }

  std::vector<int> _event_fds;
  std::unordered_map<uint64_t, std::string> _metric_reg;
  int _leader_fd;
  bool _wall_time = false;
  std::chrono::steady_clock::time_point _t0;
};

// Compiler barrier: keep `value` live so the optimizer can't delete the work
// that produced it. Template -> safe to keep in the header.
template <typename T> inline void doNotOptimize(const T &value) {
  asm volatile("" : : "r,m"(value) : "memory");
}
}; // namespace measure_perf

#endif
