/* $Id: idb.cxx,v 1.32 2001/03/26 20:55:00 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

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
File:		idb.cxx
Version:	1.02
$Revision: 1.32 $
Description:	Class IDB
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifdef UNIX
#include <stdlib.h>
#endif

#include "idb.hxx"

static FILE* GlobalFcFp;
static GDT_BOOLEAN GlobalWrongEndian;


IDB::IDB(const STRING& NewPathName, const STRING& NewFileName) 
{
	STRLIST EmptyList;
	Initialize(NewPathName, NewFileName, EmptyList);
}


IDB::IDB(const STRING& NewPathName, const STRING& NewFileName, 
	 const STRLIST& NewDocTypeOptions) 
{
  Initialize(NewPathName, NewFileName, NewDocTypeOptions);
}


void 
IDB::Initialize(const STRING& NewPathName, const STRING& NewFileName,
		const STRLIST& NewDocTypeOptions) {
  DebugMode = 0;
  DebugSkip = 0;
  TotalRecordsQueued = 0;
  DbPathName = NewPathName;
  AddTrailingSlash(&DbPathName);
  ExpandFileSpec(&DbPathName);
  DbFileName = NewFileName;
  RemovePath(&DbFileName);

  // Load DbInfo file
  STRING DbInfoFn;
  ComposeDbFn(&DbInfoFn, DbExtDbInfo);
  MainRegistry = new REGISTRY("Isearch");
  DbInfoChanged = GDT_FALSE;

  STRLIST Position;
  MainRegistry->LoadFromFile(DbInfoFn, Position);
  SetWrongEndian();

  // Create INDEX
  STRING IndexFN;
  ComposeDbFn(&IndexFN, DbExtIndex);
  MainIndex = new INDEX(this, IndexFN);

  // Create and load MDT
  STRING MDTFN;
  ComposeDbFn(&MDTFN, DbExtMdt);
  STRING FileStem;
  GetDbFileStem(&FileStem);
  MainMdt = new MDT(FileStem, IsWrongEndian());
  //	MainMdt->LoadTable(MDTFN);
  //	if (IsWrongEndian()) {
  //		MainMdt->FlipBytes();
  //	}

  STRING DFDTFN;
  ComposeDbFn(&DFDTFN, DbExtDfd);
  MainDfdt = new DFDT();
  MainDfdt->LoadTable(DFDTFN);

  // Now, load up the field type STRLIST
  INT j, i, n;
  DFD DfdRecord;
  ATTRLIST AttrList;
  STRING FieldType,FieldName,S;
  n=MainDfdt->GetTotalEntries();

  for (i=1;i<=n;i++) {
    MainDfdt->GetEntry(i,&DfdRecord);
    DfdRecord.GetAttributes(&AttrList);
    AttrList.AttrGetFieldType(&FieldType);
    AttrList.AttrGetFieldName(&FieldName);
    S = FieldName;
    S.Cat("=");
    S.Cat(FieldType);
//    Parent->FieldTypes.AddEntry(S);
    FieldTypes.AddEntry(S);
  }

#if defined(_MSDOS) && !defined(_WIN32)
  UINT4 DefaultMemSize = 16;
#else
  UINT4 DefaultMemSize = 1024;
#endif
  DefaultMemSize *= 1024;
  IndexingMemory = DefaultMemSize;

  STRLIST Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("DocTypeOptions");
  MainRegistry->GetData(Position, &Value);
  DocTypeOptions = Value;

  n = NewDocTypeOptions.GetTotalEntries();
  if (n > 0) {
    for (i=1;i<=n;i++) {
      NewDocTypeOptions.GetEntry(i,&S);
      DocTypeOptions.AddEntry(S);
    }
  }

  DocTypeReg = new DTREG(this);
}


void 
IDB::SetWrongEndian() {
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("BigEndian");
  MainRegistry->GetData(Position, &Value);
  STRING S;
  Value.GetEntry(1, &S);
  if (S.GetLength() > 0)
    WrongEndian = (S.GetInt() != IsBigEndian()) ? GDT_TRUE : GDT_FALSE;
  else
    WrongEndian = IsBigEndian();
}


#ifdef DICTIONARY
void 
IDB::CreateDictionary(void) {
  MainIndex->CreateDictionary();
}


void 
IDB::CreateCentroid(void) {
  MainIndex->CreateCentroid();
}
#endif
 

SIZE_T 
IDB::GpFwrite(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
	      FILE* Stream) const {
  if (IsWrongEndian()) {
    SIZE_T y;
    for (y=0; y<((Size*NumElements)/sizeof(GPTYPE)); y++) {
      GpSwab(Ptr + y);
    }
  }
  SIZE_T val;
  val = fwrite((char*)Ptr, Size, NumElements, Stream);
  if (val < NumElements) {
    fprintf(stderr,"ERROR: Can't Complete Write!\n");
//      strerror(errnum) << endl;
  }
  return val;
}


SIZE_T 
IDB::GpFread(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
	     FILE* Stream) const {
  SIZE_T x = fread((char*)Ptr, Size, NumElements, Stream);
  if ( (x) && (IsWrongEndian()) ) {
    SIZE_T y;
    for (y=0; y<((Size *x)/sizeof(GPTYPE)); y++) {
      GpSwab(Ptr + y);
    }
  }
  return x;
}


GDT_BOOLEAN 
IDB::IsDbCompatible() const {
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("MagicNumber");
  MainRegistry->GetData(Position, &Value);
  STRING S;
  Value.GetEntry(1, &S);
  return (S.GetInt() == IsearchMagicNumber) ? GDT_TRUE : GDT_FALSE;
}


void 
IDB::KeyLookup(const STRING& Key, RESULT *ResultBuffer) const {
  MDTREC Mdtrec;
  MainMdt->GetMdtRecord(Key, &Mdtrec);
  STRING S;
  Mdtrec.GetKey(&S);
  ResultBuffer->SetKey(S);
  Mdtrec.GetDocumentType(&S);
  ResultBuffer->SetDocumentType(S);
  Mdtrec.GetPathName(&S);
  ResultBuffer->SetPathName(S);
  Mdtrec.GetFileName(&S);
  ResultBuffer->SetFileName(S);
  ResultBuffer->SetRecordStart(Mdtrec.GetLocalRecordStart());
  ResultBuffer->SetRecordEnd(Mdtrec.GetLocalRecordEnd());
}


void 
IDB::GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer) const {
  DFDT EmptyDfdt;
  *DfdtBuffer = EmptyDfdt;
  MDTREC Mdtrec;
  MainMdt->GetMdtRecord(Key, &Mdtrec);
  GPTYPE MdtS = Mdtrec.GetGlobalFileStart() + Mdtrec.GetLocalRecordStart();
  GPTYPE MdtE = Mdtrec.GetGlobalFileStart() + Mdtrec.GetLocalRecordEnd();
  INT c = MainDfdt->GetTotalEntries();
  INT x;
  DFD dfd;
  STRING FieldName, Fn;
  PFILE Fp;
  INT Done;
  for (x=1; x<=c; x++) {
    MainDfdt->GetEntry(x, &dfd);
    dfd.GetFieldName(&FieldName);
    DfdtGetFileName(FieldName, &Fn);
    Fp = fopen(Fn, "rb");
    if (!Fp) {
      perror(Fn);
      //      EXIT_ERROR;
      exit(1);
    }
    else {
      Done = 0;
      //     fseek(Fp, 0, 2);
      fseek(Fp, 0L, SEEK_END);
      INT Total = ftell(Fp) / ( sizeof(GPTYPE) * 2 );
      INT Low = 0;
      INT High = Total - 1;
      INT X = High / 2;
      INT OX;
      GPTYPE GpS, GpE;
      do {
	OX = X;
	//fseek(Fp, X * sizeof(GPTYPE) * 2, 0);
	fseek(Fp, (long)(X * sizeof(GPTYPE) * 2), SEEK_SET);
	GpFread(&GpS, 1, sizeof(GPTYPE), Fp);
	GpFread(&GpE, 1, sizeof(GPTYPE), Fp);
	if ( (MdtS <= GpS) && (MdtE >= GpE) ) {
	  fclose(Fp);
	  Done = 1;
	  DfdtBuffer->AddEntry(dfd);
	}
	if (MdtE < GpS) {
	  High = X;
	} else {
	  Low = X + 1;
	}
	X = (Low + High) / 2;
	if (X < 0) {
	  X = 0;
	} else {
	  if (X >= Total) {
	    X = Total - 1;
	  }
	}
      } while ( (X != OX) && (!Done) );
      if (!Done) {
        fclose(Fp);
      }
//      fclose(Fp);
    }
  }
}


void 
IDB::ComposeDbFn(STRING *StringBuffer, const CHR *Suffix) const {
  GetDbFileStem(StringBuffer);
  StringBuffer->Cat(Suffix);
}


void 
IDB::GetDbFileStem(STRING *StringBuffer) const {
  *StringBuffer = DbPathName;
  StringBuffer->Cat(DbFileName);
}




DOCTYPE*
IDB::GetDocTypePtr(const STRING& DocType) const {
  DOCTYPE *DoctypePtr = DocTypeReg->GetDocTypePtr(DocType);
  if (DoctypePtr) {
    return DoctypePtr;
  } else {
    return DocTypeReg->GetDocTypePtr("");
  }
}


GDT_BOOLEAN 
IDB::ValidateDocType(const STRING& DocType) const {
  return (DocTypeReg->GetDocTypePtr(DocType) != NULL)? GDT_TRUE : GDT_FALSE;
}


void 
IDB::SetIndexingMemory(const UINT4 MemorySize) {
#if defined(_MSDOS) && !defined(_WIN32)
  UINT4 DefaultMemSize = 16;
#elif defined(MEMTEST)
  UINT4 DefaultMemSize = 32;
#else
  UINT4 DefaultMemSize = 1024;
#endif
  DefaultMemSize *= 1024;
  if (MemorySize < DefaultMemSize) {
    IndexingMemory = DefaultMemSize;
    // ER
    return;
  }
  IndexingMemory = MemorySize;
}


IRSET*
IDB::AndSearch(const SQUERY& SearchQuery) {
  if (!IsDbCompatible()) {
    return (new IRSET(this));
  }
  STRING GlobalDoctype;
  GetGlobalDocType(&GlobalDoctype);
  DOCTYPE *DoctypePtr;
  DoctypePtr = DocTypeReg->GetDocTypePtr(GlobalDoctype);
  SQUERY Query;
  Query = SearchQuery;
  DoctypePtr->BeforeSearching(&Query);
  IRSET *RsetPtr;
  RsetPtr = MainIndex->AndSearch(Query);
  RsetPtr = DoctypePtr->AfterSearching(RsetPtr);
  return RsetPtr;
  //  return MainIndex->AndSearch(SearchQuery);
}


IRSET*
IDB::Search(const SQUERY& SearchQuery) {
  if (!IsDbCompatible()) {
    return (new IRSET(this));
  }

  STRING GlobalDoctype;
  DOCTYPE *DoctypePtr;
  SQUERY Query;
  IRSET *RsetPtr;

  GetGlobalDocType(&GlobalDoctype);
  DoctypePtr = DocTypeReg->GetDocTypePtr(GlobalDoctype);
  Query = SearchQuery;

  DoctypePtr->BeforeSearching(&Query);
  RsetPtr = MainIndex->Search(Query);
  RsetPtr = DoctypePtr->AfterSearching(RsetPtr);

  return RsetPtr;
}


void 
IDB::BeginRsetPresent(const STRING& RecordSyntax) {
  STRING GlobalDoctype;
  GetGlobalDocType(&GlobalDoctype);
  PDOCTYPE DoctypePtr;
  DoctypePtr = DocTypeReg->GetDocTypePtr(GlobalDoctype);
  DoctypePtr->BeforeRset(RecordSyntax);
}


void 
IDB::EndRsetPresent(const STRING& RecordSyntax) {
  STRING GlobalDoctype;
  GetGlobalDocType(&GlobalDoctype);
  PDOCTYPE DoctypePtr;
  DoctypePtr = DocTypeReg->GetDocTypePtr(GlobalDoctype);
  DoctypePtr->AfterRset(RecordSyntax);
}


void 
IDB::DfdtGetFileName(const STRING& FieldName, STRING *StringBuffer) const 
{
  DFD Dfd;
  STRING f,g,h,FN;
	
  FN=FieldName;
  FileNames.GetValue(FN,&f);
  if(f==""){
    MainDfdt->GetDfdRecord(FieldName, &Dfd);
    INT FileNumber = Dfd.GetFileNumber();
    CHR s[16];
    INT x, y;
    ComposeDbFn(StringBuffer, ".");
    if (FileNumber > 999) {
      FileNumber = 0;
    }
    sprintf(s, "%d", FileNumber);
    y = 3 - strlen(s);
    for (x=1; x<=y; x++) {
      StringBuffer->Cat("0");
    }
    StringBuffer->Cat(s);
    g=FN;
    g.Cat("=");
    h=*StringBuffer;
    g.Cat(h);
  //  printf("Adding %s\n",g.NewCString());
    FileNames.AddEntry(g);
  }else{
    *StringBuffer=f;
  }
}


static int 
IdbCompareFcsOnDisk(const void* FcPtr1, const void* FcPtr2) {
  fseek(GlobalFcFp, (((LONG)FcPtr2) - 1) * sizeof(FC), SEEK_SET);
  static FC Fc;
  if (fread((char*)&Fc, 1, sizeof(Fc), GlobalFcFp) != sizeof(Fc)) {
    return 0;
  }
  if (GlobalWrongEndian) {
    Fc.FlipBytes();
  }
  if ( ( ((FC*)FcPtr1)->GetFieldStart() <= Fc.GetFieldStart() ) &&
      ( ((FC*)FcPtr1)->GetFieldEnd() >= Fc.GetFieldEnd() ) ) {
    return 0;
  } else {
    if ( ((FC*)FcPtr1)->GetFieldStart() < Fc.GetFieldStart() ) {
      return -1;
    } else {
      return 1;
    }
  }
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  const STRING& FieldType,
		  STRING* StringBuffer) const {
  STRLIST     Strlist;
  GDT_BOOLEAN Status;
  DOUBLE      Numeric;

  *StringBuffer = "";
  if (FieldType.CaseEquals("NUM")) {
    Status = GetFieldData(ResultRecord, FieldName, &Numeric);
    if (Status) {
      *StringBuffer = Numeric;
    }
  } else if (FieldType.CaseEquals("DATE")) {
    SRCH_DATE   dDate;
    Status = GetFieldData(ResultRecord, FieldName, &dDate);
    if (Status) {
      Numeric = dDate.GetValue();
      *StringBuffer = Numeric;
    }
  } else if (FieldType.CaseEquals("DATE-RANGE")) {
    DATERANGE   rDate;
    SRCH_DATE   dDate;
    STRING      HoldDate;
    STRING      ReturnDateRange;

    Status = GetFieldData(ResultRecord, FieldName, &rDate);
    if (Status) {
      dDate = rDate.GetStart();
      Numeric = dDate.GetValue();
      ReturnDateRange = Numeric;
      ReturnDateRange.Cat(" ");
      dDate = rDate.GetEnd();
      Numeric = dDate.GetValue();
      HoldDate = Numeric;
      ReturnDateRange.Cat(HoldDate);
      *StringBuffer = ReturnDateRange;
    }
  } else if (FieldType.CaseEquals("TEXT")) {
    Status = GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      //      Strlist.Join(" ", StringBuffer);
      Strlist.Join("|", StringBuffer);
    }
  } else {
    Status = GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      //      Strlist.Join(" ", StringBuffer);
      Strlist.Join("|", StringBuffer);
    }
  }
  return(Status);
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  STRING* StringBuffer) const {
  STRLIST Strlist;
  GDT_BOOLEAN Status;
  Status = GetFieldData(ResultRecord, FieldName, &Strlist);
  if (Status)
    Strlist.Join("|", StringBuffer);
  //    Strlist.Join(",", StringBuffer);
  return(Status);
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  STRLIST* StrlistBuffer) const {
  StrlistBuffer->Clear();
  STRING DfFileName;
  
  DfdtGetFileName(FieldName, &DfFileName);
  PFILE fp = fopen(DfFileName, "rb");
  if (!fp) {
    //perror(DfFileName);
    return(GDT_FALSE);
  }
  else {
    STRING ResultKey;
    MDTREC MdtRecord;
    ResultRecord.GetKey(&ResultKey);
    MainMdt->GetMdtRecord(ResultKey, &MdtRecord);
    INT GpStart = MdtRecord.GetGlobalFileStart() 
      + MdtRecord.GetLocalRecordStart();
    INT GpEnd = MdtRecord.GetGlobalFileStart() 
      + MdtRecord.GetLocalRecordEnd();
    PFILE fpd;
    CHR *p;
    INT x, y;
    STRING Fn;
    // binary search to find matching FC pair
	 //    fseek(fp, 0, 2);
    fseek(fp, 0L, SEEK_END);
    LONG Size = ftell(fp);
    FC Fc;
    Fc.SetFieldStart(GpStart);
    Fc.SetFieldEnd(GpEnd);
    FC* FcPtr;
    GlobalFcFp = fp;
    GlobalWrongEndian = IsWrongEndian();
    /*
      #ifndef __SUNPRO_CC
      FcPtr = (FC*)bsearch(&Fc, 
      (void*)1, 
      Size / sizeof(FC), 1, 
      IdbCompareFcsOnDisk);
      #else
    */
    
    FcPtr = (FC*)bsearch((char*)&Fc, (char*)1, Size / sizeof(FC), 1, 
			 IdbCompareFcsOnDisk);
    ///#endif
	  
	  if (FcPtr) {
	    LONG Pos, InitPos;
	    GPTYPE OldStart=0, OldEnd=0;
	    Pos = (((LONG)FcPtr) - 1);
	    fseek(fp, Pos * sizeof(FC), SEEK_SET);
	    GPTYPE GpPair[2];
	    GpFread(GpPair, 1, sizeof(FC), fp);
	    InitPos = Pos;
	    // work backwards to find first pair
		 while ( (Pos >= 0) 
			 && (GpPair[0] >= Fc.GetFieldStart()) 
			 && (GpPair[1] <= Fc.GetFieldEnd()) ) {
		   
		   OldStart = GpPair[0];
		   OldEnd = GpPair[1];
		   
		   // extract field from document
			MdtRecord.GetFullFileName(&Fn);
		   fpd = fopen(Fn, "rb");
		   if (!fpd) {
		     perror(Fn);
		     return(GDT_FALSE);
		   } else {
		     x = GpPair[1] - GpPair[0] + 1;
		     p = new CHR[x+1];
		     //	  fseek(fpd, (long)(GpPair[0] - MdtRecord.GetGlobalFileStart()), 0);
		     fseek(fpd, (long)(GpPair[0] - MdtRecord.GetGlobalFileStart()), SEEK_SET);
		     y = fread(p, 1, x, fpd);
		     p[y] = '\0';
		     StrlistBuffer->AddEntry(p);
		     delete [] p;
		     fclose(fpd);
		   }
		   
		   Pos--;
		   fseek(fp, Pos * sizeof(FC), SEEK_SET);
		   GpFread(GpPair, 1, sizeof(FC), fp);
		 }
	    StrlistBuffer->Reverse();
	    
	    // OldStart and OldEnd will be undefined if FcPtr doesnt exist!
	    GpPair[0] = OldStart;
	    GpPair[1] = OldEnd;
	    // work forwards to find last pair
		 Pos = InitPos + 1;
	    SIZE_T BytesRead = sizeof(FC);
	    while ( (BytesRead == sizeof(FC)) 
		    && (GpPair[0] >= Fc.GetFieldStart()) 
		    && (GpPair[1] <= Fc.GetFieldEnd()) ) {
	      fseek(fp, Pos * sizeof(FC), SEEK_SET);
	      BytesRead = GpFread(GpPair, 1, sizeof(FC), fp);
	      if ( (BytesRead == sizeof(FC)) 
		   && (GpPair[0] >= Fc.GetFieldStart()) 
		   && (GpPair[1] <= Fc.GetFieldEnd()) ) { // I know this is ugly
		   // extract field from document
		   MdtRecord.GetFullFileName(&Fn);
		   fpd = fopen(Fn, "rb");
		   if (!fpd) {
		     perror(Fn);
		     return(GDT_FALSE);
		   } else {
		     x = GpPair[1] - GpPair[0] + 1;
		     p = new CHR[x+1];
		     fseek(fpd, (long)(GpPair[0] - MdtRecord.GetGlobalFileStart()), 
			   SEEK_SET);
		     y = fread(p, 1, x, fpd);
		     p[y] = '\0';
		     StrlistBuffer->AddEntry(p);
		     delete [] p;
		     fclose(fpd);
		   }
	      }
	      Pos++;
	    }
	  } else {
	    fclose(fp);
	    return(GDT_FALSE);
	  }
    fclose(fp);
    return(GDT_TRUE);
  }
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  DOUBLE* Buffer) const {

  STRING      DfFileName;
  STRING      ResultKey;
  MDTREC      MdtRecord;
  INT4        GpStart, GpEnd;
  SearchState Status;
  NUMERICLIST List;
  INT4        Start=-1, End=-1;
  DOUBLE      fValue;
  //INT4 Pointer=0, Value, ListCount;

  DfdtGetFileName(FieldName, &DfFileName);

  ResultRecord.GetKey(&ResultKey);
  MainMdt->GetMdtRecord(ResultKey, &MdtRecord);
  GpStart = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordStart();
  GpEnd   = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordEnd();
  
  // Start is the smallest index in the table 
  // for which GpStart is <= to the table value
  Status = List.Find(DfFileName, GpStart, ZRelGT, &Start);
  
  if (Status == TOO_LOW)    // We ran off the bottom end without a match
    Status = NO_MATCH;
  //    if (Status == NO_MATCH)   // No matching values - bail out
  //continue;
  
  // End is the largest index in the table for which
  // GpEnd is >= to the table value;
  Status = List.Find(DfFileName, GpEnd, ZRelLT, &End);
  
  if (Status == TOO_HIGH)   // We ran off the top
    Status = NO_MATCH;
  
  List.LoadTable(Start, End, GP_BLOCK);
  fValue = List.GetNumericValue(0);
  *Buffer = fValue;
  
  return GDT_TRUE;
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  DATERANGE* Buffer) const {
  STRING       DfFileName;
  STRING       ResultKey;
  MDTREC       MdtRecord;
  INT4         GpStart, GpEnd;
  SearchState  Status;
  INTERVALLIST List;
  INT4         Start=-1, End=-1;
  DOUBLE       fValue;
  DATERANGE    Value;

  DfdtGetFileName(FieldName, &DfFileName);

  ResultRecord.GetKey(&ResultKey);
  MainMdt->GetMdtRecord(ResultKey, &MdtRecord);
  GpStart = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordStart();
  GpEnd   = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordEnd();
  
  // Start is the smallest index in the table 
  // for which GpStart is <= to the table value
  Status = List.Find(DfFileName, GpStart, ZRelGT, PTR_BLOCK, &Start);
  
  if (Status == TOO_LOW)    // We ran off the bottom end without a match
    Status = NO_MATCH;
  //    if (Status == NO_MATCH)   // No matching values - bail out
  //continue;
  
  // End is the largest index in the table for which
  // GpEnd is >= to the table value;
  Status = List.Find(DfFileName, GpEnd, ZRelLT, PTR_BLOCK, &End);
  
  if (Status == TOO_HIGH)   // We ran off the top
    Status = NO_MATCH;
  
  List.LoadTable(Start, End, PTR_BLOCK);
  fValue = List.GetStartValue(0);
  Value.SetStart(fValue);
  fValue = List.GetEndValue(0);
  Value.SetEnd(fValue);
  *Buffer = Value;
  
  return GDT_TRUE;
}


