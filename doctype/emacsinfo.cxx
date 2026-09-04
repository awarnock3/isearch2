/*

File:        emacsinfo.cxx
Version:     1
Description: class EMACSINFO - index files with "File:" separators
Author:      Erik Scott, Scott Technologies, Inc.
*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <ctype.h>
#include <string.h>  /* For strstr() in ParseRecords */
#include "isearch.hxx"
#include "emacsinfo.hxx"

EMACSINFO::EMACSINFO(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Splits FileRecord's file into one record per "File:"-prefixed
// section (a record runs from one "File:" occurrence up to, but not
// including, the next one, or to the end of the file for the last
// record). Registers each with Db->DocTypeAddRecord(); ParseFields()
// later re-reads each record's own byte range to extract its
// "File:"/"Node:" field values.
void EMACSINFO::ParseRecords(const RECORD& FileRecord) {

  GPTYPE Start = 0;
  GPTYPE End = 0;
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
    cout << "EMACSINFO::ParseRecords(): Seek failed - ";
    cout << fn << "\n";
    fclose(fp);
    return;	
  }
	
  RecStart = 0;
  RecEnd = ftell(fp);
  if(RecEnd == 0) {
    cout << "EMACSINFO::ParseRecords(): Skipping ";
    cout << " zero-length record -" << fn << "...\n";
    fclose(fp);
    return;
  }


  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "EMACSINFO::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
	
  RecLength = RecEnd - RecStart;
	
  RecBuffer = new CHR[RecLength + 2];
  if(!RecBuffer) {
    cout << "EMACSINFO::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "EMACSINFO::ParseRecords(): Failed to fread\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "EMACSINFO::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  RecBuffer[ActualLength]='\0';  // NULL-terminate the buffer for strfns

  // Now we walk through the buffer in RecBuffer and look for "FILE:" strings
  // and mark the record pairs.

  PCHR oldw = RecBuffer;
  PCHR w;
  PCHR bp = RecBuffer;

  while ((w=strstr(bp+1,"File:")) != nullptr) { // while we can still find the next File: marker
    Start = oldw - RecBuffer;
    End   = (w - RecBuffer) - 1;
    Record.SetRecordStart(Start); Record.SetRecordEnd(End);
    Db->DocTypeAddRecord(Record);
    bp = w;
    oldw = w;
  }

  Start = oldw-RecBuffer;
  End = ActualLength - 1;

  Record.SetRecordStart(Start); Record.SetRecordEnd(End);
  Db->DocTypeAddRecord(Record);

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeemacsinfocxx): RecBuffer
  // (the whole file's contents) was never freed on this function's
  // normal return path -- a confirmed leak of the entire file on
  // every successful ParseRecords() call. Same bug as, and fixed the
  // same way as, doctype/bibtex.cxx's BUGFIX #1.
  delete [] RecBuffer;
}


// The goal of this is to find those pesky "File:" and "Node:" fields and mark
// them as searchable.

