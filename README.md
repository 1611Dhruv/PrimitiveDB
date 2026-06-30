# PrimitiveDB — cache-locality study

A self-contained harness that **implements and measures** the two storage and
execution techniques that separate a textbook database engine from a fast one:

1. **PAX hybrid columnar storage** vs the classic row-major (NSM) slotted page.
2. **Cache-conscious (radix-partitioned) joins** vs a naive hash join.

The point of the project is that neither claim is asserted — both are backed by
**real hardware performance counters** read through `perf_event_open(2)` (no
`perf` binary, no estimates), and the whole rig reproduces on any Linux box in a
few seconds.

> The harness lives in [`bench/`](bench/) and is independent of the rest of the
> repo (the `src/` minirel engine it grew out of). Everything below builds and
> runs on its own.

---

## The one idea

Everything here rests on a single hardware fact: **the CPU never reads one byte
from memory — it reads a 64-byte cache line.** So performance comes down to one
question:

> *Of every cache line you drag in from DRAM, what fraction do you actually use?*

- **PAX** raises that fraction on a *scan*. A row-store puts a whole 64-byte
  tuple on a line; a query reading one 4-byte column uses 4/64 = 6% of every
  line. Store that column contiguously and you use 100%.
- **Radix joins** raise the *hit rate* on random lookups. A hash table bigger
  than the cache makes every probe a random DRAM miss (~100 ns); partition the
  work so each piece fits in cache and the same probes become ~1 ns hits.

Both are *proven* below with counted misses.

---

## Headline results

AMD Ryzen 9 7900X (L1d 32 KiB/core, L2 1 MiB/core, L3 32 MiB), g++
`-O2 -march=native`, counters via `perf_event_open`, measured 2026-06-30.
Full tables, run-to-run variance, and methodology in
[`bench/BENCHMARKS.md`](bench/BENCHMARKS.md).

**1. Single-column scan of a 488 MiB / 16-column table — PAX vs row-store:**

| layout    | time   | speedup | L1D misses | why                                       |
|-----------|-------:|--------:|-----------:|-------------------------------------------|
| NSM (row) | 10.1 ms| 1.00×   | 8.0 M      | 1 miss/tuple — 60 of 64 line bytes wasted |
| **PAX**   | 1.7 ms | **6.0×**| 0.55 M     | column packed contiguously, 16 values/line|
| Columnar  | 0.45 ms| 22×     | 0.50 M     | the limiting case (pure struct-of-arrays) |

→ **PAX: ~6× faster, ~14.5× fewer L1D misses**, identical aggregate checksum.

**2. Equi-join with the build table far past the LLC (R=8M ⋈ S=48M, ~128 MiB table):**

| join         | time    | speedup   | L3 misses | cyc/insn |
|--------------|--------:|----------:|----------:|---------:|
| naive-HT     | 2170 ms | 1.00×     | ~168 M    | 3.75     |
| **radix-HT** | 816 ms  | **2.66×** | ~1.0 M    | 0.87     |

→ **Radix: ~2.7× faster, ~170× fewer L3 misses**, identical output checksum.
IPC rises from ~0.26 to ~1.15 as the CPU stops stalling on DRAM.

(At a smaller join where the table only just reaches L3, R=2M ⋈ S=16M, radix is
1.5× faster with ~153× fewer L3 misses — the win *grows* as the table outgrows
the cache. See BENCHMARKS.)

Every reported speedup clears its own run-to-run noise (times stable to ~4%) by
more than an order of magnitude.

---

## Quick start

```sh
cd bench
make            # g++ -O2 -march=native -std=c++17
./pax_bench     # defaults: scan 8M rows, join 2M |><| 16M
./pax_bench 8000000 8000000 48000000   # the large (table ≫ L3) join
./pax_bench <scan_rows> <build_rows> <probe_rows>   # custom sizes
```

Reading the cache counters needs `cat /proc/sys/kernel/perf_event_paranoid` to
be `<= 2`. Without it, timings still work and the cache columns read 0 — the
program warns and continues. The process pins itself to core 0 for stable
measurement.

### What it measures

- **`scanBenchmark`** materializes the same 16-column / 64-byte table three ways
  (NSM row-major, PAX page-grouped minipages, pure columnar) and runs the same
  single-column `SUM` over each. All three must return an identical checksum
  before any timing is trusted.
- **`joinBenchmark`** runs `R(key,val) ⋈ S(key,val)` two ways — one big hash
  table (naive) vs radix-partitioned so each partition's table is cache-resident
  — and again gates on identical checksums.

Counters per workload: L1D read miss, LLC (L3) miss, cycles, instructions; each
workload is warmed up once, run 3×, and the fastest run's counters are reported.

---

## Files

```
bench/
  pax_bench.cpp     NSM / PAX / columnar scans + naive vs radix hash join
  perf_counters.hpp perf_event_open RAII wrapper (no `perf` binary needed)
  Makefile          standalone build (independent of the minirel Makefile)
  README.md         what the harness does and how the layouts differ
  BENCHMARKS.md     measured results, variance, and methodology
  TUTORIAL.md       build the whole thing from an empty file, step by step
```

---

## Learn how it works

[**`bench/TUTORIAL.md`**](bench/TUTORIAL.md) is a from-scratch, weekend-sized
guide: open your first `perf_event_open` counter, watch the miss count react to
an access pattern, then build the NSM/PAX/columnar layouts and the radix join
yourself — with predicted-vs-measured checkpoints at every step (including the
two bugs that bite everyone: an XOR checksum that cancels to zero, and sharing
the partitioning hash with the in-table hash, which makes the "optimized" join
*200× slower*).

The point isn't the code; it's learning to say
*cache-line waste → predicted misses → measured misses → time* and mean it.