GDT_BOOLEAN 
IDB::GetFieldData(const RESULT& ResultRecord, 
		  const STRING& FieldName,
		  SRCH_DATE* Buffer) const {
  STRING       DfFileName;
  STRING       ResultKey;
  MDTREC       MdtRecord;
  INT4         GpStart, GpEnd;
  SearchState  Status;
  INTERVALLIST List;
  INT4         Start=-1, End=-1;
  DOUBLE       fValue;
  SRCH_DATE    Value;

  DfdtGetFileName(FieldName, &DfFileName);

  ResultRecord.GetKey(&ResultKey);
  MainMdt->GetMdtRecord(ResultKey, &MdtRecord);
  GpStart = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordStart();
  GpEnd   = MdtRecord.GetGlobalFileStart() + MdtRecord.GetLocalRecordEnd();
  
  // Start is the smallest index in the table 
  // for which GpStart is <= to the table value
  Status = List.Find(DfFileName, GpStart, ZRelGT, PTR_BLOCK, &Start);
  
  if (Status == TOO_LOW)    // We ran off the bottom end without a match
    Status = NO_MATCH;
  //    if (Status == NO_MATCH)   // No matching values - bail out
  //continue;
  
  // End is the largest index in the table for which
  // GpEnd is >= to the table value;
  Status = List.Find(DfFileName, GpEnd, ZRelLT, PTR_BLOCK, &End);
  
  if (Status == TOO_HIGH)   // We ran off the top
    Status = NO_MATCH;
  
  List.LoadTable(Start, End, PTR_BLOCK);
  fValue = List.GetStartValue(0);
  Value = fValue;
  *Buffer = Value;
  
  return GDT_TRUE;
}


