# Perf test suite

Query files that exercise Minirel at a scale where measurement is meaningful,
plus the data they load. Replaces the old toy soaps/stars fixtures.

## One-time setup: generate the data

The query files `load` binary relations that are **not** committed (they're
large and reproducible). Generate them once:

```sh
make data        # builds out/gen_data, writes data/*.data
```

This produces, from a fixed seed (so numbers are comparable run to run):

| file            | rows    | ~pages | vs. 100-frame pool | schema |
|-----------------|---------|--------|--------------------|--------|
| `data/R_1k.data`   | 1,000   | ~32    | fits (in-cache)    | R |
| `data/R_10k.data`  | 10,000  | ~320   | 3x (spills)        | R |
| `data/R_100k.data` | 100,000 | ~3200  | 32x (disk-bound)   | R |
| `data/S_1k.data`   | 1,000   | ~32    | fits               | R |

Schema `R` (28-byte tuple): `unique1 int` (shuffled key), `unique2 int`
(sequential key), `pct int` (i%100), `ten int` (i%10), `fk int` (join key),
`name char(8)`.

## Run a test

```sh
./test.sh scan_small          # dbcreate -> minirel < query -> dbdestroy
./test.sh -p run1 scan_large  # -p sets the log/measure prefix
./test.sh -j NL join_nl       # -j picks the join method
```

## The tests

| file          | what it isolates |
|---------------|------------------|
| `scan_small`  | scan + selection, **in-cache** (R_1k fits the pool) |
| `scan_large`  | scan + selection, **disk/buffer-bound** (R_100k, 32x pool) — compare to `scan_small` |
| `selectivity` | fixed input (R_10k), result size swept 1%→100% — isolates materialization cost from scan cost |
| `join_nl`     | Nested Loops equi-join (1k×1k = 1M comparisons) — the only fully implemented join |

Selectivity is controlled by `pct` (0..99): `where pct < K` selects ~K% of rows.
