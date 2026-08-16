// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        filename.hxx
Version:     1
Description: class FILENAME - indexes a file using  filename
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef FILENAME_HXX
#define FILENAME_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// Indexes a file by its *filename* rather than its content: ParseRecords()
// writes the filename text out to a sibling "<name>.fn" file and indexes
// that instead (Iindex can only index text it can get a file pointer to),
// so a search matches records whose filename contains the query term.
// Present() with ElementSet "B" returns that indexed filename text;
// anything else strips the ".fn" suffix and returns the original file's
// real contents.
class FILENAME : public DOCTYPE {
public:
   FILENAME(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~FILENAME();
};

typedef FILENAME* PFILENAME;

#endif
