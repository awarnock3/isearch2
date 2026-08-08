// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        oneline.cxx
Version:     1
Description: class ONELINE - index documents one line long (like phonebooks)
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "isearch.hxx"
#include "oneline.hxx"

ONELINE::ONELINE(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Splits FileRecord's underlying file into one RECORD per
// newline-terminated line (RecordEnd lands on the newline itself, so
// a line's record includes its trailing '\n'), and adds each to Db via
// DocTypeAddRecord(). A file that doesn't end in '\n' still gets its
// final, unterminated line indexed.
void ONELINE::ParseRecords(const RECORD& FileRecord) {

  GPTYPE Start = 0;
  GPTYPE Position = 0;
  GPTYPE Pos = 0;

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      cout << "Could not access '" << Fn << "'\n";
      return;			// File not accessed

    }



  RECORD Record;
  STRING s;
  FileRecord.GetPathName(&s);
  Record.SetPathName( s );

  FileRecord.GetFileName(&s);
  Record.SetFileName( s );

  FileRecord.GetDocumentType(&s);
  Record.SetDocumentType ( s );
  
  int ci = 0;
  while (ci != EOF) {
    for (; (ci != '\n') && (ci != EOF); ci=fgetc(Fp), Position = Position + 1) ;
    // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeonelinecxx): the for-loop's
    // increment clause runs fgetc()+Position++ together, so the read
    // that finally returns EOF still bumps Position once even though
    // it consumed no real byte -- Position ends up one past the true
    // count of bytes read whenever the scan ends via EOF rather than a
    // real '\n'. Left uncorrected, this GPTYPE (unsigned UINT4,
    // src/defs.hxx) over-count made `Start != Position` wrongly true
    // for an empty file (Position went from 0 to a phantom 1), adding
    // a bogus record whose RecordEnd = Position - 2 underflowed to
    // UINT_MAX -- and, more commonly, made it wrongly true again for
    // *any* file whose very last byte is a newline (Position ends up
    // one past the real EOF-adjacent line-start, not equal to it),
    // adding a spurious trailing record with RecordEnd < RecordStart.
    // Undoing the phantom increment here, before it can corrupt either
    // check below, lets both work correctly with no special-casing.
    if (ci == EOF)
      Position = Position - 1;
    if (Start != Position) {
      Record.SetRecordStart(Start);
      Pos = Position-1;
      Record.SetRecordEnd(Pos);
      Db->DocTypeAddRecord(Record);
      }
    Start=Position;
    if (ci=='\n') ci=0;// save an EOF, but hide a newline so it will loop again
    }
fclose(Fp);

}

ONELINE::~ONELINE() {
}
