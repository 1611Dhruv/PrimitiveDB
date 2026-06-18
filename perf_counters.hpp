#ifndef PERF_COUNTERS_HPP
#define PERF_COUNTERS_HPP

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <linux/perf_event.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace measure_perf {
struct Event {
  uint32_t type;
  uint64_t config;
  const char *name;
};

struct GroupRead {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

class Perf {

public:
  Perf(const std::vector<Event> &events) : _leader_fd(-1) {
    perf_event_attr att;

    size_t N = events.size();
    for (size_t i = 0; i < N; i++) {
      const auto &event = events[i];

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
    ioctl(_leader_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(_leader_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }

  void stop() { return stop(std::cout); }
  void stop(std::ostream &os) {
    ioctl(_leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    size_t needed = 1 + 2 * _event_fds.size();
    std::vector<uint64_t> buffer(needed);

    if (read(_leader_fd, buffer.data(), needed * sizeof(uint64_t)) < 0) {
      throw std::runtime_error("Unable to read perf output");
    }

    GroupRead *gr = reinterpret_cast<GroupRead *>(buffer.data());
    for (uint64_t i = 0; i < gr->nr; i++) {
      os << _metric_reg[gr->values[i].id] << " : " << gr->values[i].value
         << "\n";
    }
  }

  // Delete Copy for simplicity
  Perf(const Perf &other) = delete;
  Perf operator=(const Perf &other) = delete;

private:
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
};

}; // namespace measure_perf

#endif
