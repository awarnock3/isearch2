// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        ftp.cxx
Version:     1
Description: class FTP - first line is headline, rest is real body
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "isearch.hxx"
#include "ftp.hxx"

FTP::FTP(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

void FTP::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) {
	
*StringBufferPtr = "";
// Basic strategy:  on a "B" present, show the first line.  On an "F" present,
// show everything *but* the first line.  Simple enough, right?

STRING myBuff;
ResultRecord.GetRecordData(&myBuff);
STRINGINDEX firstNL = myBuff.Search('\n');
if (firstNL == 0) {
   cout << "FTP::Present() -- Can't find first Newline in file to present.\n";
   return;
   }

if (ElementSet.Equals("F")) {
   myBuff.EraseBefore(firstNL+1);
   }
else if (ElementSet.Equals("B")) {
   // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeftpcxx): STRING::Search()
   // is 1-based and firstNL is the '\n' character's own position, so
   // EraseAfter(firstNL) (which keeps `firstNL` characters, inclusive)
   // kept the newline itself as part of the "headline" -- confirmed
   // via a real before/after regression test. EraseAfter(firstNL - 1)
   // keeps everything up to but not including the newline instead.
   myBuff.EraseAfter(firstNL-1);
   }

*StringBufferPtr = myBuff;


}


FTP::~FTP() {
}
