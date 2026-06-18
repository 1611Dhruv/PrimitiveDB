#ifndef PERF_COUNTERS_HPP
#define PERF_COUNTERS_HPP

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
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

  std::vector<PerfResult> stop() {
    ioctl(_leader_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    size_t needed = 1 + 2 * _event_fds.size();
    std::vector<uint64_t> buffer(needed);

    if (read(_leader_fd, buffer.data(), needed * sizeof(uint64_t)) < 0) {
      throw std::runtime_error("Unable to read perf output");
    }

    GroupRead *gr = reinterpret_cast<GroupRead *>(buffer.data());

    std::vector<PerfResult> res;
    for (size_t i = 0; i < gr->nr; i++) {
      res.push_back({_metric_reg[gr->values[i].id], gr->values[i].value});
    }
    return res;
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
};

// AI wrote this :P

// Compiler barrier: forces `value` to be materialized, so the optimizer can't
// delete the workload that produced it across rounds. (Same trick Google
// Benchmark uses for DoNotOptimize.)
template <typename T> inline void doNotOptimize(const T &value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

// Run `work` for `rounds` measured iterations (after `warmup` discarded ones),
// then report mean, sample standard deviation, and coefficient of variation
// (cv% = stddev/mean) for every counter. Each round resets the group, so each
// sample is that round's count on its own. `work` must return a value (its
// checksum) so the loop can't be optimized away.
template <typename Fn>
void measure(Perf &perf, Fn &&work, int rounds = 10, int warmup = 1,
             std::ostream &os = std::cout) {
  // Discarded warmup rounds: fault in pages, warm caches and predictors.
  for (int i = 0; i < warmup; i++) {
    perf.start();
    auto sink = work();
    perf.stop();
    doNotOptimize(sink);
  }

  std::vector<std::string> order; // metric names, in first-seen order
  std::unordered_map<std::string, std::vector<double>> samples;

  for (int r = 0; r < rounds; r++) {
    perf.start();
    auto sink = work();
    std::vector<PerfResult> round = perf.stop();
    doNotOptimize(sink);

    for (const auto &pr : round) {
      auto [it, inserted] = samples.try_emplace(pr.name);
      if (inserted)
        order.push_back(pr.name);
      it->second.push_back(static_cast<double>(pr.measure));
    }
  }

  os << std::left << std::setw(24) << "metric" << std::right << std::setw(18)
     << "mean" << std::setw(18) << "stddev" << std::setw(9) << "cv%"
     << "   (n=" << rounds << ")\n";

  for (const auto &name : order) {
    const std::vector<double> &xs = samples[name];
    double n = static_cast<double>(xs.size());

    double mean = 0.0;
    for (double x : xs)
      mean += x;
    mean /= n;

    // two-pass variance: numerically safer than sum-of-squares, and avoids
    // overflowing a uint64 accumulator when counts are in the billions
    double ss = 0.0;
    for (double x : xs) {
      double d = x - mean;
      ss += d * d;
    }
    double stddev = (n > 1.0) ? std::sqrt(ss / (n - 1.0)) : 0.0;
    double cv = (mean != 0.0) ? 100.0 * stddev / mean : 0.0;

    os << std::left << std::setw(24) << name << std::right << std::fixed
       << std::setprecision(1) << std::setw(18) << mean << std::setw(18)
       << stddev << std::setprecision(2) << std::setw(8) << cv << "%\n";
  }
}

}; // namespace measure_perf

#endif
