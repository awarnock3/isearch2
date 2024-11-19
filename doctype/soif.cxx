// $Id: soif.cxx,v 1.2 2000/09/06 21:33:31 cnidr Exp $
/*
File:        soif.cxx
Version:     1
$Revision: 1.2 $
Description: Harvest SOIF records (derived from bibtex.cxx by Erik Scott)
Author:      Peter Valkenburg
*/

#include <iostream>
#include <ctype.h>
#include "isearch.hxx"
#include "soif.hxx"


SOIF::SOIF(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}


void 
SOIF::ParseRecords(const RECORD& FileRecord) {

  GPTYPE Start = 0;
  GPTYPE i = 0;
  int    lastBrace = 0;		// this is an int because I need signed.
  CHR   *RecBuffer;
  GPTYPE RecStart, RecEnd, RecLength;
  GPTYPE ActualLength=0;

  STRING fn;
  FileRecord.GetFullFileName (&fn);
  FILE *fp = fopen (fn, "rb");
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
    cout << "SOIF::ParseRecords(): Seek failed - ";
    cout << fn << "\n";
    fclose(fp);
    return;	
  }

  RecStart = 0;
  RecEnd = ftell(fp);
  if(RecEnd == 0) {
    cout << "SOIF::ParseRecords(): Skipping ";
    cout << " zero-length record -" << fn << "...\n";
    fclose(fp);
    return;
  }


  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "SOIF::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }

  RecLength = RecEnd - RecStart;

  RecBuffer = new CHR[RecLength + 2];
  if(!RecBuffer) {
    cout << "SOIF::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "SOIF::ParseRecords(): Failed to fread\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "SOIF::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  RecBuffer[ActualLength]='\0'; // NULL-terminate the buffer for strfns

  Record.SetRecordStart(Start);
  Record.SetRecordEnd(ActualLength);
  Db->DocTypeAddRecord(Record);

}


//
//
// The new goal:  scan the record looking for (title = ") and (") pairs
// and mark them as a field named "title".
//
//
void 
SOIF::ParseFields(PRECORD NewRecord) {
  FILE 	*fp;
  STRING 	fn;
  GPTYPE 	RecStart;
  GPTYPE        RecEnd;
  GPTYPE        RecLength;
  GPTYPE        ActualLength;
  CHR 	*RecBuffer;
  CHR 	*file;


  // Open the file
  NewRecord->GetFullFileName(&fn);
  file = fn.NewCString();
  fp = fopen(fn, "rb");
  if (!fp) {
    cout << "SOIF::ParseRecords(): Failed to open file\n\t";
    perror(file);
    return;
  }

  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();

  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      cout << "SOIF::ParseRecords(): Seek failed - ";
      cout << fn << "\n";
      fclose(fp);
      return;	
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      cout << "SOIF::ParseRecords(): Skipping ";
      cout << " zero-length record -" << fn << "...\n";
      fclose(fp);
      return;
    }
    //RecEnd -= 1;
  }

  // Make two copies of the record in memory
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "SOIF::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
  RecLength = RecEnd - RecStart;

  RecBuffer = new CHR[RecLength + 1];
  if(!RecBuffer) {
    cout << "SOIF::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "SOIF::ParseRecords(): Failed to fread\n\t";
    perror(file);
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "SOIF::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }
  RecBuffer[RecLength]='\0';


  // Parse the record and add fields to record structure
  STRING FieldName;
  FC   fc;
  FCT *pfct;
  DF   df;
  DFT *pdft;
  CHR *p, *q;
  INT  val_start;
  INT  val_end=0;
  INT  val_len;
  DFD  dfd;

  pdft = new DFT();
  if(!pdft) {
    cout << "SOIF::ParseRecords(): Failed to allocate DFT - ";
    cout << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  p = RecBuffer;
  while (*p != '\0') {
    CHR name[128], c;

    FieldName = NULL;
    if (strncmp(p, "@FILE { ", 8) == 0) {
      if ( (q = strchr(p, '\n')) == NULL) {
        cout << "SOIF::ParseRecords(): Badly started record - ";
        cout << fn << "\n";
        delete [] RecBuffer;
        return;
      }
      val_start = (p - RecBuffer) + 8;
      val_end = q - RecBuffer;
      val_len = val_end - val_start;
      strcpy (name, "url");
    }
    else if (sscanf(p, "%127[^{:\n \t]{%u}:%c", name, &val_len, &c) == 3 &&
	     c == '\t' && strchr(p, '\t') + val_len < RecBuffer + RecLength) {
      name[127] = '\0';
      val_start = strchr(p, '\t') - RecBuffer + 1;
      val_end = val_start + val_len;
    }
    else {
      if (strcmp (p, "}\n") != 0) {
        cout << "SOIF::ParseRecords(): Badly ended record - ";
        cout << fn << "\n";
        delete [] RecBuffer;
        return;
      }
      break;
    }

    // We have a attr/val pair
#define UnifiedName(tag) (tag) /* for now */
    CHR *unified_name = UnifiedName(name);
    FieldName = unified_name ? unified_name: "Misc";
    dfd.SetFieldName(FieldName);
    Db->DfdtAddEntry(dfd);
    fc.SetFieldStart(val_start);
    fc.SetFieldEnd(val_end);
    pfct = new FCT();
    pfct->AddEntry(fc);
    df.SetFct(*pfct);
    df.SetFieldName(FieldName);
    pdft->AddEntry(df);
    delete pfct;

    if (RecBuffer[val_end] != '\n') {
      cout << "SOIF::ParseRecords(): Badly formatted record (missing nl) - ";
      cout << fn << "\n";
      delete [] RecBuffer;
      return;
    }

    p = RecBuffer + val_end + 1;

    // cout << "Added attr[" << FieldName << "]/val[" << val_start << "," << val_end << "]\n";

  }

  NewRecord->SetDft(*pdft);
  delete pdft;
  delete [] RecBuffer;

}


void 
SOIF::Present(const RESULT& ResultRecord, const STRING& ElementSet,
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


SOIF::~SOIF() {
}