void
IDB::Present(const RESULT& ResultRecord, const STRING& ElementSet,
             const STRING& RecordSyntax, GDT_BOOLEAN HighlightTerms,
             STRING *StringBuffer) const 
{
	STRING ESet;
	ESet = ElementSet;
	ESet.UpperCase();
	STRING DocType;
	ResultRecord.GetDocumentType(&DocType);
	PDOCTYPE DocTypePtr = DocTypeReg->GetDocTypePtr(DocType);
	DocTypePtr->Present(ResultRecord, ESet, RecordSyntax,
			    HighlightTerms, StringBuffer);
}


void 
IDB::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		  const STRING& RecordSyntax, STRING *StringBuffer) const 
{
  Present(ResultRecord, ElementSet, RecordSyntax, 
	  GDT_FALSE, StringBuffer);
}


void 
IDB::Present(const RESULT& ResultRecord, const STRING& ElementSet,
	     STRING *StringBuffer) const 
{
  STRING RecordSyntax;
  Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


void 
IDB::GetDbVersionNumber(STRING *StringBuffer) const {
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("VersionNumber");
  MainRegistry->GetData(Position, &Value);
  Value.GetEntry(1, StringBuffer);
}


void 
IDB::AddRecord(const RECORD& NewRecord) {
  STRING IndexingQueueFn;
  ComposeDbFn(&IndexingQueueFn, DbExtIndexQueue1);
  PFILE fp;
  fp = IDB::ffopen(IndexingQueueFn, "a");
  if (!fp) {
    perror(IndexingQueueFn);
    //    EXIT_ERROR;
    exit(1);
  }
  fprintf(fp, "#\n");
  NewRecord.Write(fp);
  IDB::ffclose(fp);
}


void 
IDB::DocTypeAddRecord(const RECORD& NewRecord) {
  STRING IndexingQueueFn;
  ComposeDbFn(&IndexingQueueFn, DbExtIndexQueue2);
  PFILE fp;
  fp = IDB::ffopen(IndexingQueueFn, "a");
  if (!fp) {
    perror(IndexingQueueFn);
    //    EXIT_ERROR;
    exit(1);
  }
  fprintf(fp, "#\n");
  NewRecord.Write(fp);
  IDB::ffclose(fp);
  TotalRecordsQueued++;
}


void 
IDB::SetDbState(const INT4 DbState) {
  STRING DbStateFn;
  ComposeDbFn(&DbStateFn, DbExtDbState);
  FILE* fp = fopen(DbStateFn, "wb");
  if (fp) {
    fwrite(&DbState, 1, sizeof(DbState), fp);
    fclose(fp);
  }
}


INT4 
IDB::GetDbState() {
  INT4 DbState;
  STRING DbStateFn;
  ComposeDbFn(&DbStateFn, DbExtDbState);
  FILE* fp = fopen(DbStateFn, "rb");
  if (fp) {
    if (fread(&DbState, 1, sizeof(DbState), fp) != sizeof(DbState)) {
      fclose(fp);
      return IsearchDbStateReady;
    }
    fclose(fp);
    return DbState;
  } else {
    return IsearchDbStateReady;
  }
}


void 
IDB::Index() {
  if (!IsDbCompatible()) {
    return;
  }
  STRING GlobalDoctype;
  GetGlobalDocType(&GlobalDoctype);
  PDOCTYPE DoctypePtr;
  DoctypePtr = DocTypeReg->GetDocTypePtr(GlobalDoctype);
  DoctypePtr->BeforeIndexing();
  IndexingStatus(IndexingStatusParsingFiles, 0, 0);
  STRING IqFn;
  ComposeDbFn(&IqFn, DbExtIndexQueue1);
  PFILE fp;
  fp = IDB::ffopen(IqFn, "r");
  if (!fp) {
    SetDbState(IsearchDbStateInvalid);
    return;
  }
  STRING s;
  RECORD Record;
  STRING DocType;
  PDOCTYPE DocTypePtr;
  // Check whether we need to set the Global DocType
  STRING GDocType;
  PDOCTYPE GDocTypePtr;
  GDT_BOOLEAN SetGDocType;
  GetGlobalDocType(&GDocType);
  if (GDocType == "") {
    SetGDocType = GDT_TRUE;
  } else {
    SetGDocType = GDT_FALSE;
  }
  
  GDocTypePtr = DocTypeReg->GetDocTypePtr(GDocType);
  GDocTypePtr->LoadFieldTable();
  
  do {
    s.FGet(fp, 3);
    if (s == "#") {
      Record.Read(fp); // Read a record from file queue
      Record.GetDocumentType(&DocType);
      if (SetGDocType == GDT_TRUE) { // Set Global DocType
	SetGlobalDocType(DocType);
	SetGDocType = GDT_FALSE;
      }
      DocTypePtr = DocTypeReg->GetDocTypePtr(DocType);
      DocTypePtr->AddFieldDefs();
      if (Record.GetRecordEnd() == 0) {
	DocTypePtr->ParseRecords(Record);
      } else {
	DocTypeAddRecord(Record);
      }
      MainIndex->SetDocTypePtr(DocTypePtr); // added by aw3
    }
  } while (s == "#");
  IDB::ffclose(fp);
  StrUnlink(IqFn);
  MainMdt->Resize(MainMdt->GetTotalEntries() + TotalRecordsQueued);
  ComposeDbFn(&IqFn, DbExtIndexQueue2);
  fp = IDB::ffopen(IqFn, "r");
  if (!fp) {
    SetDbState(IsearchDbStateReady);
    fprintf(stderr,"No valid files found for indexing...\n");
    //    EXIT_ERROR;
    exit(1);
  }
  MainIndex->AddRecordList(fp);
  IDB::ffclose(fp);
  StrUnlink(IqFn);
  TotalRecordsQueued = 0;
  MainFpt.CloseAll();
  DoctypePtr->AfterIndexing();
  SetDbState(IsearchDbStateReady);
}


void 
IDB::ParseFields(RECORD *Record) {
  PDOCTYPE DocTypePtr;
  STRING DocType;
/*
  STRLIST StrList;
  STRING S;
  GetDocTypeOptions(&StrList);
  StrList.GetValue("fieldtype", &S);
*/
  Record->GetDocumentType(&DocType);
  DocTypePtr = GetDocTypePtr(DocType);
  DocTypePtr->ParseFields(Record);
}


void
IDB::ReplaceWithSpace(RECORD *Record, PCHR data, INT length)
{
  PDOCTYPE DocTypePtr;
  STRING DocType;
  
  Record->GetDocumentType(&DocType);
  DocTypePtr = GetDocTypePtr(DocType);
  DocTypePtr->ReplaceWithSpace(data, length);
}


INT 
IDB::IsStopWord(CHR* WordStart, INT WordMaximum) const {
  return ( MainIndex->IsStopWord(WordStart, WordMaximum) );
}
   

GPTYPE 
IDB::ParseWords(const STRING& Doctype, CHR* DataBuffer, INT DataLength,
		INT DataOffset, GPTYPE* GpBuffer, INT GpLength) {
  // Redirect the call to this method to the appropriate doctype.
  PDOCTYPE DocTypePtr;

  DocTypePtr = GetDocTypePtr(Doctype);
  return ( DocTypePtr->ParseWords(DataBuffer, DataLength, DataOffset, 
				  GpBuffer, GpLength) );
}
 
/*
INT 
IDB::IsSystemFile(const STRING& FileName) {
  STRING s;
  ComposeDbFn(&s, DbExtIndex);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtMdt);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtMdtKeyIndex);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtMdtGpIndex);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtDfd);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtTemp);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtIndexQueue1);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtIndexQueue2);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, DbExtDbInfo);
  if (s.Equals(FileName)) {
    return 1;
  }
  ComposeDbFn(&s, ".");
  STRING t;
  CHR b[10];
  INT x = 1;
  INT y, z;
  do {
    t = s;
    sprintf(b, "%d", x);
    y = 3 - strlen(b);
    for (z=1; z<=y; z++) {
      t.Cat("0");
    }
    t.Cat(b);
    if (t.Equals(FileName)) {
      return 1;
    }
    x++;
  } while (x < 1000);
  return 0;
}
*/

int 
IDB::IsSystemFile(const STRING& FileName) {
  STRINGINDEX i = FileName.SearchReverse('.');
  if (!i)
    return 0;

  STRING ext = FileName;
  STRING base = FileName;
  ext.EraseBefore(i);
  base.EraseAfter(i-1);

  // Does the file, without extension, match the database name?
  if (base != DbFileName)
    return 0;

  // If so, does it have one of the database file extensions?
  if (ext == DbExtIndex
      || ext == DbExtMdt
      || ext == DbExtMdtKeyIndex
      || ext == DbExtMdtGpIndex
      || ext == DbExtDfd
      || ext == DbExtTemp
      || ext == DbExtIndexQueue1
      || ext == DbExtIndexQueue2
      || ext == DbExtDbInfo
      || ext == DbExtDbState
      || ext == DbExtCentroid
      || ext == DbExtDict
      || ext == DbExtSparse)
    return 1;

  // Finally, is it one of the data field tables (i.e. has a 3-digit
  // numerical extension)
  if (ext.GetLength() == 4) {
    for (int i = 2; i <= 4; i++)
      if (!isdigit(ext.GetChr(i)))
	return 0;
    return 1;
  }
      
  return 0;
}

void 
IDB::KillAll() {
  // Delete files
  STRING s;
  ComposeDbFn(&s, DbExtIndex);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtMdt);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtMdtKeyIndex);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtMdtGpIndex);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtDfd);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtTemp);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtIndexQueue1);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtIndexQueue2);
  StrUnlink(s);
  ComposeDbFn(&s, DbExtDbInfo);
  StrUnlink(s);
  ComposeDbFn(&s, ".mno"); // temporary
  StrUnlink(s);
  ComposeDbFn(&s, DbExtDbState);
  StrUnlink(s);
  INT t = MainDfdt->GetTotalEntries();
  INT x;
  DFD Dfd;
  STRING f;
  for (x=1; x<=t; x++) {
    MainDfdt->GetEntry(x, &Dfd);
    Dfd.GetFieldName(&f);
    DfdtGetFileName(f, &s);
    StrUnlink(s);
  }
  // Delete objects
  if (MainIndex) {
    delete MainIndex;
  }
  if (MainMdt) {
    delete MainMdt;
  }
  if (MainDfdt) {
    delete MainDfdt;
  }
  delete DocTypeReg;
  // Re-init objects
  STRING IndexFN;
  ComposeDbFn(&IndexFN, DbExtIndex);
  MainIndex = new INDEX(this, IndexFN);
  STRING MDTFN;
  ComposeDbFn(&MDTFN, DbExtMdt);
  STRING FileStem;
  GetDbFileStem(&FileStem);
  MainMdt = new MDT(FileStem, GDT_FALSE);
  STRING DFDTFN;
  ComposeDbFn(&DFDTFN, DbExtDfd);
  MainDfdt = new DFDT();
  DocTypeReg = new DTREG(this);
  // Recycle Main Registry
  delete MainRegistry;
  MainRegistry = new REGISTRY("Isearch");
  DbInfoChanged = GDT_FALSE;
  // Register Isearch Version Number
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("VersionNumber");
  STRING S;
  S = IsearchVersion;
  Value.AddEntry(S);
  MainRegistry->SetData(Position, Value);
  // Register Magic Number
  Position.SetEntry(2, "MagicNumber");
  S = IsearchMagicNumber;
  Value.SetEntry(1, S);
  MainRegistry->SetData(Position, Value);
  // Register Doctype
  Position.SetEntry(2, "DocType");
  Value.SetEntry(1, "");
  MainRegistry->SetData(Position, Value);
  // Register Doctype options
  Position.SetEntry(2, "DocTypeOptions");
  Value.SetEntry(1, "");
  MainRegistry->SetData(Position, Value);
  // Register Endianness
  Position.SetEntry(2, "BigEndian");
  S = IsBigEndian();
  Value.SetEntry(1, S);
  MainRegistry->SetData(Position, Value);
  SetWrongEndian();
}


