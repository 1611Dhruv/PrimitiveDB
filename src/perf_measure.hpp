#ifndef PERF_MEASURE_HPP
#define PERF_MEASURE_HPP

// Logging harness built on the counter core (perf_counters.hpp): runs a
// workload N times and appends one CSV row of counter values per round.
//
// The free functions and LogPrefix are *declared* here and *defined* once in
// perf_measure.C, so this header is safe to include from many translation
// units. Only measure() lives here, because it is a template.

#include "perf_counters.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace measure_perf {

// Optional path stem prepended to every log path (set once, e.g. by minirel
// from its CLI prefix). Empty by default, in which case paths are used as-is.
extern std::string LogPrefix;

// Reads the comma-separated header line of an existing CSV and returns each
// column name -> its index. Returns {} if the file is empty/absent.
std::unordered_map<std::string, int> parseHeader(const std::string &path);

// Appends one row of this round's counter values to LogPrefix + path, writing
// a header line first if the file is new.
void logOne(const Perf &perf, const std::vector<PerfResult> &round,
            const std::string &path);

// Runs `work` warmup times (discarded), then `rounds` times, logging each.
template <typename Fn>
void measure(Perf &perf, Fn &&work, const std::string &path, int rounds = 10,
             int warmup = 1) {
  // Discarded warmup rounds: fault in pages, warm caches and predictors.
  for (int i = 0; i < warmup; i++) {
    perf.start();
    auto sink = work();
    perf.stop();
    doNotOptimize(sink);
  }

  for (int r = 0; r < rounds; r++) {
    perf.start();
    auto sink = work();
    std::vector<PerfResult> round = perf.stop();
    doNotOptimize(sink);
    logOne(perf, round, path);
  }
}

} // namespace measure_perf

#endif
