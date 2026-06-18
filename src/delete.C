#include "catalog.h"
#include "query.h"

/*
 * Deletes records from a specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Delete(const string &relation, const string &attrName,
                       const Operator op, const Datatype type,
                       const char *attrValue) {
  // part 6
  Status status;

  HeapFileScan scan(relation, status);
  if (status != OK)
    return status;
  char *filter;
  int x;
  float fx;

  if (attrName.empty()) {
    status = scan.startScan(0, 0, STRING, NULL, EQ);
    if (status != OK)
      return status;

  } else {
    AttrDesc attrdesc;
    status = attrCat->getInfo(relation, attrName, attrdesc);
    if (status != OK)
      return status;
    switch (type) {
    case INTEGER: {
      x = atoi(attrValue);
      filter = (char *)&x;
      status = scan.startScan(attrdesc.attrOffset, attrdesc.attrLen, type,
                              filter, op);
      if (status != OK)
        return status;
      break;
    }

    case FLOAT: {
      fx = atof(attrValue);
      filter = (char *)&fx;
      status = scan.startScan(attrdesc.attrOffset, attrdesc.attrLen, type,
                              filter, op);
      if (status != OK)
        return status;
      break;
    }

    default: {
      status = scan.startScan(attrdesc.attrOffset, attrdesc.attrLen, type,
                              attrValue, op);
      if (status != OK)
        return status;
      break;
    }
    }
  }

  RID rid;
  while (scan.scanNext(rid) == OK) {
    status = scan.deleteRecord();
    if (status != OK)
      return status;
  }

  return OK;
}