void 
IDB::SetDocumentInfo(const INT Index, const RECORD& Record) {
  MDTREC Mdtrec;
  MainMdt->GetEntry(Index, &Mdtrec);
  STRING S;
  Record.GetKey(&S);
  Mdtrec.SetKey(S);
  Record.GetFileName(&S);
  Mdtrec.SetFileName(S);
  Record.GetPathName(&S);
  Mdtrec.SetPathName(S);
  Mdtrec.SetLocalRecordStart(Record.GetRecordStart());
  Mdtrec.SetLocalRecordEnd(Record.GetRecordEnd());
  Record.GetDocumentType(&S);
  Mdtrec.SetDocumentType(S);
  // Do we just ignore the DFT???
  MainMdt->SetEntry(Index, Mdtrec);
}


void 
IDB::GetDocumentInfo(const INT Index, RECORD *RecordBuffer) const {
  MDTREC Mdtrec;
  MainMdt->GetEntry(Index, &Mdtrec);
  RECORD Record;
  STRING S;
  Mdtrec.GetKey(&S);
  Record.SetKey(S);
  Mdtrec.GetFileName(&S);
  Record.SetFileName(S);
  Mdtrec.GetPathName(&S);
  Record.SetPathName(S);
  Record.SetRecordStart(Mdtrec.GetLocalRecordStart());
  Record.SetRecordEnd(Mdtrec.GetLocalRecordEnd());
  Mdtrec.GetDocumentType(&S);
  Record.SetDocumentType(S);
  // Here needs to go a call to a function that builds the DFT.
  *RecordBuffer = Record;
}


