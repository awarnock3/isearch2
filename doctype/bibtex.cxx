// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// $Id: bibtex.cxx,v 1.6 1998/05/12 16:48:27 cnidr Exp $
/*

File:        bibtex.cxx
Version:     1
$Revision $
Description: class BIBTEX - index documents by paragraphs
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <iostream>
#include <ctype.h>
#include "isearch.hxx"
#include "bibtex.hxx"

BIBTEX::BIBTEX(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Splits FileRecord's file into one record per "}"-terminated BibTeX
// entry (the last record's end is extended to the last byte of the
// file, to absorb any trailing whitespace/comments after its closing
// "}"). Registers each with Db->DocTypeAddRecord(); ParseFields()
// later re-reads each record's own byte range to extract its title.
void BIBTEX::ParseRecords(const RECORD& FileRecord) {

  GPTYPE Start = 0;
  GPTYPE i = 0;
  int    lastBrace = 0;		// this is an int because I need signed.
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
    cout << "BIBTEX::ParseRecords(): Seek failed - ";
    cout << fn << "\n";
    fclose(fp);
    return;	
  }
  
  RecStart = 0;
  RecEnd = ftell(fp);
  if(RecEnd == 0) {
    cout << "BIBTEX::ParseRecords(): Skipping ";
    cout << " zero-length record -" << fn << "...\n";
    fclose(fp);
    return;
  }
  
  
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "BIBTEX::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
  
  RecLength = RecEnd - RecStart;
  
  RecBuffer = new CHR[RecLength + 2];
  if(!RecBuffer) {
    cout << "BIBTEX::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }
  
  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "BIBTEX::ParseRecords(): Failed to fread\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "BIBTEX::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }
  
  RecBuffer[ActualLength]='\0'; // NULL-terminate the buffer for strfns
  
  int w;
  for (w=ActualLength; w>0; w--) 
    if (RecBuffer[w]=='}') {
      lastBrace = w;
      w=-1;				// to break out of loop.
    }
  
  for (i=Start; i<= ActualLength; i++) {
    if (RecBuffer[i]=='}') {	// did we find the end of a record?  Good.
      if (i==(GPTYPE)lastBrace) {	// we're on the very last one.
	i=ActualLength-1;			// so we mark it at the very end, after the whitespace.
      }
      Record.SetRecordStart(Start);
      Record.SetRecordEnd(i);
      Db->DocTypeAddRecord(Record);
      Start = i+1;
    }
  }

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypebibtexcxx): RecBuffer (the
  // whole file's contents) was never freed on this function's normal
  // return path -- a confirmed leak of the entire file on every
  // successful ParseRecords() call.
  delete [] RecBuffer;
}

//
//
// The new goal:  scan the record looking for (title = ") and (") pairs
// and mark them as a field named "title".
//
//

// Reads NewRecord's bytes off disk and adds a single "title" DF field
// (see BUGFIX #4 below) spanning the first quoted string found after
// the literal substring "title" outside of any other quoted string.
void BIBTEX::ParseFields(PRECORD NewRecord) {
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
    cout << "BIBTEX::ParseRecords(): Failed to open file\n\t";
    // BUGFIX #2 (docs/BUG_CATALOG.md#doctypebibtexcxx): this used to
    // call fn.NewCString() into a `file` variable that was never
    // freed on any return path (a leak on every call, since fopen()
    // already uses fn directly via STRING::operator const char*()).
    // perror() can just take fn the same way.
    perror(fn);
    return;
  }
  
  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();
  
  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      cout << "BIBTEX::ParseRecords(): Seek failed - ";
      cout << fn << "\n";
      fclose(fp);
      return;	
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      cout << "BIBTEX::ParseRecords(): Skipping ";
      cout << " zero-length record -" << fn << "...\n";
      fclose(fp);
      return;
    }
    //RecEnd -= 1;
  }
  
  // Make two copies of the record in memory
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "BIBTEX::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
  RecLength = RecEnd - RecStart;
  
  RecBuffer = new CHR[RecLength + 1];
  if(!RecBuffer) {
    cout << "BIBTEX::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }
  
  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "BIBTEX::ParseRecords(): Failed to fread\n\t";
    perror(fn);
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "BIBTEX::ParseRecords(): Failed to fread ";
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
  INT val_end=0;
  //INT val_len;
  DFD dfd;
  
  pdft = new DFT();
  if(!pdft) {
    cout << "BIBTEX::ParseRecords(): Failed to allocate DFT - ";
    cout << fn << "\n";
    delete [] RecBuffer;
    return;
  }
  
  // OK - we need to scan RecBuffer and find the "title" element, then fast-
  // forward to the next '"' character.  That will be the title field val_start.
    // Then we go forward again until we see another '"' character, and that will
      // be the field end.
      // If someone (a) knows how BibTeX represents a literal quotation mark and
      // (b) wants to hack this to do the right thing, be my guest.  I'm a long-
      // time [nt]roff user, myself. :-)
      
      GPTYPE i;  // was `int`; unified with ActualLength's type (both
                 // are always non-negative here) to avoid signed/
                 // unsigned comparison warnings throughout this loop.
  int state;
#define LOOKING 1
#define INQUOTES 2
  
  // Basically, the following God-awful excuse for a state machine will
  // look for title not occuring inside quotation marks.
  
  val_start = 0;
  state = LOOKING;
  for (i=0; i< ActualLength; i++) {
    if (state==LOOKING) {
      if (RecBuffer[i]=='t')
	if (RecBuffer[i+1]=='i')
	  if (RecBuffer[i+2]=='t')
	    if (RecBuffer[i+3]=='l') {
	      if (RecBuffer[i+4]=='e') {
		// look for the quotation mark
		i=i+5;
		for (;(i<ActualLength) && (val_start==0); i++) {
		  if (RecBuffer[i]=='"') {
		    val_start = i;
		  }
		}
		if (i==ActualLength) {
		  cout << "Cannot find quote mark after title.\n";
		  // BUGFIX #3 (docs/BUG_CATALOG.md#doctypebibtexcxx): this
		  // early return leaked both RecBuffer and pdft (the
		  // heap-allocated DFT from a few lines above).
		  delete pdft;
		  delete [] RecBuffer;
		  return;
		}
		for (i=val_start+1; (RecBuffer[i]!='"') && (i<ActualLength); i++);
		if (i==ActualLength) {
		  cout << "couldn't find ending quote.\n";
		  // BUGFIX #3: same leak as above, same fix.
		  delete pdft;
		  delete [] RecBuffer;
		  return;
		}
		else {
		  val_end = i;
		  i=ActualLength + 2;		// a side effect to ensure exiting.
		}
	      }				// end of if we found title
	      else if (RecBuffer[i]=='"') state=INQUOTES;
	    }
    }				// end of looking
    else if (state == INQUOTES) {
      if (RecBuffer[i]=='"') state=LOOKING;
    }
  }				// end of for loop
  
  
  // BUGFIX #4 (docs/BUG_CATALOG.md#doctypebibtexcxx): val_start stays
  // at its 0 initializer if and only if "title" was never found in the
  // record at all (both malformed-title early-return paths above
  // already returned before reaching here, and a legitimately parsed
  // title's quote position is always >= 5). This block used to run
  // unconditionally, adding a bogus zero-length "title" field
  // (FieldStart=0, FieldEnd=0) to every record that has no title at
  // all. Only add the tag pair once we've actually found one.
  if (val_start != 0) {
    // We have a tag pair
    FieldName = "title";
    dfd.SetFieldName(FieldName);
    Db->DfdtAddEntry(dfd);
    // BUGFIX #5 (docs/BUG_CATALOG.md#doctypebibtexcxx): val_start/
    // val_end are the positions of the opening/closing quote
    // characters themselves, not the title text between them -- using
    // them directly made the field's stored text read as
    // `"A Great Title"`, quote marks included, rather than
    // `A Great Title`. Excluded here. (A `title = ""` empty title is
    // a pre-existing, unhandled degenerate case either way -- val_end
    // would equal val_start+1, giving an inverted, not just empty,
    // range; not pursued further since it's vanishingly rare input.)
    fc.SetFieldStart(val_start + 1);
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



// "F" returns the whole record verbatim; anything else returns the
// "title" field's value (empty if the record has none).
void BIBTEX::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		     STRING* StringBufferPtr) {
  *StringBufferPtr = "";
  
  // rationale:  If an F present, dump the whole buffer.
  // Otherwise, if there are fields, then on any othjer present
  // return the "title" field value.
  
  if (ElementSet.Equals("F")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;
  }
  if (Db->DfdtGetTotalEntries() == 0) {
    return;
  }
  STRING FieldName;
  FieldName = "title";
  
  Db->GetFieldData(ResultRecord, FieldName, StringBufferPtr);
}


BIBTEX::~BIBTEX() {
}
