// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        gopher.hxx
Version:     1
Description: class GOPHER - present with with gopher-style .cap name files
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef GOPHER_HXX
#define GOPHER_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// ElementSet "F" returns the raw record data; anything else ("B")
// looks for a gopher-style ".cap/<filename>" sidecar file next to the
// record and, if found, returns the value of its "Name=" line;
// otherwise falls back to the record's own filename (or full path, if
// FULLFILENAME is defined). Record splitting/field parsing are
// inherited unchanged from DOCTYPE -- only Present() is customized.
class GOPHER : public DOCTYPE {
public:
   GOPHER(PIDBOBJ DbParent);
   //void ParseRecords(const RECORD& FileRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~GOPHER();
};

typedef GOPHER* PGOPHER;

#endif