GDT_BOOLEAN 
IDB::GetDocumentDeleted(const INT Index) const {
  MDTREC Mdtrec;
  MainMdt->GetEntry(Index, &Mdtrec);
  return Mdtrec.GetDeleted();
}


INT 
IDB::DeleteByKey(const STRING& Key) {
  INT x = MainMdt->LookupByKey(Key);
  if (x) {
    MDTREC Mdtrec;
    MainMdt->GetEntry(x, &Mdtrec);
    Mdtrec.SetDeleted(GDT_TRUE);
    MainMdt->SetEntry(x, Mdtrec);
    return 1;
  } else {
    return 0;
  }
}


INT 
IDB::UndeleteByKey(const STRING& Key) {
  INT x = MainMdt->LookupByKey(Key);
  if (x) {
    MDTREC Mdtrec;
    MainMdt->GetEntry(x, &Mdtrec);
    Mdtrec.SetDeleted(GDT_FALSE);
    MainMdt->SetEntry(x, Mdtrec);
    return 1;
  } else {
    return 0;
  }
}


SIZE_T 
IDB::CleanupDb() {
  // Compute offset GP changes for each MDTREC
  INT MdtTotalEntries = MainMdt->GetTotalEntries();
  PGPTYPE GpList;
  GpList = new GPTYPE[MdtTotalEntries];
  INT Offset = 0;
  INT x;
  MDTREC Mdtrec;
  for (x=1; x<=MdtTotalEntries; x++) {
    MainMdt->GetEntry(x, &Mdtrec);
    if (Mdtrec.GetDeleted() == GDT_TRUE) {
      Offset += Mdtrec.GetLocalRecordEnd() - Mdtrec.GetLocalRecordStart() + 1;
    } else {
      GpList[x-1] = Offset;
    }
  }
  // Remove deleted GPs from index and field files, also collapsing GP space
  INT FileNum;
  INT DfdtTotalEntries = MainDfdt->GetTotalEntries();
  DFD Dfd;
  STRING S, Fn, TempFn;
  ComposeDbFn(&TempFn, DbExtTemp);
  PFILE Fpo, Fpn;
  for (FileNum=0; FileNum<=DfdtTotalEntries; FileNum++) {
    if (FileNum == 0) {
      ComposeDbFn(&Fn, DbExtIndex);
    } else {
      MainDfdt->GetEntry(FileNum, &Dfd);
      Dfd.GetFieldName(&S);
      DfdtGetFileName(S, &Fn);
    }
    if ( (Fpo = fopen(Fn, "rb")) == NULL) {
      if (FileNum == 0) // *.inx may or may not exist...
	{ continue; } 
      perror(Fn);
      //      EXIT_ERROR;
      exit(1);
    }
    if ( (Fpn = fopen(TempFn, "wb")) == NULL) {
      perror(TempFn);
      //      EXIT_ERROR;
      exit(1);
    }
    //should I bail if this doesnt work? -jem.
    GPTYPE Gp;
    while (GpFread(&Gp, 1, sizeof(GPTYPE), Fpo)) {
      x = MainMdt->LookupByGp(Gp);
      MainMdt->GetEntry(x, &Mdtrec);
      if (Mdtrec.GetDeleted() == GDT_FALSE) {
	Gp -= GpList[x-1];
	GpFwrite(&Gp, 1, sizeof(GPTYPE), Fpn);
      }
    }
    fclose(Fpn);
    fclose(Fpo);
    CHR *Temp1, *Temp2;
    Temp1 = Fn.NewCString();
    Temp2 = TempFn.NewCString();
 
#if defined(_MSDOS) || defined(_WIN32)
 
    /*
     * MSDOS / WIN32 rename doesnt remove an existing file so
     * we have to do it ourselves.
     */
 
    remove(Temp1);
#endif
 
    rename(Temp2, Temp1);
    delete [] Temp1;
    delete [] Temp2;
    
  }
  // Update GPs in MDT
  for (x=1; x<=MdtTotalEntries; x++) {
    MainMdt->GetEntry(x, &Mdtrec);
    if (Mdtrec.GetDeleted() == GDT_FALSE) {
      Mdtrec.SetGlobalFileStart(Mdtrec.GetGlobalFileStart() - GpList[x-1]);
      Mdtrec.SetGlobalFileEnd(Mdtrec.GetGlobalFileEnd() - GpList[x-1]);
      MainMdt->SetEntry(x, Mdtrec);
    }
  }
  /*
    // Remove MDTREC's marked as deleted from MainMdt
    INT n = 1;
    for (x=1; x<=MdtTotalEntries; x++) {
    MainMdt->GetEntry(x, &Mdtrec);
    if (Mdtrec.GetDeleted() == GDT_FALSE) {
    if (x != n) {
    Mdtrec.SetGlobalFileStart(Mdtrec.GetGlobalFileStart() - GpList[x-1]);
    Mdtrec.SetGlobalFileEnd(Mdtrec.GetGlobalFileEnd() - GpList[x-1]);
    MainMdt->SetEntry(n, Mdtrec);
    }
    n++;
    }
    }
    INT Count = MdtTotalEntries - n + 1;
    MainMdt->SetTotalEntries(n - 1);
  */
  delete [] GpList;
  return (MainMdt->RemoveDeleted());
}


