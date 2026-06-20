#include "catalog.h"
#include "joinHT.h"
#include "perf_measure.hpp"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"
#include <linux/perf_event.h>

extern JoinType JoinMethod;

const int matchRec(const Record &outerRec, const Record &innerRec,
                   const AttrDesc &attrDesc1, const AttrDesc &attrDesc2);

/*
 * Joins two relations.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

// implementation of nested loops join goes here
const Status QU_NL_Join(const string &result, const int projCnt,
                        const attrInfo projNames[], const attrInfo *attr1,
                        const Operator op, const attrInfo *attr2) {
  Status status;
  int resultTupCnt = 0;

  if (attr1->attrType != attr2->attrType || attr1->attrLen != attr2->attrLen) {
    return ATTRTYPEMISMATCH;
  }

  // go through the projection list and look up each in the
  // attr cat to get an AttrDesc structure (for offset, length, etc)
  AttrDesc attrDescArray[projCnt];
  for (int i = 0; i < projCnt; i++) {
    Status status = attrCat->getInfo(projNames[i].relName,
                                     projNames[i].attrName, attrDescArray[i]);
    if (status != OK) {
      return status;
    }
  }

  // get AttrDesc structure for the first join attribute
  AttrDesc attrDesc1;
  status = attrCat->getInfo(attr1->relName, attr1->attrName, attrDesc1);
  if (status != OK) {
    return status;
  }
  // get AttrDesc structure for the first join attribute
  AttrDesc attrDesc2;
  status = attrCat->getInfo(attr2->relName, attr2->attrName, attrDesc2);
  if (status != OK) {
    return status;
  }

  // get output record length from attrdesc structures
  int reclen = 0;
  for (int i = 0; i < projCnt; i++) {
    reclen += attrDescArray[i].attrLen;
  }

  // open the result table
  InsertFileScan resultRel(result, status);
  if (status != OK) {
    return status;
  }

  char outputData[reclen];
  Record outputRec;
  outputRec.data = (void *)outputData;
  outputRec.length = reclen;

  // start scan on outer table
  HeapFileScan outerScan(string(attrDesc1.relName), status);
  if (status != OK) {
    return status;
  }
  status = outerScan.startScan(0, 0, STRING, NULL, EQ);
  if (status != OK) {
    return status;
  }

  // scan outer table
  RID outerRID;
  Record outerRec;

  Operator myop;
  switch (op) {
  case EQ:
    myop = EQ;
    break;
  case GT:
    myop = LT;
    break;
  case GTE:
    myop = LTE;
    break;
  case LT:
    myop = GT;
    break;
  case LTE:
    myop = GTE;
    break;
  case NE:
    myop = NE;
    break;
  }

  while (outerScan.scanNext(outerRID) == OK) {
    status = outerScan.getRecord(outerRec);
    ASSERT(status == OK);

    // scan inner table
    HeapFileScan innerScan(string(attrDesc2.relName), status);
    if (status != OK) {
      return status;
    }
    status = innerScan.startScan(
        attrDesc2.attrOffset, attrDesc2.attrLen, (Datatype)attrDesc2.attrType,
        ((char *)outerRec.data) + attrDesc1.attrOffset, myop);
    if (status != OK) {
      return status;
    }

    RID innerRID;
    while (innerScan.scanNext(innerRID) == OK) {
      Record innerRec;
      status = innerScan.getRecord(innerRec);
      ASSERT(status == OK);

      // we have a match, copy data into the output record
      int outputOffset = 0;
      for (int i = 0; i < projCnt; i++) {
        // copy the data out of the proper input file (inner vs. outer)
        if (0 == strcmp(attrDescArray[i].relName, attrDesc1.relName)) {
          memcpy(outputData + outputOffset,
                 (char *)outerRec.data + attrDescArray[i].attrOffset,
                 attrDescArray[i].attrLen);
        } else // get data from the inner record
        {
          memcpy(outputData + outputOffset,
                 (char *)innerRec.data + attrDescArray[i].attrOffset,
                 attrDescArray[i].attrLen);
        }
        outputOffset += attrDescArray[i].attrLen;
      } // end copy attrs

      // add the new record to the output relation
      RID outRID;
      status = resultRel.insertRecord(outputRec, outRID);
      ASSERT(status == OK);
      resultTupCnt++;
    } // end scan inner
  } // end scan outer
  printf("tuple nested join produced %d result tuples \n", resultTupCnt);
  return OK;
}

// implementation of sort merge join goes here
const Status QU_SM_Join(const string &result, const int projCnt,
                        const attrInfo projNames[], const attrInfo *attr1,
                        const Operator op, const attrInfo *attr2) {
  int resultTupCnt = 0;

  if (attr1->attrType != attr2->attrType || attr1->attrLen != attr2->attrLen) {
    return ATTRTYPEMISMATCH;
  }

  printf("sm join produced %d result tuples \n", resultTupCnt);
  return OK;
}

// This is really not a hash join implementation.  It is actually a block nested
// loops join that uses hashing on each block of outer tuples read.
// It assumes that blocks of the outer table are read M pages at a time

const Status QU_Hash_Join(const string &result, const int projCnt,
                          const attrInfo projNames[], const attrInfo *attr1,
                          const Operator op, const attrInfo *attr2) {
  int resultTupCnt = 0;

  if (attr1->attrType != attr2->attrType || attr1->attrLen != attr2->attrLen) {
    return ATTRTYPEMISMATCH;
  }

  printf("blockNL Hash join produced %d result tuples \n", resultTupCnt);
  return OK;
}

const Status QU_Join(const string &result, const int projCnt,
                     const attrInfo projNames[], const attrInfo *attr1,
                     const Operator op, const attrInfo *attr2) {

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

  Status stat;
  p.start();
  if ((JoinMethod == NLJoin) || ((JoinMethod == HashJoin) && (op != EQ))) {
    stat = QU_NL_Join(result, projCnt, projNames, attr1, op, attr2);
  } else if (JoinMethod == SMJoin) {
    stat = QU_SM_Join(result, projCnt, projNames, attr1, op, attr2);
  } else
    stat = QU_Hash_Join(result, projCnt, projNames, attr1, op, attr2);

  // Get the result
  std::vector<measure_perf::PerfResult> perf = p.stop();
  measure_perf::logOne(p, perf, "join.perf");

  return stat;
}

const int matchRec(const Record &outerRec, const Record &innerRec,
                   const AttrDesc &attrDesc1, const AttrDesc &attrDesc2) {
  int tmpInt1, tmpInt2;
  float tmpFloat1, tmpFloat2;

  switch (attrDesc1.attrType) {
  case INTEGER:
    memcpy(&tmpInt1, (char *)outerRec.data + attrDesc1.attrOffset, sizeof(int));
    memcpy(&tmpInt2, (char *)innerRec.data + attrDesc2.attrOffset, sizeof(int));
    return tmpInt1 - tmpInt2;

  case FLOAT:
    memcpy(&tmpFloat1, (char *)outerRec.data + attrDesc1.attrOffset,
           sizeof(float));
    memcpy(&tmpFloat2, (char *)innerRec.data + attrDesc2.attrOffset,
           sizeof(float));
    return int(tmpFloat1 - tmpFloat2);

  case STRING:
    return strcmp((char *)outerRec.data + attrDesc1.attrOffset,
                  (char *)innerRec.data + attrDesc2.attrOffset);
  }

  return 0;
}
