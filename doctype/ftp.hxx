// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        ftp.hxx
Version:     1
Description: class FTP - first line is headline, rest is real body
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef FTP_HXX
#define FTP_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// Splits a record on its first '\n': ElementSet "B" returns just the
// first line (the "headline"); "F" returns everything after it (the
// "body"). Record splitting/field parsing are inherited unchanged
// from DOCTYPE -- only Present() is customized.
class FTP : public DOCTYPE {
public:
   FTP(PIDBOBJ DbParent);
   //void ParseRecords(const RECORD& FileRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~FTP();
};

typedef FTP* PFTP;

#endif