void
IDB::SetDocTypeOptions() {
  STRING S;
  STRLIST Position, Value;
  INT i,n;

  Position.AddEntry("DbInfo");
  Position.AddEntry("DocTypeOptions");
  
  //  DocTypeOptions.Join(",",&S);
  //  S.UpperCase();
  n = DocTypeOptions.GetTotalEntries();
  for (i=1;i<=n;i++) {
    DocTypeOptions.GetEntry(i,&S);
    Value.AddEntry(S);
  }
  MainRegistry->SetData(Position, Value);
  DbInfoChanged = GDT_TRUE;
}


void 
IDB::SetGlobalDocType(const STRING& NewGlobalDocType) {
  STRING S;
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("DocType");
  S = NewGlobalDocType;
  S.UpperCase();
  Value.AddEntry(S);
  MainRegistry->SetData(Position, Value);
  DbInfoChanged = GDT_TRUE;
}


void 
IDB::GetGlobalDocType(STRING *StringBuffer) const {
  STRLIST Position, Value;
  Position.AddEntry("DbInfo");
  Position.AddEntry("DocType");
  MainRegistry->GetData(Position, &Value);
  Value.GetEntry(1, StringBuffer);
}


void 
IDB::WriteCentroid(FILE* fp) {
       MainIndex->WriteCentroid(fp);
}


