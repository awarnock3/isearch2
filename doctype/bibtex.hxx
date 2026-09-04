// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        bibtex.hxx
Version:     $Revision: 1.2 $
Description: class BIBTEX - index BibTEX files
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef BIBTEX_HXX
#define BIBTEX_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// A BibTeX DOCTYPE: splits a file into "}"-terminated entries and
// extracts each entry's "title = "..."" value as its sole indexed
// field. See doctype/bibtex.cxx for the exact grammar and known
// limitations (no escaped-quote handling).
class BIBTEX : public DOCTYPE {
public:
   BIBTEX(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(PRECORD NewRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~BIBTEX();
};

typedef BIBTEX* PBIBTEX;

#endif
