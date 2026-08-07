/* $Id: dfdt.cxx,v 1.15 2000/02/04 23:19:58 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1995. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of noteworthy
uses of this software. 

3. The names of MCNC and Clearinghouse for Networked Information Discovery
and Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

/*@@@
File:		dfdt.cxx
Version:	1.02
$Revision: 1.15 $
Description:	Class DFDT - Data Field Definitions Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "dfdt.hxx"

DFDT::DFDT() {
  Initialize();
}


// BUGFIX #1: see docs/BUG_CATALOG.md. Deep-copies Table instead of
// sharing the source's pointer.
DFDT::DFDT(const DFDT& OtherDfdt) {
  INT x;
  Table = new DFD[OtherDfdt.MaxEntries];
  for (x=0; x<OtherDfdt.TotalEntries; x++) {
    Table[x] = OtherDfdt.Table[x];
  }
  TotalEntries = OtherDfdt.TotalEntries;
  MaxEntries = OtherDfdt.MaxEntries;
  Changed = OtherDfdt.Changed;
}


void
DFDT::Initialize() {
  Table = new DFD[500];
  TotalEntries = 0;
  MaxEntries = 500;
  Changed = 0;
}


DFDT&
DFDT::operator=(const DFDT& OtherDfdt) {
  // BUGFIX #2: no self-assignment guard -- delete [] Table; Initialize();
  // ran before OtherDfdt.GetTotalEntries() was read, so `x = x;` saw an
  // already-emptied table and copied nothing back, silently wiping it.
  // Same shape of bug already found and fixed in ATTRLIST's/STRLIST's
  // operator='s. See docs/BUG_CATALOG.md.
  if (this == &OtherDfdt) {
    return *this;
  }
  if (Table) {
    delete [] Table;
  }
  Initialize();
  INT y = OtherDfdt.GetTotalEntries();
  INT x;
  DFD dfd;
  for (x=1; x<=y; x++) {
    OtherDfdt.GetEntry(x, &dfd);
    AddEntry(dfd);
  }
  return *this;
}


void 
DFDT::LoadTable(const STRING& FileName) {
  PCHR b;
  INT4 RecStart,RecEnd,ActualLength,len;

  if (Table) {
    delete [] Table;
  }
  Initialize();

  // Make sure we actually have a valid file name
  if (FileName.GetLength() == 0)
    return;

  // This has to be opened as a text file or NT gets confused
#if defined(_MSDOS) || defined(_WIN32)
  PFILE fp = fopen(FileName, "r");
#else
  PFILE fp = fopen(FileName, "rb");
#endif

  // Bring the entire file into memory
  if (!fp) {
    return;
  }
  //  fseek(fp, 0, 2);
  fseek(fp, 0L, SEEK_END);
  RecStart = 0;
  RecEnd = ftell(fp);

  //  fseek(fp, RecStart, 0);
  fseek(fp, (long)RecStart, SEEK_SET);
  len = RecEnd - RecStart;
  b = new CHR[len + 1];
  ActualLength = fread(b, 1, len, fp);
  b[ActualLength] = '\0';
  fclose(fp);

  INT x,y;
  STRING s;
  INT DfdCount, AttrCount;
  DFD dfd;
  ATTR attr;
  PATTRLIST AttrList;
  PCHR pBuf;

  // Get the # of fields from the start of the buffer
  pBuf = strtok(b,"\n");
  DfdCount = atoi(pBuf);

  // Run through the buffer, looking for newlines
  // Get the file number, the # of attribs, the attribute IDs,
  // attribute types and values
  for (x=0; x<DfdCount; x++) {
    AttrList = new ATTRLIST();
    pBuf = strtok(nullptr,"\n");         // Get the file #
    dfd.SetFileNumber(atoi(pBuf));    // Save it
    pBuf = strtok(nullptr,"\n");         // Get the # of attributes
    AttrCount = atoi(pBuf);
    for (y=0;y<AttrCount;y++) {
      pBuf = strtok(nullptr,"\n");
      s = pBuf;
      attr.SetSetId(s);
      pBuf = strtok(nullptr,"\n");
      attr.SetAttrType(atoi(pBuf));
      pBuf = strtok(nullptr,"\n");
      s = pBuf;
      attr.SetAttrValue(s);
      AttrList->AddEntry(attr);
    }
    dfd.SetAttributes(*AttrList);
    delete AttrList;
    FastAddEntry(dfd);
  }
  delete [] b;
  Changed = 0;
}


void 
DFDT::SaveTable(const STRING& FileName) 
{
  if (TotalEntries == 0) {
    StrUnlink(FileName);
    return;
  }
  PFILE fp = fopen(FileName, "w");
  if (!fp) {
    perror(FileName);
    exit(1);
  } else {
    INT x, y, z;
    STRING s;
    PATTRLIST AttrList;
    ATTR Attr;
    fprintf(fp, "%d\n", TotalEntries);
    for (x=0; x<TotalEntries; x++) {
      AttrList = new ATTRLIST();
      //			Table[x].GetFieldName(&s);
      //			s.Print(fp);
      fprintf(fp, "%d\n", Table[x].GetFileNumber());
      Table[x].GetAttributes(AttrList);
      z = AttrList->GetTotalEntries();
      fprintf(fp, "%d\n", z);
      for (y=1; y<=z; y++) {
	AttrList->GetEntry(y, &Attr);
	Attr.GetSetId(&s);
	if (!(s.IsPrint())) {
#if defined (WIN32) || defined (_MSDOS) || defined (_WIN32)
	  // We might need to clean up the text file line endings in Windows
	  s.MakePrintable();
	  s.Trim();
#endif
	  //cout << "Non-ascii SetID found in " << s;
	  //cout << " at x=" << x << ", y=" << y << endl;
	}
	s.Print(fp);
	fprintf(fp, "\n%d\n", Attr.GetAttrType());
	Attr.GetAttrValue(&s);
	if (!(s.IsPrint())) {
#if defined (WIN32) || defined (_MSDOS) || defined (_WIN32)
	  // We might need to clean up the text file line endings in Windows
	  s.MakePrintable();
	  s.Trim();
#endif
	  //cout << "Non-ascii AttrValue found in " << s;
	  //cout << " at x=" << x << ", y=" << y << endl;
	}
	s.Print(fp);
	fprintf(fp, "\n");
      }
      delete AttrList;
    }
    fclose(fp);
  }
  Changed = 0;
}


void 
DFDT::AddEntry(const DFD& DfdRecord) 
{
  Changed = 1;
  DFD Dfd;
  Dfd = DfdRecord;
  INT x;
  STRING f1, f2;
  // linear!
  for (x=0; x<TotalEntries; x++) {
    Table[x].GetFieldName(&f1);
    Dfd.GetFieldName(&f2);
    if (f1.Equals(f2)) {
      Dfd.SetFileNumber(Table[x].GetFileNumber());
      Table[x] = Dfd;
      return;
    }
  }
  
  if (TotalEntries == MaxEntries)
    Expand();
  Dfd.SetFileNumber(GetNewFileNumber());
  Table[TotalEntries] = Dfd;
  TotalEntries = TotalEntries + 1;
}


void 
DFDT::FastAddEntry(const DFD& DfdRecord) 
{
  Changed = 1;
  DFD Dfd;
  Dfd = DfdRecord;
//  INT x;
  STRING f1, f2;
  /* Leave out the loop if we're creating the list for the first time
    for (x=0; x<TotalEntries; x++) {
    Table[x].GetFieldName(&f1);
    Dfd.GetFieldName(&f2);
    if (f1.Equals(f2)) {
    Dfd.SetFileNumber(Table[x].GetFileNumber());
    Table[x] = Dfd;
    return;
    }
    }
  */  
  if (TotalEntries == MaxEntries)
    Expand();
  Dfd.SetFileNumber(GetNewFileNumber());
  Table[TotalEntries] = Dfd;
  TotalEntries = TotalEntries + 1;
}