IDB::~IDB() {
  if (DebugMode) {
    MainMdt->Dump();
    MainIndex->DumpIndex(DebugSkip);
  }

  //  FlushFiles();

  if (MainIndex) {
    delete MainIndex;
  }
  if (MainMdt) {
    delete MainMdt;
  }
  if (MainDfdt) {
    delete MainDfdt;
  }
  delete DocTypeReg;
  delete MainRegistry;
}


void
IDB::FlushFiles() {
  if (IsDbCompatible()) {
    STRLIST Position;
    Position.AddEntry("DbInfo");
    if ( (MainMdt->GetChanged()) || (MainDfdt->GetChanged()) ||
	(DbInfoChanged) ) {
      // Register Isearch Version Number
      STRLIST Value;
      Position.AddEntry("VersionNumber");
      STRING S;
      S = IsearchVersion;
      Value.AddEntry(S);
      MainRegistry->SetData(Position, Value);
      // Save DbInfo registry
      Position.Clear();
      Position.AddEntry("DbInfo");
      STRING DbInfoFn;
      ComposeDbFn(&DbInfoFn, DbExtDbInfo);
      MainRegistry->SaveToFile(DbInfoFn, Position);
    }

    STRING DFDTFN;
    ComposeDbFn(&DFDTFN, DbExtDfd);
    if (MainDfdt->GetChanged()) {
      MainDfdt->SaveTable(DFDTFN);
    }
  }
}



/* Might use these someday
   
   void IDB::SetTitle(const STRING& NewTitle) {
   Title = NewTitle;
   }
   
   void IDB::GetTitle(PSTRING StringBuffer) {
   *StringBuffer = Title;
   }
   
   void IDB::SetComments(const STRING& NewComments) {
   Comments = NewComments;
   }
   
   void IDB::GetComments(PSTRING StringBuffer) {
   *StringBuffer = Comments;
   }
   

   void IDB::GenerateKeys() {
   MainMdt->GenerateKeys();
   }


   void IDB::SelectRegions(const RECORD& Record, FCT* RegionsPtr) const {
   PDOCTYPE DocTypePtr;
   STRING DocType;
   Record.GetDocumentType(&DocType);
   DocTypePtr = GetDocTypePtr(DocType);
   RegionsPtr->Clear();
   DocTypePtr->SelectRegions(Record, RegionsPtr);
   }


   FILE*
   IDB::ffopen(const STRING& FileName, const CHR *Type) {
   //	return MainFpt.ffopen(FileName, Type);
   return fopen(FileName, Type);
   }
   
  
   INT 
   IDB::ffclose(FILE *FilePointer) { 
   //	return MainFpt.ffclose(FilePointer);
   return fclose(FilePointer);
   }
*/


