#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static long pe_open(struct perf_event_attr *a) {
  a->size = sizeof(*a);
  a->disabled = 1;
  a->exclude_kernel = 1;
  a->exclude_hv = 1;
  return syscall(__NR_perf_event_open, a, 0, -1, -1, 0);
}

int main() {
  struct perf_event_attr a;
  memset(&a, 0, sizeof(a));
  a.type = PERF_TYPE_HW_CACHE;
  a.config = PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
             (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
  int fd = pe_open(&a);
  if (fd < 0) {
    perror("perf_event_open");
    return 1;
  }

  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

  static int big[8'000'000];
  long s = 0;
  for (int i = 0; i < 8'000'000; i++) {
    s += big[i * 16 % 8'000'000];
  }

  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  long long v = 0;
  read(fd, &v, sizeof(v));
  printf("L1D read misses: %lld (sink %ld)\n", v, s);
}
