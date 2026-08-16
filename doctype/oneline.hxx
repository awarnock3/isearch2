// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        oneline.hxx
Version:     1
Description: class ONELINE - documents one per line
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef ONELINE_HXX
#define ONELINE_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// A DOCTYPE that indexes one record per newline-terminated line (e.g.
// a phonebook-style file, one entry per line). ParseFields()/Present()
// are inherited unchanged from DOCTYPE; only ParseRecords() is
// overridden.
class ONELINE : public DOCTYPE {
public:
   ONELINE(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);
//   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
//                PSTRING StringBuffer);
   ~ONELINE();
};

typedef ONELINE* PONELINE;

#endif
