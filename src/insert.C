#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "page.h"
#include "query.h"
#include <cstring>
#include <map>


#define CHKRET(statement, status) if((status = statement) != OK) return status; 



/*
 * Inserts a record into the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */
const Status QU_Insert(const string & relation, 
	const int attrCnt, 
	const attrInfo attrList[])
{
    AttrDesc* relAttrDesc;
    int relAttrCount;
    Status status;

    // Get's the relation's information
    CHKRET(attrCat->getRelInfo(relation, relAttrCount,relAttrDesc), status);
    
    // If the attr # != # expected in rel, return bad status
    if (attrCnt != relAttrCount) {
        return  BADCATPARM;
    }
   
    // Calculates the total amount of buffer needed
    int len = 0;

    // Stores the relative location where the attribute should be stored
    // Stores <name,offset>
    map<string, AttrDesc*> infoMap;

    // Initilaizes the length of the buffer and the attribute map
    for (int i=0; i< attrCnt; i++) {
        len += relAttrDesc[i].attrLen;

        // Insert the "name" -> pointer to description
        infoMap[relAttrDesc[i].attrName] = &relAttrDesc[i];
    }

    // The record and the data fields for that record
    Record rec;
    char data[len];

    // Reorders the attributes if not in the correct order
    // and inserts in appropriate location
    for (int i=0; i < attrCnt; i++) {
        
        // If there is no value associated with the var
        if (!attrList[i].attrValue) return BADCATPARM;

        // Returns the iterator associated with this name
        map<string, AttrDesc*>::iterator it = 
            infoMap.find(attrList[i].attrName);
        
        // If the key doesn't exist
        if (it == infoMap.end()) return BADCATPARM;

        AttrDesc *infoPtr = it->second;
       
        // Switch the type and convert it accordingly
        // if it is an integer or float and save it back
        // to the value
        switch (infoPtr->attrType) {
            case INTEGER: {
                        int x = atoi((char *)attrList[i].attrValue);
                        memcpy(attrList[i].attrValue, &x, sizeof(int));
                        break;
                    }
            case FLOAT: {
                        float y = atof((char *)attrList[i].attrValue);
                        memcpy(attrList[i].attrValue, &y, sizeof(float));
                        break;
                    }
        }
        
        // Copy over the information to the data tuple
        memcpy(data + infoPtr->attrOffset, attrList[i].attrValue, infoPtr->attrLen);
    }

    // Instantiate the record with all this data
    rec.data = data;
    rec.length = len;

    // Open a InsertFileScan
    InsertFileScan insertFS(relation, status);
    if (status != OK) return status;

    // Try to insert the record to the heapfile
    RID out;
    CHKRET(insertFS.insertRecord(rec, out),status);

    // If succesful, return OK
    return OK;
}

