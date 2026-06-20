#include <iostream>
#include <linux/perf_event.h>

#include "perf_measure.hpp"

int main() {
  measure_perf::Perf perf({
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "CPU Cycles"},
      {PERF_TYPE_HW_CACHE,
       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
           (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16),
       "L1D Cache Accesses"},
      {PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "Num Instructions"},
  });

  const int N = 1'000'000;
  static int data[N] = {0};

  auto work = [&]() -> int {
    int s = 0;
    for (int i = 0; i < N; i++) {
      s += data[i];
    }
    return s;
  };

  measure_perf::measure(perf, work, "test_measure.csv");
}
