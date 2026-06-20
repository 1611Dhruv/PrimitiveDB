//=============================================================================
// gen_data — generate a binary relation for Minirel perf testing.
//
// Emits fixed-width tuples that match this schema EXACTLY (declared column
// order, no struct padding — Minirel's `load` just reads sum(attrLen) bytes
// per tuple):
//
//     create table R (unique1 int,    -- shuffled unique key 0..N-1 (random access)
//                      unique2 int,    -- sequential unique key 0..N-1
//                      pct     int,    -- i % 100   (selectivity knob: `where pct < K` ~ K%)
//                      ten     int,    -- i % 10
//                      fk      int,    -- foreign key: i % fkRange (join key)
//                      name    char(8));-- "%07d" of unique1, NUL-padded
//
// Tuple width = 5*4 + 8 = 28 bytes.
//
// Usage:  gen_data <numRows> <fkRange> <outfile>
//=============================================================================

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

static const int NAME_LEN = 8;
static const int TUPLE_WIDTH = 5 * sizeof(int32_t) + NAME_LEN; // 28

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <numRows> <fkRange> <outfile>\n", argv[0]);
    return 1;
  }
  const int64_t n = atoll(argv[1]);
  const int32_t fkRange = atoi(argv[2]);
  const char *outFile = argv[3];
  if (n <= 0 || fkRange <= 0) {
    fprintf(stderr, "numRows and fkRange must be positive\n");
    return 1;
  }

  // A fixed seed keeps generated data (and therefore any checksum) stable
  // across runs, which matters for trustworthy before/after comparisons.
  srand(42);

  // unique1: a shuffled permutation of 0..n-1, so probing by unique1 is random.
  std::vector<int32_t> unique1(n);
  std::iota(unique1.begin(), unique1.end(), 0);
  for (int64_t i = n - 1; i > 0; --i) {
    int64_t j = rand() % (i + 1);
    std::swap(unique1[i], unique1[j]);
  }

  std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
  if (!out) {
    fprintf(stderr, "cannot open %s for writing\n", outFile);
    return 1;
  }

  char rec[TUPLE_WIDTH];
  for (int64_t i = 0; i < n; ++i) {
    int32_t u1 = unique1[i];
    int32_t u2 = (int32_t)i;
    int32_t pct = (int32_t)(i % 100);
    int32_t ten = (int32_t)(i % 10);
    int32_t fk = (int32_t)(i % fkRange);

    char *p = rec;
    memcpy(p, &u1, 4);  p += 4;
    memcpy(p, &u2, 4);  p += 4;
    memcpy(p, &pct, 4); p += 4;
    memcpy(p, &ten, 4); p += 4;
    memcpy(p, &fk, 4);  p += 4;
    memset(p, 0, NAME_LEN);
    snprintf(p, NAME_LEN, "%07d", u1); // 7 digits + NUL fits in 8 bytes

    out.write(rec, TUPLE_WIDTH);
  }
  out.close();

  printf("wrote %lld tuples (%d bytes each, %lld bytes total) to %s\n",
         (long long)n, TUPLE_WIDTH, (long long)n * TUPLE_WIDTH, outFile);
  return 0;
}
