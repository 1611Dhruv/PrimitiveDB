#include "catalog.h"
#include "perf_measure.hpp"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"

// forward declaration
const Status ScanSelect(const string &result, const int projCnt,
                        const AttrDesc projNames[], const AttrDesc *attrDesc,
                        const Operator op, const char *filter,
                        const int reclen);

/*
 * Selects records from the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Select(const string &result, const int projCnt,
                       const attrInfo projNames[], const attrInfo *attr,
                       const Operator op, const char *attrValue) {
  // Qu_Select sets up things and then calls ScanSelect to do the actual work
  cout << "Doing QU_Select " << endl;

  Status status;

  // go through the projection list and look up each in the
  // attr cat to get an AttrDesc structure (for offset, length, etc)
  AttrDesc attrDescArray[projCnt];
  for (int i = 0; i < projCnt; i++) {
    Status status = attrCat->getInfo(projNames[i].relName,
                                     projNames[i].attrName, attrDescArray[i]);
    if (status != OK)
      return status;
  }

  // get output record length from attrdesc structures
  int reclen = 0;
  for (int i = 0; i < projCnt; i++) {
    reclen += attrDescArray[i].attrLen;
  }

  if (attr == NULL) {
    return ScanSelect(result, projCnt, attrDescArray, NULL, op, NULL, reclen);
  }

  // get AttrDesc structure for the attribute
  AttrDesc attrDesc;
  status = attrCat->getInfo(attr->relName, attr->attrName, attrDesc);
  if (status != OK)
    return status;

  return ScanSelect(result, projCnt, attrDescArray, &attrDesc, op, attrValue,
                    reclen);
}

const Status ScanSelect(const string &result, const int projCnt,
                        const AttrDesc projNames[], const AttrDesc *attrDesc,
                        const Operator op, const char *filter,
                        const int reclen) {
  cout << "Doing HeapFileScan Selection using ScanSelect()" << endl;

  Status status;

  // open the result table
  InsertFileScan resultRel(result, status);
  if (status != OK)
    return status;

  char outputData[reclen];
  Record outputRec;
  outputRec.data = (void *)outputData;
  outputRec.length = reclen;

  // start scan on table
  HeapFileScan scan(string(projNames[0].relName), status);
  if (status != OK)
    return status;

  int x;
  float f;

  if (attrDesc != NULL) {
    printf("type %d filter %s op %d \n", attrDesc->attrType, filter, op);

    switch (attrDesc->attrType) {
    case INTEGER: {
      x = atoi(filter);
      status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen,
                              (Datatype)attrDesc->attrType, (char *)&x, op);
      break;
    }

    case FLOAT: {
      f = atof(filter);
      status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen,
                              (Datatype)attrDesc->attrType, (char *)&f, op);
      break;
    }

    default: {
      status = scan.startScan(attrDesc->attrOffset, attrDesc->attrLen,
                              (Datatype)attrDesc->attrType, (char *)filter, op);
      break;
    }
    }
  } else {
    status = scan.startScan(0, 0, STRING, NULL, EQ);
  }

  if (status != OK)
    return status;

  // scan table
  struct RID RID;
  Record rec;
  int resultTupCnt = 0;

  measure_perf::Perf p({
      {
          PERF_TYPE_HW_CACHE,
          PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
              (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
          "l1d_read_miss",
      },
      {
          PERF_WALL_TIME,
          0,
          "latency",
      },
      {
          PERF_TYPE_HARDWARE,
          PERF_COUNT_HW_INSTRUCTIONS,
          "instructions",
      },
      {
          PERF_TYPE_HARDWARE,
          PERF_COUNT_HW_CPU_CYCLES,
          "cpu_cycles",
      },
      {
          PERF_TYPE_HARDWARE,
          PERF_COUNT_HW_CACHE_MISSES,
          "llc_misses",
      },
      {
          PERF_TYPE_HARDWARE,
          PERF_COUNT_HW_CACHE_REFERENCES,
          "llc_references",
      },
  });
  p.start();

  while (scan.scanNext(RID) == OK) {
    status = scan.getRecord(rec);
    ASSERT(status == OK);

    // we have a match, copy data into the output record
    int outputOffset = 0;
    for (int i = 0; i < projCnt; i++) {
      // copy the data out
      // if (0 == strcmp(projNames[i].relName, attrDesc->relName)) {
      memcpy(outputData + outputOffset,
             (char *)rec.data + projNames[i].attrOffset, projNames[i].attrLen);
      // }
      outputOffset += projNames[i].attrLen;
    } // end copy attrs
    // add the new record to the output relation
    struct RID outRID;
    status = resultRel.insertRecord(outputRec, outRID);

    ASSERT(status == OK);
    resultTupCnt++;
  } // end scan

  printf("select produced %d tuples\n", resultTupCnt);

  // Get the result
  std::vector<measure_perf::PerfResult> perf = p.stop();
  measure_perf::logOne(p, perf, "select_scan.perf");

  return OK;
}
