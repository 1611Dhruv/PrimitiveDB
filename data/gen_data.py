#!/usr/bin/env python3
"""Generate a fixed-width binary relation for Minirel's `load table`.

Minirel's on-disk format is just each tuple's columns packed back-to-back with
no padding (it reads sum(column widths) bytes per row):

    int        -> 4 bytes, little-endian signed
    real/float -> 4 bytes, IEEE-754 little-endian
    char(N)    -> N bytes, NUL-padded

So the schema you pass here MUST match the `create table` you load it into.
This script prints that exact DDL (see --table) so you can copy it verbatim.

Schema: a comma-separated list of  name:type[:pattern]  fields.

    types:   int | real | char(N)
    int patterns:   seq (default) | uniq | mod(K) | rand(LO,HI) | const(V)
    real patterns:  seq (default) | rand(LO,HI) | const(V)
    char pattern:   zero-padded row index (default)

  seq    sequential 0..N-1            (unique, ordered key)
  uniq   shuffled permutation 0..N-1  (unique, random order -> random access)
  mod(K) row_index % K                (selectivity knob / foreign key)
  rand   uniform random in [LO,HI]
  const  the same value in every row

Example:
  gen_data.py -n 10000 -o data/R.data \\
      -s "unique1:int:uniq,unique2:int:seq,pct:int:mod(100),fk:int:mod(1000),name:char(8)"
"""
import argparse
import random
import re
import struct
import sys
from array import array


def split_fields(s):
    """Split on top-level commas only, so mod(100)/rand(1,9) survive."""
    fields, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            fields.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        fields.append(cur)
    return fields


def make_int_gen(pattern, n):
    if pattern in (None, "seq"):
        return lambda i: i
    if pattern == "uniq":
        perm = array("i", range(n))
        random.shuffle(perm)
        return lambda i: perm[i]
    m = re.fullmatch(r"mod\((\d+)\)", pattern)
    if m:
        k = int(m.group(1))
        return lambda i: i % k
    m = re.fullmatch(r"rand\((-?\d+),(-?\d+)\)", pattern)
    if m:
        lo, hi = int(m.group(1)), int(m.group(2))
        return lambda i: random.randint(lo, hi)
    m = re.fullmatch(r"const\((-?\d+)\)", pattern)
    if m:
        v = int(m.group(1))
        return lambda i: v
    sys.exit(f"unknown int pattern: {pattern!r}")


def make_real_gen(pattern):
    if pattern in (None, "seq"):
        return lambda i: float(i)
    m = re.fullmatch(r"rand\((-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)\)", pattern)
    if m:
        lo, hi = float(m.group(1)), float(m.group(2))
        return lambda i: random.uniform(lo, hi)
    m = re.fullmatch(r"const\((-?\d+(?:\.\d+)?)\)", pattern)
    if m:
        v = float(m.group(1))
        return lambda i: v
    sys.exit(f"unknown real pattern: {pattern!r}")


def make_char_gen(n, pattern):
    # default: the row index, zero-padded, leaving room for a NUL terminator.
    width = max(n - 1, 1)
    return lambda i: f"{i % (10 ** width):0{width}d}".encode()[: n - 1]


def parse_schema(s, n):
    """Return (columns, struct_format, ddl_parts).

    columns: list of (name, value_gen).  struct_format packs one whole row.
    """
    columns, fmt, ddl = [], "<", []
    for field in split_fields(s):
        parts = field.strip().split(":", 2)
        if len(parts) < 2:
            sys.exit(f"bad field {field!r} (need name:type[:pattern])")
        name, typ = parts[0].strip(), parts[1].strip()
        pattern = parts[2].strip() if len(parts) == 3 else None

        if typ in ("int", "integer"):
            columns.append((name, make_int_gen(pattern, n)))
            fmt += "i"
            ddl.append(f"{name} int")
        elif typ in ("real", "float"):
            columns.append((name, make_real_gen(pattern)))
            fmt += "f"
            ddl.append(f"{name} real")
        else:
            m = re.fullmatch(r"char\((\d+)\)", typ)
            if not m:
                sys.exit(f"unknown type {typ!r} in {field!r}")
            width = int(m.group(1))
            columns.append((name, make_char_gen(width, pattern)))
            fmt += f"{width}s"
            ddl.append(f"{name} char({width})")
    return columns, fmt, ddl


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("-n", "--rows", type=int, required=True, help="number of tuples")
    ap.add_argument("-s", "--schema", required=True, help="name:type[:pattern],...")
    ap.add_argument("-o", "--out", required=True, help="output .data file")
    ap.add_argument("-t", "--table", default="R", help="relation name for the DDL")
    ap.add_argument("--seed", type=int, default=42, help="RNG seed (reproducible)")
    args = ap.parse_args()

    if args.rows <= 0:
        sys.exit("rows must be positive")
    random.seed(args.seed)

    columns, fmt, ddl = parse_schema(args.schema, args.rows)
    row = struct.Struct(fmt)
    gens = [g for _, g in columns]

    with open(args.out, "wb") as f:
        buf = bytearray()
        for i in range(args.rows):
            buf += row.pack(*[g(i) for g in gens])
            if len(buf) >= 1 << 20:  # flush ~1 MB at a time
                f.write(buf)
                del buf[:]
        f.write(buf)

    ddl_str = f"create table {args.table} ({', '.join(ddl)});"
    print(f"wrote {args.rows} tuples ({row.size} B each, "
          f"{args.rows * row.size} B total) to {args.out}", file=sys.stderr)
    print(f"-- matching DDL:\n{ddl_str}", file=sys.stderr)


if __name__ == "__main__":
    main()
