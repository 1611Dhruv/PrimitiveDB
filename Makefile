#
# Makefile for Minirel
#
# Run from the repository root. Sources live in src/, the parser in
# src/parser/, and all build output (objects + binaries) goes to out/.
#

.SUFFIXES:

#
# Directory layout
#
SRCDIR =	src
PARSER =	$(SRCDIR)/parser
OUTDIR =	out
OBJDIR =	$(OUTDIR)/obj

#
# Compiler and loader definitions
#
CXX =		g++
CXXFLAGS =	-g -Wall -DDEBUG -I$(SRCDIR) #-DDEBUGIND -DDEBUGBUF
LDFLAGS =
LIBS =		-lm

#
# Object files (names only; resolved into $(OBJDIR) below)
#
OBJS =		buf.o bufHash.o db.o heapfile.o error.o page.o \
		catalog.o create.o destroy.o \
		help.o load.o print.o quit.o insert.o delete.o \
		select.o join.o sort.o partition.o joinHT.o perf_measure.o

DBOBJS =	catalog.o buf.o bufHash.o db.o heapfile.o error.o page.o

OBJS_O =	$(addprefix $(OBJDIR)/,$(OBJS))
DBOBJS_O =	$(addprefix $(OBJDIR)/,$(DBOBJS))

# Built by the parser sub-make (src/parser/makefile emits ../parser.o)
PARSEROBJ =	$(SRCDIR)/parser.o

BINS =		$(OUTDIR)/minirel $(OUTDIR)/dbcreate $(OUTDIR)/dbdestroy

.PHONY: all clean perf parser

all:		$(BINS)

#
# Compile any src/%.C into out/obj/%.o
#
$(OBJDIR)/%.o:	$(SRCDIR)/%.C | $(OBJDIR)
		$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
		mkdir -p $(OBJDIR)

#
# Parser: delegate to its own makefile, which produces $(SRCDIR)/parser.o
#
parser:		$(PARSEROBJ)

$(PARSEROBJ):
		$(MAKE) -C $(PARSER)

#
# Executables
#
$(OUTDIR)/minirel:	$(OBJDIR)/minirel.o $(OBJS_O) $(PARSEROBJ)
		$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OUTDIR)/dbcreate:	$(OBJDIR)/dbcreate.o $(DBOBJS_O)
		$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OUTDIR)/dbdestroy:	$(OBJDIR)/dbdestroy.o
		$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

#
# Performance-counter test harness (needs C++17)
#
perf:		$(OUTDIR)/perf_test

$(OUTDIR)/perf_test:	$(SRCDIR)/perf_test.C $(SRCDIR)/perf_measure.C \
			$(SRCDIR)/perf_measure.hpp $(SRCDIR)/perf_counters.hpp | $(OUTDIR)
		$(CXX) -std=c++17 $(CXXFLAGS) -o $@ \
			$(SRCDIR)/perf_test.C $(SRCDIR)/perf_measure.C

#
# Perf-test data: write the standard relations into data/ via gen_data.py
# (R_1k fits the buffer pool; R_10k/R_100k spill it; S_1k is the join build
# side). See tests/perf/README.md.
#
GEN    = python3 data/gen_data.py
SCHEMA = unique1:int:uniq,unique2:int:seq,pct:int:mod(100),ten:int:mod(10),fk:int:mod(1000),name:char(8)

.PHONY: data
data:
		$(GEN) -n 1000   -s "$(SCHEMA)" -o data/R_1k.data
		$(GEN) -n 10000  -s "$(SCHEMA)" -o data/R_10k.data
		$(GEN) -n 100000 -s "$(SCHEMA)" -o data/R_100k.data
		$(GEN) -n 1000   -s "$(SCHEMA)" -o data/S_1k.data

$(OUTDIR):
		mkdir -p $(OUTDIR)

clean:
		rm -rf $(OBJDIR)
		rm -f $(BINS) $(OUTDIR)/perf_test $(PARSEROBJ)
		$(MAKE) -C $(PARSER) clean
