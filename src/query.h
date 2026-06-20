#ifndef QUERY_H
#define QUERY_H

#include <string>
#include "heapfile.h"

enum JoinType {NLJoin, SMJoin, HashJoin};

//
// Path stem for perf "measure" log files, set by minirel from its optional
// command-line prefix (see minirel.C). The measurement code builds a concrete
// path with, e.g., logPath("_measure.csv") -> "<prefix>_measure.csv".
//
extern std::string LogPrefix;

inline std::string logPath(const std::string & suffix)
{
  return LogPrefix + suffix;
}

//
// Prototypes for query layer functions
//


const Status QU_Select(const string & result, 
		       const int projCnt, 
		       const attrInfo projNames[],
		       const attrInfo *attr, 
		       const Operator op, 
		       const char *attrValue);

const Status QU_Join(const string & result, 
		     const int projCnt, 
		     const attrInfo projNames[],
		     const attrInfo *attr1, 
		     const Operator op, 
		     const attrInfo *attr2);

const Status QU_Insert(const string & relation, 
		       const int attrCnt, 
		       const attrInfo attrList[]);

const Status QU_Delete(const string & relation, 
		       const string & attrName, 
		       const Operator op,
		       const Datatype type, 
		       const char *attrValue);

#endif
