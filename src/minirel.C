#include "catalog.h"
#include "perf_measure.hpp"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"
#include <limits.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

DB db;
Error error;

BufMgr *bufMgr;
RelCatalog *relCat;
AttrCatalog *attrCat;

JoinType JoinMethod;
// path stem the perf "measure" code uses for its logs (defined in perf_measure.C)
using measure_perf::LogPrefix;

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " dbname" << endl;
    return 1;
  }

  // Resolve the log-path prefix BEFORE chdir. argv[3], if given, is the prefix
  // the measurement code uses to build log paths. We anchor a relative prefix
  // to the launch directory, because the chdir below moves us into the database
  // directory (often a throwaway), where we don't want logs to land.
  {
    const char *prefix = (argc >= 4) ? argv[3] : "minirel";
    if (prefix[0] == '/') {
      LogPrefix = prefix; // already absolute
    } else {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof cwd) == NULL) {
        perror("getcwd");
        exit(1);
      }
      LogPrefix = std::string(cwd) + "/" + prefix;
    }
  }

  if (chdir(argv[1]) < 0) {
    perror("chdir");
    exit(1);
  }

  JoinMethod = NLJoin; // default join method
  if (argc >= 3)       // alternative join method specified
  {
    if (strcmp(argv[2], "SM") == 0)
      JoinMethod = SMJoin;
    else if (strcmp(argv[2], "HJ") == 0)
      JoinMethod = HashJoin;
  }

  // create buffer manager

  bufMgr = new BufMgr(100);

  // open relation and attribute catalogs

  Status status;
  relCat = new RelCatalog(status);
  if (status == OK)
    attrCat = new AttrCatalog(status);
  if (status != OK) {
    error.print(status);
    exit(1);
  }

  cout << "Welcome to Minirel" << endl;
  cout << "    Using ";
  if (JoinMethod == NLJoin) {
    cout << "Nested Loops Join Method" << endl;
  } else if (JoinMethod == HashJoin) {
    cout << "Hash Join Method" << endl;
  } else {
    cout << "Sort Merge Join Method" << endl;
  }
  cout << "    Logging perf measurements to " << LogPrefix << "*" << endl;

  extern void parse();
  parse();

  return 0;
}
