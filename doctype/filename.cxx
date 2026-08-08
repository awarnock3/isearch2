// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        filename.cxx
Version:     1
Description: class FILENAME - index files based on their filename
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "isearch.hxx"
#include "filename.hxx"

FILENAME::FILENAME(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Iindex can only index text it can get a file pointer to, so this
// writes the record's own filename out to a sibling "<name>.fn" file
// and indexes *that* file instead of the original -- a search then
// matches records by filename rather than content. Removed here:
// unused `GPTYPE Start/Position/Pos;` locals and a `static int
// gdb_tester;`, none of which were ever read or written anywhere in
// this function -- surfaced by -Wunused-variable while bringing this
// file to a clean -Wall -Wextra build for the first time.
void FILENAME::ParseRecords(const RECORD& FileRecord) {

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);

  // Now we know the filename.  Make a "filename.fn" filename, and write
  // the file name into that file?  Make sense?  Of course not.  Here's
  // the box score:
  //
  // Given a file called "bubba" that contains the string "howdy, slim."
  // create a file named bubba.fn that contains the string "bubba".
  // On a search, we'll look for "bubba" in bubba.fn.
  //
  // We have to do this the silly way because Iindex can only index
  // text that it can get a file pointer to.  Text in RAM-only cannot
  // be indexed.  This also means that derived text cannot be indexed.

  STRING newfilename = Fn ;
  newfilename.Cat(".fn");


  // Now that we have the new filename, let's open it for writing, stick the old
  // file name in there, and close up that file.

  FILE *fnfp = fopen(newfilename,"wb");
  if (fnfp == nullptr) {
    cout << "Cannot write file " << newfilename << ", bailing out.\n";
    return; // leaking all the way...
  }

  Fn.Print(fnfp);
  fclose(fnfp);

  // Now, we need to use the SetFileName member function, to, eh, *adjust*
  // (yeah, yeah, that's what we'll call it) the file name.
  // Note that we'll use GetFileName to get the non-path part of the name,
  // append to it, and then save the appended name.  It's just mildly
  // easier that way.

  RECORD Record;
  STRING s;
//  GPTYPE i;
  FileRecord.GetPathName(&s);
  Record.SetPathName( s );

  FileRecord.GetFileName(&s);
  s.Cat(".fn");
  Record.SetFileName( s );

  FileRecord.GetDocumentType(&s);
  Record.SetDocumentType ( s );

  Record.GetFullFileName(&s);

  Record.SetRecordStart(FileRecord.GetRecordStart());
  Record.SetRecordEnd(FileRecord.GetRecordEnd());


  // Now we can finally do the whatever to tell this thing to use the
  // modified record.

  Db->DocTypeAddRecord(Record);

  // That *should* be all we need.
  // Now we need a mutant Present().


}


// ElementSet "B" returns the record's raw indexed data (the filename
// text written to the "<name>.fn" sibling file by ParseRecords()
// above); anything else strips the ".fn" suffix back off and returns
// the *original* file's contents instead.
void FILENAME::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		       STRING* StringBufferPtr) {
  *StringBufferPtr = "";

  // First we see if the element set is "B", meaning they just want a file
  // name.  We're going to return the full file name for now.

  if (ElementSet.Equals("B")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;
  }


  // Now we add code to see if the element set was "F", and if it was
  // then we return the contents of the original file instead of the
  // filename.  This is of course never called if an above condition
  // was satisfied.

  // First, let's get the (eh, "amended") filename...

  // Documented, not changed: if ".fn" is somehow absent, SearchReverse()
  // returns 0 and `dotFN - 1` underflows (STRINGINDEX is size_t) to
  // SIZE_MAX; EraseAfter() bounds-checks its argument against Length
  // and no-ops rather than crashing, so this degrades to leaving
  // hackedFN unmodified instead of corrupting it. Not reachable in
  // practice: every RESULT of this DOCTYPE was indexed via
  // ParseRecords() above, which always appends ".fn" before calling
  // Db->DocTypeAddRecord().
  STRING hackedFN;
  ResultRecord.GetFullFileName(&hackedFN);
  STRINGINDEX dotFN = hackedFN.SearchReverse(".fn");
  hackedFN.EraseAfter(dotFN - 1);

  // Brace yourself for the most overkill "read-a-file-and-malloc-a-buffer"
  // in history.  I really, really should use mmap(), but alas, there are still
  // HPUX 9.X machines without the unsupported kernal hack...

  PFILE fp = fopen (hackedFN, "rb");
  if (!fp)
    {
      cout << "Could not access '" << hackedFN << "'\n";
      return;			// File not accessed

    }

  

  if(fseek(fp, 0L, SEEK_END) == -1) {
    cout << "FILENAME::Present(): Seek failed (I) - ";
    cout << hackedFN << "\n";
    fclose(fp);
    return;	
  }
	
  GPTYPE RecStart = 0;
  GPTYPE RecEnd = ftell(fp);
  if(RecEnd == 0) {
    cout << "FILENAME::Present(): Skipping ";
    cout << " zero-length record -" << hackedFN << "...\n";
    fclose(fp);
    return;
  }


  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "FILENAME::Present(): Seek failed (II) - " << hackedFN << "\n";
    fclose(fp);
    return;	
  }
	
  GPTYPE RecLength = RecEnd - RecStart;
	
  PCHR RecBuffer = new CHR[RecLength + 2];
  if(!RecBuffer) {
    cout << "FILENAME::Present(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << hackedFN << "\n";
    fclose(fp);
    return;
  }

  GPTYPE ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "FILENAME::Present(): Failed to fread\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "FILENAME::Present(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << hackedFN << "\n";
    delete [] RecBuffer;
    return;
  }

  RecBuffer[ActualLength]='\0';  // NULL-terminate the buffer for strfns

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypefilenamecxx): RecBuffer was
  // never freed after this assignment. STRING::operator=(const CHR*)
  // copies the bytes into the STRING's own internal buffer (it doesn't
  // take ownership of RecBuffer), so every successful "F"-element-set
  // call leaked the whole file buffer -- same bug shape as
  // doctype/bibtex.cxx's BUGFIX #1 and doctype/emacsinfo.cxx's
  // BUGFIX #1.
  *StringBufferPtr = RecBuffer;
  delete [] RecBuffer;

}


FILENAME::~FILENAME() {
}
