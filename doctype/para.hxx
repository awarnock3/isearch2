// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        para.hxx
Version:     1
Description: class PARA - index paragraphs
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef PARA_HXX
#define PARA_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// A DOCTYPE that indexes one record per paragraph, splitting on a
// blank line ("\n\n") -- consecutive newlines beyond the first pair
// are burned through together, so a run of several blank lines still
// counts as a single paragraph boundary. ParseFields()/Present() are
// inherited unchanged from DOCTYPE; only ParseRecords() is overridden.
class PARA : public DOCTYPE {
public:
   PARA(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);
//   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
//                PSTRING StringBuffer);
   ~PARA();
};

typedef PARA* PPARA;

#endif
