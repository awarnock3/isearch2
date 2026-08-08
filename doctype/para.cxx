// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        para.cxx
Version:     1
Description: class PARA - index documents by paragraphs
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "isearch.hxx"
#include "para.hxx"

PARA::PARA(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Splits FileRecord's underlying file into one RECORD per paragraph,
// delimited by a blank line ("\n\n"; a longer run of consecutive
// newlines is burned through together as a single boundary, and
// RecordEnd lands on that run's last newline, so a paragraph's record
// includes its trailing separator, not just its own text -- same
// convention as doctype/oneline.cxx's documented "RecordEnd lands on
// the newline itself"), and adds each to Db via DocTypeAddRecord(). The
// final paragraph (with no trailing blank line after it) is flushed
// after the main scan.
void PARA::ParseRecords(const RECORD& FileRecord) {


  GPTYPE Start = 0;
  GPTYPE i = 0;
  PCHR   RecBuffer;
  GPTYPE RecStart, RecEnd, RecLength;
  GPTYPE ActualLength=0;
  
  STRING fn;
  FileRecord.GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp)
    {
      cout << "Could not access '" << fn << "'\n";
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
  
  

  if(fseek(fp, 0L, SEEK_END) == -1) {
    cout << "PARA::ParseRecords(): Seek failed - ";
    cout << fn << "\n";
    fclose(fp);
    return;	
  }
	
  RecStart = 0;
  RecEnd = ftell(fp);
  if(RecEnd == 0) {
    cout << "PARA::ParseRecords(): Skipping ";
    cout << " zero-length record -" << fn << "...\n";
    fclose(fp);
    return;
  }


  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "PARA::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
	
  RecLength = RecEnd - RecStart;
	
  RecBuffer = new CHR[RecLength + 2];
  if(!RecBuffer) {
    cout << "PARA::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "PARA::ParseRecords(): Failed to fread\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "PARA::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  RecBuffer[ActualLength]='\0';  // NULL-terminate the buffer for strfns

  // Now we can loop, scan for "\n\n", and use that to mark beginnings and
  // endings.


  Start = 0; GPTYPE j=0;
  for (i=0; i< ActualLength-1; i++) {
    if ( (RecBuffer[i]=='\n') && (RecBuffer[i+1]=='\n') ) {
      // We found a para marker, didn't we?
      // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeparacxx): this used to be
      // `(i-1) > Start`; i and Start are both GPTYPE (unsigned UINT4,
      // src/defs.hxx), so whenever a "\n\n" marker was found at the
      // very start of the file (i == 0, e.g. a file beginning with a
      // blank line), `i - 1` underflowed to UINT_MAX, which is always
      // greater than Start -- wrongly satisfying a check whose whole
      // purpose is to *reject* a degenerate paragraph this close to
      // Start. This added a bogus, empty "paragraph" record spanning
      // just the two leading newline characters. Confirmed via a
      // before/after test-revert. Rewritten as `i > Start + 1`, an
      // exactly equivalent comparison for every i that doesn't
      // underflow, since it never subtracts from the unsigned i at all.
      if ( i > Start + 1) {
	// Now we need to burn "\n"s until we get to the start of the
	// new para.
	for (j=i; (j < ActualLength) && (RecBuffer[j]=='\n'); j++);
	Record.SetRecordStart(Start);
	Record.SetRecordEnd(j-1);
	Db->DocTypeAddRecord(Record);
	Start = i = j;
      }

    }
  }

  // Add the last record entry now

  if (Start != ActualLength) {
    Record.SetRecordStart(Start);
    Record.SetRecordEnd(ActualLength-1);

    Db->DocTypeAddRecord(Record);
  }

  // BUGFIX #2 (docs/BUG_CATALOG.md#doctypeparacxx): RecBuffer was freed
  // on the early-error paths above (failed/short fread()) but never
  // here, on the normal success path -- a straightforward memory leak
  // on every file this function successfully processes. Confirmed via
  // ASan (`make tests-asan` caught real leaks from this exact
  // function, one per test that reached this point).
  delete [] RecBuffer;
}

PARA::~PARA() {
}