void EMACSINFO::ParseFields(PRECORD NewRecord) {
  PFILE 	fp;
  STRING 	fn;
  GPTYPE 	RecStart, 
    RecEnd, 
    RecLength, 
    ActualLength;
  PCHR 	RecBuffer;

  // Open the file
  NewRecord->GetFullFileName(&fn);
  fp = fopen(fn, "rb");
  if (!fp) {
    cout << "EMACSINFO::ParseFields(): Failed to open file\n\t";
    // BUGFIX #2 (docs/BUG_CATALOG.md#doctypeemacsinfocxx): this used
    // to call fn.NewCString() into a `file` variable that was never
    // freed on any return path (a leak on every call, since fopen()
    // already uses fn directly via STRING::operator const char*()).
    // perror() can just take fn the same way. Same bug as, and fixed
    // the same way as, doctype/bibtex.cxx's BUGFIX #2.
    perror(fn);
    return;
  }

  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();
	
  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      cout << "EMACSINFO::ParseFields(): Seek failed - ";
      cout << fn << "\n";
      fclose(fp);
      return;	
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      cout << "EMACSINFO::ParseFields(): Skipping ";
      cout << " zero-length record -" << fn << "...\n";
      fclose(fp);
      return;
    }
    //RecEnd -= 1;
  }

  // Make two copies of the record in memory
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "EMACSINFO::ParseFields(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
  RecLength = RecEnd - RecStart;
	
  RecBuffer = new CHR[RecLength + 1];
  if(!RecBuffer) {
    cout << "EMACSINFO::ParseFields(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "EMACSINFO::ParseFields(): Failed to fread\n\t";
    perror(fn);
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "EMACSINFO::ParseFields(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }
  RecBuffer[RecLength]='\0';
	

  // Parse the record and add fields to record structure
  STRING FieldName;
  FC fc;
  PFCT pfct;
  DF df;
  PDFT pdft;
//  PCHR p;
  INT val_start;
  INT val_end;
//  INT val_len;
  DFD dfd;
  CHR *fileStarter, *nodeStarter;

  pdft = new DFT();
  if(!pdft) {
    cout << "EMACSINFO::ParseFields(): Failed to allocate DFT - ";
    cout << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  fileStarter = strstr(RecBuffer,"File:");
  if (fileStarter != nullptr) {
    val_start = (fileStarter-RecBuffer)+5;
    for (val_end = val_start; (RecBuffer[val_end]!=',')
	   && (val_end < (INT)ActualLength); val_end++);
    // We have a tag pair
    FieldName = "file";
    dfd.SetFieldName(FieldName);
    Db->DfdtAddEntry(dfd);
    fc.SetFieldStart(val_start);
    // BUGFIX #3 (docs/BUG_CATALOG.md#doctypeemacsinfocxx): val_end
    // stops *at* the comma delimiter (or ActualLength if none is
    // found), not at the value's own last character; SetFieldEnd()
    // takes an inclusive end index, so passing val_end directly
    // included the trailing comma (or, with no comma, was one past
    // the buffer's last real byte) in the stored field. Excluded here.
    fc.SetFieldEnd(val_end - 1);
    pfct = new FCT();
    pfct->AddEntry(fc);
    df.SetFct(*pfct);
    df.SetFieldName(FieldName);
    pdft->AddEntry(df);
    delete pfct;
  }
		

  nodeStarter = strstr(RecBuffer,"Node:");
  if (nodeStarter != nullptr) {
    val_start = (nodeStarter-RecBuffer)+5;
    for (val_end = val_start; (RecBuffer[val_end]!=',')
	   && (val_end < (INT)ActualLength); val_end++);
    // We have a tag pair
    FieldName = "node";
    dfd.SetFieldName(FieldName);
    Db->DfdtAddEntry(dfd);
    fc.SetFieldStart(val_start);
    // BUGFIX #3: same off-by-one as the "File:" field above, same fix.
    fc.SetFieldEnd(val_end - 1);
    pfct = new FCT();
    pfct->AddEntry(fc);
    df.SetFct(*pfct);
    df.SetFieldName(FieldName);
    pdft->AddEntry(df);
    delete pfct;
  }
  NewRecord->SetDft(*pdft);
  delete pdft;
  delete [] RecBuffer;

}




void EMACSINFO::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) {
	
*StringBufferPtr = "";
// Basic strategy: on a "B" present, show just the first line (the
// headline). On an "F" present, show the whole record as-is, headline
// included (this comment used to claim "F" excludes the first line,
// which doesn't match the code below -- the "F" branch is a
// deliberate no-op).

STRING myBuff;
ResultRecord.GetRecordData(&myBuff);
STRINGINDEX firstNL = myBuff.Search('\n');
if (firstNL == 0) {
   // BUGFIX #4 (docs/BUG_CATALOG.md#doctypeemacsinfocxx): wrong class
   // name in this diagnostic (copy-pasted from doctype/ftp.cxx).
   cout << "EMACSINFO::Present() -- Can't find first Newline in file to present.\n";
   return;
   }

if (ElementSet.Equals("F")) {
   // do we want the File: and Node: line on a full present? I think so.
   }
else if (ElementSet.Equals("B")) {
   myBuff.EraseAfter(firstNL);
   }

*StringBufferPtr = myBuff;

} 


EMACSINFO::~EMACSINFO() {
}
