/*

File:        emacsinfo.hxx
Version:     1
Description: class EMACSINFO - first line is headline, rest is real body
Author:      Erik Scott, Scott Technologies, Inc.
*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef EMACSINFO_HXX
#define EMACSINFO_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// An Emacs-info-style DOCTYPE: records are separated by "File:" lines
// within a single source file (see ParseRecords() in emacsinfo.cxx),
// and each record's "File:"/"Node:" line values (up to the next
// comma) are indexed as their own fields. Present()'s "B" (brief)
// element set shows just the first line; "F" (full) shows the whole
// record as-is.
class EMACSINFO : public DOCTYPE {
public:
   EMACSINFO(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);   
   void ParseFields(PRECORD NewRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~EMACSINFO();
};

typedef EMACSINFO* PEMACSINFO;

#endif
