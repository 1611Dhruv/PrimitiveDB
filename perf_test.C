#include <iostream>
#include <linux/perf_event.h>

#include "perf_counters.hpp"

int main() {
  measure_perf::Perf perf({
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "CPU Cycles"},
      {PERF_TYPE_HW_CACHE,
       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
           (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
       "L1D Cache Accesses"},
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "Num Instructions"},
  });

  perf.start();
  const int N = 1'000'000;
  static int data[N] = {0};
  int s = 0;
  for (int i = 0; i < N; i++) {
    s += data[i];
  }

  data[0] = s;
  auto res = perf.stop();
  for (auto &metric : res) {
    std::cout << metric.name << ": " << metric.measure << std::endl;
  }
}
