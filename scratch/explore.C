#include <asm/unistd_64.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <linux/perf_event.h>
#include <unistd.h>

long perf_open(perf_event_attr *a) {
  a->size = sizeof(*a);
  // We dont want to start logging
  a->disabled = 1;
  // We dont want perf to log kernel and hypervisor insts
  a->exclude_kernel = 1;
  a->exclude_hv = 1;

  // measure current procs (> 0 some other proc, -1 all procs)
  int pid = 0;
  // >=0 specific CPU, -1 all CPU
  int cpu = -1;
  // Group multiple perf counters into 1 if >=0
  int group_fd = -1;
  int flags = 0;
  return syscall(__NR_perf_event_open, a, pid, cpu, group_fd, flags);
}

// We will explore HW_CACHE availabilities and ops I can do on them
const char *cache_id_names[] = {
    [PERF_COUNT_HW_CACHE_L1D] = "L1D",
    [PERF_COUNT_HW_CACHE_L1I] = "L1I",
    [PERF_COUNT_HW_CACHE_LL] = "LLC (L3)",
    [PERF_COUNT_HW_CACHE_DTLB] = "DTLB",
    [PERF_COUNT_HW_CACHE_ITLB] = "ITLB",
    [PERF_COUNT_HW_CACHE_BPU] = "BPU",
    [PERF_COUNT_HW_CACHE_NODE] = "NUMA-Node",
};

// ops
const char *op_id_names[] = {
    [PERF_COUNT_HW_CACHE_OP_READ] = "READ",
    [PERF_COUNT_HW_CACHE_OP_WRITE] = "WRITE",
    [PERF_COUNT_HW_CACHE_OP_PREFETCH] = "PREFETCH",
};

// results
const char *result_id_names[] = {
    [PERF_COUNT_HW_CACHE_RESULT_ACCESS] = "ACCESS",
    [PERF_COUNT_HW_CACHE_RESULT_MISS] = "MISS",
};

// String map using the kernel macros directly as indices
const char *hw_event_names[] = {
    [PERF_COUNT_HW_CPU_CYCLES] = "CPU_CYCLES",
    [PERF_COUNT_HW_INSTRUCTIONS] = "INSTRUCTIONS",
    [PERF_COUNT_HW_CACHE_REFERENCES] = "CACHE_REFERENCES",
    [PERF_COUNT_HW_CACHE_MISSES] = "CACHE_MISSES",
    [PERF_COUNT_HW_BRANCH_INSTRUCTIONS] = "BRANCH_INSTRUCTIONS",
    [PERF_COUNT_HW_BRANCH_MISSES] = "BRANCH_MISSES",
    [PERF_COUNT_HW_BUS_CYCLES] = "BUS_CYCLES",
    [PERF_COUNT_HW_STALLED_CYCLES_FRONTEND] = "STALLED_CYCLES_FRONTEND",
    [PERF_COUNT_HW_STALLED_CYCLES_BACKEND] = "STALLED_CYCLES_BACKEND",
    [PERF_COUNT_HW_REF_CPU_CYCLES] = "REF_CPU_CYCLES",
};

#define ARR_SZ(arr) (sizeof(arr) / sizeof(arr[0]))
int main() {
  perf_event_attr a;

  printf("PERF_TYPE_HW_CACHE: \n");
  printf("%-12s | %-8s | %-8s | %s\n", "Cache Type", "Op", "Result",
         "Available");
  printf("--------------------------------------------------------\n");

  for (size_t c = 0; c < ARR_SZ(cache_id_names); c++) {
    if (cache_id_names[c] == NULL)
      continue;
    for (size_t op = 0; op < ARR_SZ(op_id_names); op++) {
      if (op_id_names[op] == NULL)
        continue;
      for (size_t res = 0; res < ARR_SZ(result_id_names); res++) {
        if (result_id_names[res] == NULL)
          continue;

        // Clear the event
        memset(&a, 0, sizeof(a));
        a.type = PERF_TYPE_HW_CACHE;
        a.config = c | (op << 8) | (res << 16);
        int fd = perf_open(&a);
        if (fd >= 0) {
          printf("%-12s | %-8s | %-8s | %s\n", cache_id_names[c],
                 op_id_names[op], result_id_names[res], "[YES]");
          close(fd);
        } else {
          printf("%-12s | %-8s | %-8s | %s (Err: %s)\n", cache_id_names[c],
                 op_id_names[op], result_id_names[res], "[NO]",
                 strerror(errno));
        }
      }
    }
  }

  printf("\n\n");
  printf("PERF_TYPE_HARDWARE: \n");
  for (size_t hw_config = 0; hw_config < PERF_COUNT_HW_MAX; hw_config++) {
    const char *hw_name = (hw_config < ARR_SZ(hw_event_names) &&
                           hw_event_names[hw_config] != NULL)
                              ? hw_event_names[hw_config]
                              : "UNKNOWN_HW_EVENT";
    memset(&a, 0, sizeof(a));
    a.type = PERF_TYPE_HARDWARE;
    a.config = hw_config;
    int fd = perf_open(&a);
    if (fd >= 0) {
      printf("Event %zu [%s]: AVAILABLE\n", hw_config, hw_name);
      close(fd);
    } else {
      printf("Event %zu [%s]: UNAVAILABLE\n", hw_config, hw_name);
    }
  }
  return 0;
}