void 
DFDT::GetEntry(const INT Index, PDFD DfdRecord) const {
  if ( (Index > 0) && (Index <= TotalEntries) ) {
    *DfdRecord = Table[Index-1];
  }
}


void 
DFDT::GetDfdRecord(const STRING& FieldName, PDFD DfdRecord) const {
  INT x = 0;
  STRING s;
  STRING Field;
  Field = FieldName;
  Field.UpperCase();
  while (x < TotalEntries) {
    Table[x].GetFieldName(&s);
    if (s.Equals(Field)) {
      *DfdRecord = Table[x];
      return; // Got the one we're looking for, so return
    }
    x++;
  }
  // BUGFIX #3: FieldName not found. This used to do
  // `DfdRecord=(PDFD)NULL;` here, which assigns to the local copy of
  // the by-value pointer parameter and has no effect the caller can
  // observe -- confirmed dead by checking the sole call site
  // (src/idb.cxx:441), which never checks for a null/sentinel result
  // and simply relies on *DfdRecord being left as whatever the caller
  // passed in. See docs/BUG_CATALOG.md.
}


INT 
DFDT::GetNewFileNumber() const {
  INT x = 1;
  INT y;
  INT Found;
  do {
    y = 0;
    Found = 0;
    while ( (y < TotalEntries) && (!Found) ) {
      if (Table[y].GetFileNumber() == x) {
	Found = 1;
      }
      y++;
    }
    if (!Found) {
      return x;
    }
    x++;
  } while (x < 1000);
  // ER - can't take any more fields
  return 0;
}


void 
DFDT::Expand() {
  Resize(TotalEntries+500);
}


void 
DFDT::CleanUp() {
  Resize(TotalEntries);
}


void 
DFDT::Resize(const INT Entries) {
  PDFD Temp = new DFD[Entries];
  INT RecsToCopy;
  INT x;
  if (Entries >= TotalEntries) {
    RecsToCopy = TotalEntries;
  } else {
    RecsToCopy = Entries;
    TotalEntries = Entries;
  }
  for (x=0; x<RecsToCopy; x++) {
    Temp[x] = Table[x];
  }
  if (Table)
    delete [] Table;
  Table = Temp;
  MaxEntries = Entries;
}


INT 
DFDT::GetTotalEntries() const {
  return TotalEntries;
}


INT 
DFDT::GetChanged() const {
  return Changed;
}


DFDT::~DFDT() {
  if (Table)
    delete [] Table;
}