/* Inlined
  GDT_BOOLEAN 
  IDB::IsWrongEndian() const {
  return WrongEndian;
  }


  void 
  IDB::SetMergeStatus(GDT_BOOLEAN x) {
  MainIndex->SetMergeStatus(x);
  }


  void 
  IDB::GetAllDocTypes(STRLIST *StringListBuffer) const {
  DocTypeReg->GetDocTypeList(StringListBuffer);
  }

  
  void 
  IDB::GetDocTypeOptions(STRLIST *StringListBuffer) const {
  *StringListBuffer = DocTypeOptions;
  }


  void 
  IDB::GetDfdt(DFDT *DfdtBuffer) const {
  *DfdtBuffer = *MainDfdt;
  }


  void 
  IDB::DfdtAddEntry(const DFD& NewDfd) {
  MainDfdt->AddEntry(NewDfd);
  }


  void 
  IDB::DfdtGetEntry(const INT Index, DFD *DfdRecord) const {
  MainDfdt->GetEntry(Index, DfdRecord);
  }


  INT 
  IDB::DfdtGetTotalEntries() const {
  return MainDfdt->GetTotalEntries();
  }


  void 
  IDB::IndexingStatus(const INT StatusMessage, const STRING *FileName,
  const INT WordCount) const 
  {
  }


  INT 
  IDB::GetTotalRecords() const {
  return MainMdt->GetTotalEntries();
  }


  UINT4 
  IDB::GetIndexingMemory() const {
  return IndexingMemory;
  }


  void 
  IDB::DebugModeOn() {
  DebugMode = 1;
  }


  void 
  IDB::DebugModeOff() {
  DebugMode = 0;
  }


  void 
  IDB::SetDebugSkip(const INT Skip) {
  DebugSkip = Skip;
  }


  PMDT 
  IDB::GetMainMdt() {
  return MainMdt;
  }
  


  PDFDT 
  IDB::GetMainDfdt() {
  return MainDfdt;
  }


  void 
  IDB::GetIsearchVersionNumber(STRING *StringBuffer) const {
  *StringBuffer = IsearchVersion;
  }


  void 
  IDB::MergeIndexFiles(INT m) {
  MainIndex->MergeIndexFiles( m);
  }
  

  void 
  IDB::CollapseIndexFiles(INT m) {
  MainIndex->CollapseIndexFiles( m);
  }
*/

void
MakeDbGilsRec(IDB *IdbPtr, STRING& PathName, STRING& FileName, STRING* buffer)
{
  /* Get today''s date */
  time_t today;
  struct tm    *t;
  CHR *date=(CHR*)NULL;
  STRING DbName;

  IdbPtr->GetDbFileStem(&DbName);

  today = time((time_t *)NULL);
  t = localtime(&today);
  if ((date = (CHR *)malloc(9))) {
      strftime(date,9,"%Y%m%d",t);
  }

  /* Put out the header */
  buffer->Cat("<?XML VERSION=\"1.0\" ENCODING=\"UTF-8\" ?>\n");
  buffer->Cat("<!DOCTYPE Locator SYSTEM \"xml.dtd\" >\n");

  buffer->Cat("<Locator>\n\n");

  buffer->Cat("<Title>\n");
  buffer->Cat(DbName);
  buffer->Cat("\n</Title>\n\n");

  buffer->Cat("<Originator> </Originator>\n\n");
  buffer->Cat("<Contributor> </Contributor>\n\n");
  buffer->Cat("<Date-of-Publication> </Date-of-Publication>\n\n");
  buffer->Cat("<Place-of-Publication> </Place-of-Publication>\n\n");

  buffer->Cat("<Language-of-Resource>\n");
  buffer->Cat("ENG\n");
  buffer->Cat("</Language-of-Resource>\n\n");

  buffer->Cat("<Abstract>\n</Abstract>\n\n");
  buffer->Cat("<Uncontrolled-Subject-Terms> ");
  buffer->Cat("</Uncontrolled-Subject-Terms>\n\n");

  buffer->Cat("<Availability>\n");
  buffer->Cat("<Medium> </Medium>\n");
  buffer->Cat("<Name>\n");
  /* can we get this from the author? */
  buffer->Cat("\n</Name>\n");
  buffer->Cat("<Organization> </Organization>\n");
  buffer->Cat("<Street-Address> </Street-Address>\n");
  buffer->Cat("<City> </City>\n");
  buffer->Cat("<State-or-Province> </State-or-Province>\n");
  buffer->Cat("<Postal-Code> </Postal-Code>\n");
  buffer->Cat("<Network-Address> </Network-Address>\n");
  buffer->Cat("<Telephone> </Telephone>\n");
  buffer->Cat("<Fax> </Fax>\n");
  buffer->Cat("<Resource-Description> </Resource-Description>\n");
  buffer->Cat("<Available-Linkage>\n");
  buffer->Cat("<Linkage-Type>");
  buffer->Cat("text/HTML");
  buffer->Cat("</Linkage-Type>\n\n");
  buffer->Cat("<Linkage> ");
  /* can we come up with the linkage to put here? */
  buffer->Cat("</Linkage>\n");
  buffer->Cat("</Available-Linkage>\n");
  buffer->Cat("</Availability>\n\n");

  buffer->Cat("<Point-of-Contact>\n\n");
  buffer->Cat("<Name> ");
  /* can we get this from the author? */
  buffer->Cat("</Name>\n");
  buffer->Cat("<Organization> </Organization>\n");
  buffer->Cat("<Street-Address> </Street-Address>\n");
  buffer->Cat("<City> </City>\n");
  buffer->Cat("<State-or-Province> </State-or-Province>\n");
  buffer->Cat("<Postal-Code> </Postal-Code>\n");
  buffer->Cat("<Network-Address> </Network-Address>\n");
  buffer->Cat("<Telephone> </Telephone>\n");
  buffer->Cat("<Fax> </Fax>\n");
  buffer->Cat("</Point-of-Contact>\n\n");

  buffer->Cat("<Data-Sources> </Data-Sources>\n");
  buffer->Cat("<Access-Constraints> </Access-Constraints>\n");
  buffer->Cat("<Use-Constraints> </Use-Constraints>\n");
  buffer->Cat("<Purpose> </Purpose>\n");
  buffer->Cat("<Agency-Program> </Agency-Program>\n");

  buffer->Cat("<Cross-Reference>\n");
  buffer->Cat("<Title> </Title>\n");
  buffer->Cat("<Cross-Reference-Linkage>\n");
  buffer->Cat("<Linkage> </Linkage>\n");
  buffer->Cat("<Linkage-Type> </Linkage-Type>\n");
  buffer->Cat("</Cross-Reference-Linkage>\n");
  buffer->Cat("</Cross-Reference>\n\n");

  buffer->Cat("<Methodology>\n");
  buffer->Cat("Automatically generated by CNIDR Iindex\n");
  buffer->Cat("</Methodology>\n");
  
  buffer->Cat("<Control-Identifier>\n");
  buffer->Cat(DbName);
  //  buffer->Cat(PathName);
  //  buffer->Cat(FileName);
  buffer->Cat("\n</Control-Identifier>\n\n");
  buffer->Cat("<Language-of-Record>\n");
  buffer->Cat("ENG\n");
  buffer->Cat("</Language-of-Record>\n\n");

  buffer->Cat("<Date-of-Last-Modification>\n");
  buffer->Cat(date);
  buffer->Cat("\n</Date-of-Last-Modification>\n\n");

  buffer->Cat("<Record-Source> </Record-Source>\n");
  //  buffer->Cat("</Locator>\n"); // Has to be written later
}
