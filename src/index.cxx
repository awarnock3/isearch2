/* $Id: index.cxx,v 1.51 2000/10/12 18:00:06 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1994.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*@@@
File:		index.cxx
Version:	1.01
$Revision: 1.51 $
Description:	Class INDEX
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "common.hxx"
#include "sw.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "date.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "mergeunit.hxx"
#include "filemap.hxx"
#ifdef DICTIONARY
#include "dictionary.hxx"
#endif

//const INT StopWordSize = 400;
const INT StopWordSize = (sizeof(stoplist)/sizeof(stoplist[0]));
#define CACHELIMIT 50000	// 50,000 entries, at 8 bytes each
#define MAXINDEXNUM 20

static PCHR MemoryData;
static INT MemoryDataLength;

FILE *flist[40];
STRING Names[40];
INT fcount=0;


void 
BufferClean(CHR *Buffer)
{
  INT z;
  for (z = 0; z<StringCompLength; z++) {
    //    if (!isalnum(Buffer[z]))
    if (!IsAlnum(Buffer[z]))
      Buffer[z] = ' ';
  }
}


INDEX::INDEX(const PIDBOBJ DbParent, const STRING& NewFileName) 
{
  STRING CheckName;
  CHR Tmp[256];
  Parent = DbParent;
  IndexFileName = NewFileName;
  Parent->ComposeDbFn(&CheckName, ".num");
  //  SetCache=new RCACHE(Parent);
  // see if .num file exists...  
  FILE *fa=Parent->ffopen(CheckName,"r");
  if(fa){
    if (!fgets(Tmp,256,fa)) {
      Parent->ffclose(fa);
      IndexNum=0;
      return;
    }
    IndexNum=atoi(Tmp);
    fclose(fa);
  }else
    IndexNum=0;
#ifdef DICTIONARY
  Dict = new DICTIONARY(DbParent);
#endif
}

/* Inlined
  void 
  INDEX::SetDocTypePtr(const PDOCTYPE NewDocTypePtr) {
  DocTypePtr = NewDocTypePtr;
  }


  PDOCTYPE 
  INDEX::GetDocTypePtr() {
  return DocTypePtr;
  }
*/

int 
MemIndexCompare(const void* x, const void* y) {
  return strncmp(MemoryData + (*((PGPTYPE)x)),
		 MemoryData + (*((PGPTYPE)y)), StringCompLength);
}


#ifdef DICTIONARY
void 
INDEX::CreateDictionary(void) {
  Dict->CreateNew();
}


void 
INDEX::CreateCentroid(void) {
  FILE *out=(FILE*)NULL;
  STRING CentroidName;
  Parent->ComposeDbFn(&CentroidName, DbExtCentroid);
  out = Parent->ffopen(CentroidName, "w");
  if (!out) {
    fprintf(stderr,"Can't open ");
    CentroidName.Print(stderr);
    fprintf(stderr,":");
    fprintf(stderr,"%s\n", strerror(errno));
  }
  if (Dict->GetSearchable())
    Dict->Print(out);
  else {
    fprintf(stderr,"You must generate a dictionary with the -dict option,\n");
    fprintf(stderr,"before you can create a centroid.\n");
  }
}
#endif


void 
INDEX::WriteFieldData(const RECORD& Record, const GPTYPE GpOffset) 
{
  DFT dft;
  Record.GetDft(&dft);
  INT total = dft.GetTotalEntries();
  SIZE_T ytotal;
  INT x, y;
  DF df;
  FCT fct;
  FC fc;
  PFILE fp=(PFILE)NULL;
  STRING FieldName, FileName;
  STRING FieldType;
  GPTYPE gp;
  CHR *Buffer;
  INT4 fLen;
  INT4 Val;
  GDT_BOOLEAN doClose;
  DOUBLE fVal;
  DOUBLE fStartVal, fEndVal;
 
  STRING tmp;
  CHR MyFile[256],tt[256],*p;
  INT FileVal,k,j;

  PDOCTYPE DocTypePtr;
  STRING DocType;
  Record.GetDocumentType(&DocType);
  DocTypePtr = GetDocTypePtr();
 
  for (x=1; x<=total; x++) {
    dft.GetEntry(x, &df);
    df.GetFieldName(&FieldName);
   
    Parent->FieldTypes.GetValue(FieldName, &FieldType);
    Parent->DfdtGetFileName(FieldName, &FileName);
    
    // do a simple test cache here...
    
    doClose=GDT_FALSE;
    fp=(FILE*)NULL;

    if (fp==NULL) {
      fp = Parent->ffopen(FileName, "ab");
      if (!fp) {
	perror(FileName);    
	EXIT_ERROR;
      }
    }
    
    df.GetFct(&fct);
    ytotal = fct.GetTotalEntries();
    for (y=1; y<=ytotal; y++) {
      fct.GetEntry(y, &fc);
      
      if (FieldType.CaseEquals("text")) {
	gp = fc.GetFieldStart() + GpOffset;
	//	fwrite(&gp, 1, sizeof(GPTYPE), fp);
	
	Parent->GpFwrite(&gp, 1, sizeof(GPTYPE), fp);
	gp = fc.GetFieldEnd() + GpOffset;
	
	//	fwrite(&gp, 1, sizeof(GPTYPE), fp);
	Parent->GpFwrite(&gp, 1, sizeof(GPTYPE), fp);
	
#ifdef DEBUG
	printf("FieldName=");
	FieldName.Print();
	printf("\nFieldType=");
	FieldType.Print();
	printf(", gp(start)=%i, gp(end)=%i\n", gp, gp);
#endif
	
	
      } else if (FieldType.CaseEquals("num")) {
	
	gp = fc.GetFieldStart() + GpOffset;
	
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);
	//	Buffer[fLen] = '\0';
	fVal = DocTypePtr->ParseNumeric(Buffer);
	
	fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	fwrite((char*)&fVal, 1, sizeof(DOUBLE), fp);
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType;
	cout << ", gp=" << gp;
	cout << ", Val=" << fVal<< endl;
#endif
	
	delete [] Buffer;

      } else if (FieldType.CaseEquals("date")) {

	gp = fc.GetFieldStart() + GpOffset;
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);
	//	Buffer[fLen] = '\0';
	
	DocTypePtr->ParseDate(Buffer,&fStartVal,&fEndVal);
	fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	fwrite((char*)&fStartVal, 1, sizeof(DOUBLE), fp);
	fwrite((char*)&fEndVal, 1, sizeof(DOUBLE), fp);
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType << endl;
	cout << "gp=" << gp;
	cout << ", [" << fStartVal;
	cout << ", " << fEndVal << "]" << endl;
#endif
	
	delete [] Buffer;
	
      } else if (FieldType.CaseEquals("date-range")) {
	gp = fc.GetFieldStart() + GpOffset;
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);
	//	Buffer[fLen] = '\0';
	
	DocTypePtr->ParseDateRange(Buffer,&fStartVal,&fEndVal);
	fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	fwrite((char*)&fStartVal, 1, sizeof(DOUBLE), fp);
	fwrite((char*)&fEndVal, 1, sizeof(DOUBLE), fp);
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType << endl;
	cout << "gp=" << gp;
	cout << ", [" << (INT)fStartVal;
	cout << ", " << (INT)fEndVal << "]" << endl;
#endif
	
	delete [] Buffer;
	
      } else if (FieldType.CaseEquals("range")) {
	gp = fc.GetFieldStart() + GpOffset;
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);
	//	Buffer[fLen] = '\0';
	
	DocTypePtr->ParseRange(Buffer,&fStartVal,&fEndVal);
	fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	fwrite((char*)&fStartVal, 1, sizeof(DOUBLE), fp);
	fwrite((char*)&fEndVal, 1, sizeof(DOUBLE), fp);
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType << endl;
	cout << "gp=" << gp;
	cout << ", [" << fStartVal;
	cout << ", " << fEndVal << "]" << endl;
#endif
	
	delete [] Buffer;
	
      } else if (FieldType.CaseEquals("gpoly")) {
	gp = fc.GetFieldStart() + GpOffset;
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);

	DOUBLE vertices[4];
	INT npairs = DocTypePtr->ParseGPoly(Buffer,vertices);

	if (npairs > 0) {
	  fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	  fwrite((char*)&npairs, 1, sizeof(INT), fp);
	  fwrite((char*)vertices, npairs*2, sizeof(DOUBLE), fp);
	}
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType << endl;
	cout << "gp=" << gp << ", " << npairs << " points," << endl;
	cout << "GPOLY = [(" 
	     << vertices[0] << "," 
	     << vertices[1] << ") ("
	     << vertices[2] << ","
	     << vertices[3] << ")]" << endl;
#endif
	delete [] Buffer;
	
      } else if (FieldType.CaseEquals("computed")) {
	gp = fc.GetFieldStart() + GpOffset;
	fLen = fc.GetFieldEnd() - fc.GetFieldStart() + 1;
	
	Buffer = new CHR [fLen+1];
	GetIndirectBuffer(gp,Buffer,0,fLen);
	//	Buffer[fLen] = '\0';
	
	fVal = DocTypePtr->ParseComputed(FieldName,Buffer);
	
	fwrite((char*)&gp, 1, sizeof(GPTYPE), fp);
	fwrite((char*)&fVal, 1, sizeof(DOUBLE), fp);
	
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "FieldType=" << FieldType << endl;
	cout << "gp=" << gp;
	cout << ", computed value=" << fVal;
	cout << endl;
#endif
	
	delete [] Buffer;
	
      } else {
	gp = fc.GetFieldStart() + GpOffset;
	Parent->GpFwrite(&gp, 1, sizeof(GPTYPE), fp);
	gp = fc.GetFieldEnd() + GpOffset;
	Parent->GpFwrite(&gp, 1, sizeof(GPTYPE), fp);
#ifdef DEBUG
	cout << "FieldName=" << FieldName << endl;
	cout << "No FieldType";
	cout << ", gp(start)=" << gp;
	cout << ", gp(end)=" << gp << endl;
#endif
      }
    } 
    
    Parent->ffclose(fp);
    
  }
}

/*
  void 
  INDEX::SetMergeStatus(GDT_BOOLEAN a)
  {
  MergeStatus=a;
  }
*/

void 
INDEX::AddRecordList(PFILE RecordListFp) 
{
  UINT4 DataMemorySize = (UINT4)(Parent->GetIndexingMemory() );

  // JMF Test

  //DataMemorySize=5000;
  
  //  UINT4 IndexMemorySize = (UINT4)((DataMemorySize / 3) * sizeof(GPTYPE));
  //  PGPTYPE MemoryIndex = new GPTYPE[(IndexMemorySize / sizeof(GPTYPE)) + 1];


  UINT4 IndexMemorySize = (UINT4)(DataMemorySize / 3); // jw patch
  PGPTYPE MemoryIndex = new GPTYPE[IndexMemorySize+1]; // jw patch
  
  MemoryData = new CHR[DataMemorySize];
  INT FirstRecord = 1;
  INT CurrentRecord;
  INT MemoryIndexLength;
  PFILE DataFp = (PFILE)NULL;
  RECORD record;
  STRING s, DataFileName, OldDataFileName;
  MDTREC mdtrec;
  UINT4 DataFileSize;
  INT GpListSize;
  INT Error;
  GPTYPE TrueGlobalStart = 0;
  GPTYPE TrueGlobalEnd = 0;
  GPTYPE OldGlobalStart=0;
  MDTREC lastmdtrec;
  INT j;
  CHR *p;
  CHR TempBuffer[80];
  STRING RecordFlag;
  STRING Doctype;
  GDT_BOOLEAN Break;
  INT rcount=0,didmod=0;

  Break = GDT_FALSE;
  do {
    CurrentRecord = FirstRecord;
    Error = 0;
    MemoryDataLength = 0;
    MemoryIndexLength = 0;
    //    MemoryData[0] = (CHR)NULL;
    MemoryData[0] = '\0';
    OldGlobalStart = Parent->GetMainMdt()->GetNextGlobal();

    do {
      didmod=1;
      if (!Break) {
	RecordFlag.FGet(RecordListFp, 3);
      }
      if ( (RecordFlag == "#") || (Break) ) {
	
	if (!Break) {
	  record.Read(RecordListFp);
	}
	Break = GDT_FALSE;
	record.GetFullFileName(&DataFileName);
	if (!DataFileName.Equals(OldDataFileName)) {
	  if (DataFp) {
	    fclose(DataFp);
	  }
	  DataFp = fopen(DataFileName, "rb");
	}
	if (!DataFp) {
	  // ER
	  fprintf(stderr,"   Skipping file ");
	  DataFileName.Print(stderr);
	  fprintf(stderr," ... (Error opening file)\n");
	  CurrentRecord++;
	} else {
	  if (record.GetRecordEnd() == 0) {
	    //	    fseek(DataFp, 0L, 2);
	    fseek(DataFp, 0L, SEEK_END);
	    DataFileSize = ftell(DataFp);
	  } else {
	    DataFileSize = record.GetRecordEnd() -
	      record.GetRecordStart() + 1;
	  }
	  
	  if (!DataFileName.Equals(OldDataFileName)) {
	    // New document file, so update global pointers

	    /*
	      if (MemoryDataLength) {	// <<<-------------
	      MemoryDataLength++;	// <<<-------------
	      }
	      */
	    Parent->IndexingStatus(IndexingStatusParsingDocument,
				   &DataFileName, 0);
	    TrueGlobalStart = Parent->GetMainMdt()->GetNextGlobal();
	    //	    fseek(DataFp, 0, 2);
	    fseek(DataFp, 0L, SEEK_END);
	    TrueGlobalEnd = TrueGlobalStart + ftell(DataFp) - 1;
	    
	  }
	  
	  if ( DataFileSize >= DataMemorySize ) {
	    fprintf(stderr,"One of the document records you are indexing ");
	    fprintf(stderr,"is too large for the amount\n");
	    fprintf(stderr,"of memory allocated by Iindex.  Use the `-m' ");
	    fprintf(stderr,"option to set a value\n");
	    fprintf(stderr,"greater than the largest document record you ");
	    fprintf(stderr,"are indexing.  For example,\n");
	    fprintf(stderr,"use `-m 2' if the largest document is 1.5 MB.\n");
	    EXIT_ERROR;
	  }
	  
	  if ( (DataFileSize + MemoryDataLength) >= DataMemorySize ) {
	    Break = GDT_TRUE;
	    break;
	  }
	  //	  fseek(DataFp, record.GetRecordStart(), 0); // core dump
	  fseek(DataFp, (long)record.GetRecordStart(), SEEK_SET); // core dump

#ifdef DEBUG
	  cout << "Reading " << DataFileSize << " bytes into MemoryData array";
	  cout << " (length=" << strlen(MemoryData);
	  cout << "), at offset " << MemoryDataLength << endl;
#endif

	  if (fread(MemoryData + MemoryDataLength, 1, DataFileSize,
		    DataFp) != (size_t)DataFileSize) {
	    perror(DataFileName);
	    EXIT_ERROR;
	  }
#if 0   
	  for (p = MemoryData + MemoryDataLength;
	       p < (MemoryData + MemoryDataLength + DataFileSize); p++) {
	    *p = tolower(*p);
	    //	    if (!isalnum(*p)) {
	    if (!IsAlnum(*p)) {
	      *p = ' ';
	    }
	  }
	  *p = '\0';			// Add a NULL to terminate the record
#else
	  Parent->ReplaceWithSpace(&record, 
				   MemoryData + MemoryDataLength, 
				   DataFileSize);
#endif
#ifdef VERBOSE
	  printf("   ...Parsing fields\n");
#endif
         
	  Parent->ParseFields(&record);
	  INT4 nRecordStart,nRecordEnd;

	  record.GetDocumentType(&s);
	  mdtrec.SetDocumentType(s);
	  record.GetPathName(&s);
	  mdtrec.SetPathName(s);
	  record.GetFileName(&s);
	  mdtrec.SetFileName(s);
	  nRecordStart = record.GetRecordStart();
	  nRecordEnd = record.GetRecordEnd();
	  mdtrec.SetLocalRecordStart(nRecordStart);

	  if ( (nRecordStart == 0) &&
	      (nRecordEnd == 0) ) {
	    mdtrec.SetLocalRecordEnd(DataFileSize - 1);
	  } else {
	    mdtrec.SetLocalRecordEnd(nRecordEnd);
	  }
	  
	  mdtrec.SetGlobalFileStart(TrueGlobalStart);
	  mdtrec.SetGlobalFileEnd(TrueGlobalEnd);
	  
	  record.GetKey(&s);
	  // Something that needs to be added somewhere:
	  // If record already contains a user-defined key,
	  // we need to make sure that it is unique!
	  if (s == "") {
	    sprintf(TempBuffer, "%d", 
		    mdtrec.GetGlobalFileStart() 
		    + mdtrec.GetLocalRecordStart());
	    s = TempBuffer;
	    Parent->GetMainMdt()->GetUniqueKey(&s);
	  }
	  mdtrec.SetKey(s);
	  Parent->IndexingStatus(IndexingStatusKeySet,
				 &s, 0);
	  

#ifdef DEBUG
	  STRING XXX;
	  mdtrec.GetKey(&XXX);
	  cout << "MDTrec key=" << XXX;
	  cout << endl;
	  cout << "MDTrec LocalRecordStart=" << mdtrec.GetLocalRecordStart();
	  cout << endl;
	  cout << "MDTrec LocalRecordEnd=" << mdtrec.GetLocalRecordEnd() ;
	  cout << endl;
	  cout << "MDTrec GlobalFileStart=" << mdtrec.GetGlobalFileStart();
	  cout << endl;
	  cout << "MDTrec GlobalFileEnd=" << mdtrec.GetGlobalFileEnd() ;
	  cout << endl;
#endif				/* DEBUG */
	  Parent->GetMainMdt()->AddEntry(mdtrec);
	  OldDataFileName = DataFileName;
	  CurrentRecord++;
	  
	  //#ifdef VERBOSE
	  //	  printf("   ...Parsing fields\n");
	  //#endif
	  //       	  Parent->ParseFields(&record);

       	  record.GetDocumentType(&Doctype); 

	  // BuildGpList has to be called after parsefields!
	  GpListSize = BuildGpList(Doctype, 
				   MemoryDataLength,
				   MemoryData,
				   MemoryDataLength + DataFileSize,
				   MemoryIndex + MemoryIndexLength,
				   IndexMemorySize - MemoryIndexLength);
	  if (GpListSize == -1) {
	    // ??
	    Break = GDT_TRUE;
	    break;
	  }

	  MemoryDataLength += DataFileSize;
	  //	  MemoryDataLength += DataFileSize+1;
	  
	  MemoryIndexLength += GpListSize;

#ifdef VERBOSE
	  printf("   ...Writing field data\n");
#endif
	  
	  WriteFieldData(record, mdtrec.GetGlobalFileStart() +
			 mdtrec.GetLocalRecordStart());
	}
	if (Break) {
	  break;
	}
      }
    } while (RecordFlag == "#");
    if (Error == 0) {
      Parent->IndexingStatus(IndexingStatusIndexing, 0,
			     MemoryIndexLength);
      qsort(MemoryIndex, MemoryIndexLength, sizeof(GPTYPE),
	    MemIndexCompare);
    //  Parent->IndexingStatus(IndexingStatusMerging, 0, 0);
      FlushIndexFiles(MemoryData, MemoryDataLength, MemoryIndex,
		 MemoryIndexLength, OldGlobalStart);

    }
    FirstRecord = CurrentRecord;
  } while (RecordFlag == "#");

  if (DataFp) {
    fclose(DataFp);
  }
  //  DumpIndex(0);
  
  delete [] MemoryData;
  delete [] MemoryIndex;
  
  // Now that we're done with the main index we need to sort the numeric
  // field tables.
  SortNumericFieldData();

  // now, do our *experimental* merge

  {
    FILE *fy=(FILE*)NULL;
    STRING CheckName;
    Parent->ComposeDbFn(&CheckName, ".num");
  
    fy=Parent->ffopen(CheckName,"w");
    fprintf(fy,"%d\n",IndexNum);
    Parent->ffclose(fy);

    CHR *oldpath,*newpath;
    struct stat info;
    if (IndexNum == 1) {
      // Rename the index file if there was only one chunk
      Parent->ComposeDbFn(&CheckName, ".inx");
      newpath = CheckName.NewCString();
      CheckName.Cat(".1");
      oldpath = CheckName.NewCString();
      rename(oldpath,newpath);
      delete [] oldpath;
      delete [] newpath;
    } else if (IndexNum > 1) {
      // And if we're appending, we may need to rename *.inx to *.inx.1
      Parent->ComposeDbFn(&CheckName, ".inx");
      oldpath = CheckName.NewCString();
      if (stat(oldpath, &info) ==0) {
	CheckName.Cat(".1");
	newpath = CheckName.NewCString();
	rename(oldpath,newpath);
	delete [] newpath;
      }
      delete [] oldpath;
    }

  }
  if(MergeStatus==GDT_TRUE)
    MergeIndexFiles(Parent->GetIndexingMemory());
  
}


void 
INDEX::MergeIndexFiles(INT MemMB)
{

  STRING TmpIndexFileName;
  CHR Tmp[256];
  INT i,j,k,CurrSmallest;
  STRING Current;
  GDT_BOOLEAN val;
  FILEMAP map(Parent);

  STRING CheckName;
  Parent->ComposeDbFn(&CheckName, ".num");
  FILE *fa=Parent->ffopen(CheckName,"r");
  if (!fgets(Tmp,256,fa)) {
    Parent->ffclose(fa);
    return;
  }
  IndexNum=atoi(Tmp);

  //  MERGEUNIT A[IndexNum];
  //  MERGEUNIT A[MAXINDEXNUM];
  MERGEUNIT *A;
  A = new MERGEUNIT[sizeof(MERGEUNIT)*IndexNum];

  Parent->ffclose(fa); 
#ifdef VERBOSE
  printf("%i Sub-Indexes to Merge\n", IndexNum);
#endif
//  Parent->IndexingStatus(IndexingStatusMerging, 0, 0);
  FILE *fj=fopen(IndexFileName,"w");
  
  INT MCount;
  
  MemMB/=IndexNum;
  MemMB/=(sizeof(GPTYPE)+sizeof(INT)+StringCompLength+sizeof(CHR)); //size of a sistring record
#ifdef VERBOSE
  printf("%i Optimizer Entries\n", MemMB);
#endif
  
  for(i=1; i<=IndexNum; i++){
    sprintf(Tmp,".%d",i);
    TmpIndexFileName=IndexFileName;
    TmpIndexFileName.Cat(Tmp);
    A[i-1].SetLoadLimit(MemMB);
    A[i-1].Initialize(TmpIndexFileName,Parent,&map,i-1);
//    A[i-1].Initialize(TmpIndexFileName,Parent,&map);
  }
  
  INT ActiveCount=0,ActiveItem=0;

  for(;;){
    ActiveCount=0;
    for(j=0; j<IndexNum; j++){	// count active items
      if(A[j].Empty()==GDT_FALSE){
	++ActiveCount;
	ActiveItem=j;
      }
    }
    if(ActiveCount==1){	// if only 1 is left, we are done.  Flush it.
      A[ActiveItem].Flush(fj);
      break;			// go do cleanup and close files
    }
    
    // find first active item of remaining several
    for(k=0; k<IndexNum; k++)
      if(A[k].Empty()==GDT_FALSE)
	break;
    // k is number of first active item
    A[k].GetSistring(&Current);
    CurrSmallest=k;
    for(++k;k<IndexNum; k++){	// loop through other active items
      if(A[k].Empty()==GDT_FALSE){
	val=A[k].Smallest(&Current); // if true, current was smaller
	if(val==GDT_FALSE)
	  CurrSmallest=k;
      }
      
    }
    // at this point, CurrSmallest is the one to write and reload
    
    A[CurrSmallest].Write(fj);
    
  }				// loop
  // clean up old files
  for(i=1; i<=IndexNum; i++){
    sprintf(Tmp,".%d",i);
    TmpIndexFileName=IndexFileName;
    TmpIndexFileName.Cat(Tmp);
    CHR *p=TmpIndexFileName.NewCString();
#ifdef VERBOSE
    printf("Deleting %s\n", p);
#endif
    unlink(p);
    delete [] p;
  }
  fclose(fj);
  
  StrUnlink(CheckName);
  delete [] A;
}				// end function


void 
INDEX::FlushIndexFiles(CHR *MemoryData, INT MemoryDataLength,
			    GPTYPE *NewMemoryIndex, INT MemoryIndexLength,
			    GPTYPE GlobalStart) 
{
  // Open index file
  // in this new implementation, we will never do an in-line merge
  
  STRING TmpIndexFileName=IndexFileName;
  CHR Tmp[256];
  IndexNum++;
  sprintf(Tmp,".%d",IndexNum);
  TmpIndexFileName.Cat(Tmp); // get the section file name
  
  //  PFILE fp = Parent->ffopen(TmpIndexFileName, "rb");
  PFILE fp=(PFILE)NULL;
  
  INT i;
  fp = Parent->ffopen(TmpIndexFileName, "wb");
  if (!fp) {
    perror(TmpIndexFileName);
    EXIT_ERROR;
  }
  // Dump out index
#ifdef VERBOSE
  printf("Adding GlobalStart %d\n", GlobalStart);
#endif
  for(i=0; i<MemoryIndexLength; i++){
    //  cout << NewMemoryIndex[i]<<" + "<<GlobalStart<<" = ";
    NewMemoryIndex[i]+=GlobalStart;
    //  cout << NewMemoryIndex[i]<<endl;
  }
  Parent->GpFwrite(NewMemoryIndex, 1, MemoryIndexLength*sizeof(GPTYPE), fp);
  Parent->ffclose(fp);
  
}


PFILE 
INDEX::GetFilePointer(const GPTYPE gp) const {
  INT x = Parent->GetMainMdt()->LookupByGp(gp);
  if (x) {
    STRING FileName;
    PFILE fp=(PFILE)NULL;
    MDTREC Mdtrec;
    Parent->GetMainMdt()->GetEntry(x, &Mdtrec);
    Mdtrec.GetFullFileName(&FileName);
    if (FileName.GetLength() > 0) {
      fp = Parent->ffopen(FileName, "rb");
      if (!fp) {
	perror(FileName);
	EXIT_ERROR;
      }
    } else {
      EXIT_ERROR;
    }

    //    fseek(fp, gp - Mdtrec.GetGlobalFileStart(), 0);
    fseek(fp, (long)(gp - Mdtrec.GetGlobalFileStart()), SEEK_SET);
    return fp;
  } else {
    return 0;
  }
}


INT 
INDEX::IsStopWord(CHR *WordStart, INT WordMaximum) const {
  return 0; // added for testing
  INT x = 0;
  INT WordLength = 0;
  while ( (WordLength < WordMaximum) &&
	 (IsAlnum(WordStart[WordLength])) ) {
    //	 (isalnum(WordStart[WordLength])) ) {
    WordLength++;
  }
  CHR SaveCh = WordStart[WordLength];
  WordStart[WordLength] = '\0';
  INT High = StopWordSize;
  INT Low = 0;
  INT Middle = 0;
  INT Old;
  do {
    Old = Middle;
    Middle = (Low + High) / 2;
    //    x = strcasecmp(WordStart, stoplist[Middle]);
    x = StrCaseCmp(WordStart, stoplist[Middle]);
    if (x == 0) {
      //not good to leave nuls embedded!
      WordStart[WordLength] = SaveCh; 
      return 1;
    }
    if (x < 0) {
      High = Middle;
    }
    if (x > 0) {
      Low = Middle;
    }
  } while (Middle != Old);
  WordStart[WordLength] = SaveCh;
  return 0;
}


GPTYPE 
INDEX::BuildGpList(
			  //@ManMemo: The associated doctype (for calling ParseWords())
			  const STRING& Doctype,
			  //@ManMemo: Index offset into the text buffer where the document starts.
			  INT StartingPosition,
			  //@ManMemo: Pointer to beginning of big text buffer.
			  CHR *MemoryData,
			  //@ManMemo: Length of big text buffer.
			  INT MemoryDataLength,
			  //@ManMemo Pointer to beginning of remaining GP index list buffer.
			  GPTYPE *MemoryIndex,
			  //@ManMemo: Length of GP index list buffer remaining.
			  INT MemoryIndexLength
			  ) 
{
  return ( Parent->ParseWords(Doctype, MemoryData + StartingPosition,
			      MemoryDataLength - StartingPosition,
			      StartingPosition, MemoryIndex,
			      MemoryIndexLength) );	// Convert parameters to what ParseWords() wants
}


GDT_BOOLEAN 
INDEX::DiskValidateInField(const GPTYPE HitGp, 
			   FILE *Fp, INT Total)
{
  
  INT Low = 0;
  INT High = Total - 1;
  INT X = High / 2;
  INT OX;
  GPTYPE GpS, GpE;
  do {
    OX = X;
    //    fseek(Fp, X * sizeof(GPTYPE) * 2, 0);
    fseek(Fp, (long)(X * sizeof(GPTYPE) * 2), SEEK_SET);
    Parent->GpFread(&GpS, 1, sizeof(GPTYPE), Fp);
    Parent->GpFread(&GpE, 1, sizeof(GPTYPE), Fp);
    if ( (HitGp >= GpS) && (HitGp <= GpE) ) {
      return GDT_TRUE;
    }
    
    if (HitGp < GpS) {
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
  } while (X != OX);
  
  return GDT_FALSE;
}


// JMF
GDT_BOOLEAN 
INDEX::ValidateInField(const GPTYPE HitGp, FILE *Fp, 
		       INT Total, INT Disk, 
		       GPTYPE *Cache, INT CacheSize, 
		       INT CacheBase) 
{
  
  // Hit Gps increase.  So, when are over 5% out of the cache  
  // in the upper direction, load a new cache
  
  INT Low = 0;
  INT High = Total - 1;
  INT X = High / 2;
  INT OX;
  GPTYPE GpS, GpE;
  INT Current=0,Pass=0;
  
  do {
    OX = X;
    
    if(Disk || X>=(CacheBase+CacheSize) || X<CacheBase ){
      
      //      fseek(Fp, X * sizeof(GPTYPE) * 2, 0);
      fseek(Fp, (long)(X * sizeof(GPTYPE) * 2), SEEK_SET);
      Parent->GpFread(&GpS, 1, sizeof(GPTYPE), Fp);
      Parent->GpFread(&GpE, 1, sizeof(GPTYPE), Fp);
      Current=0;
      
    } else {
      INT y=X*2;
      GpS=Cache[y-CacheBase];
      GpE=Cache[y+1-CacheBase];
      Current=1;
    }
    
    if ( (HitGp >= GpS) && (HitGp <= GpE) ) {
      if(Current==0) {
	++OutCache;
	if(Accesses>10) {
#ifdef DEBUG
	  printf("Slide Cache at X = %d\n",X);
#endif
	  CacheBase=X;
	  //	  fseek(Fp, X * sizeof(GPTYPE) * 2, 0);
	  fseek(Fp, (long)(X * sizeof(GPTYPE) * 2), SEEK_SET);
	  Parent->GpFread(Cache,sizeof(GPTYPE)*2,CacheSize,Fp);
	  Accesses=0;
	} else
	  ++Accesses;
      } else
	++InCache;
      return GDT_TRUE;
    }
    
    if (HitGp < GpS) {
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
  } while (X != OX);
  return GDT_FALSE;
}


PIRSET 
INDEX::RsetOr(const OPOBJ& Set1, const OPOBJ& Set2) const 
{
  return 0;
}


PIRSET 
INDEX::Search(const SQUERY& SearchQuery) 
{
  // Flip OPSTACK upside-down to convert so we can
  // pop from it in RPN order.
  OPSTACK Stack;
  SearchQuery.GetOpstack(&Stack);
  Stack.Reverse();
  // Pop OPOBJs, converting OPERANDs to result sets, and
  // executing OPERATORs
  OPSTACK TempStack;
  POPOBJ OpPtr;
  PIRSET NewIrset;
  INT Relation,Structure;
  ATTRLIST Attrlist;
  STRING Term, FieldName, S, FieldType, spTerm;
  INT TermWeight;
  POPOBJ Op1, Op2;

  MDT* pMDT;

  while (Stack >> OpPtr) {
    if (OpPtr->GetOpType() == TypeOperator) {
      TempStack >> Op1;
      TempStack >> Op2;
      if (OpPtr->GetOperatorType() == OperatorOr) {
	Op1->Or(*Op2);
	Stack << Op1;
      }
      if (OpPtr->GetOperatorType() == OperatorAnd) {
	Op1->And(*Op2);
	Stack << Op1;
      }
      if (OpPtr->GetOperatorType() == OperatorAndNot) {
#ifdef MULTI
        // Ugly Hack:
        // If using the MULTI version of AndNot, we need to
        // swap the order of the objects, so use the stack for a second
        TempStack << Op1;
        TempStack << Op2;// Op1 = Op2 might work here, too
        TempStack >> Op1;
        TempStack >> Op2;
#endif
	Op1->AndNot(*Op2);
	Stack << Op1;
      }
      // delete Op1;
      delete Op2;
    }
    if (OpPtr->GetOpType() == TypeOperand) {
      if (OpPtr->GetOperandType() == TypeRset) {
	TempStack << OpPtr;
      }
      if (OpPtr->GetOperandType() == TypeTerm) {
	OpPtr->GetTerm(&Term);
	spTerm = Term;
	spTerm.UpperCase();
	OpPtr->GetAttributes(&Attrlist);
 
	// check if the Local-Control-Identifier is enabled
        if (Attrlist.Lookup(GilsAttributeSet, ZdistUseAttr, 12)) {
	  // if so, treat the term as a key and return the document
          IRSET* NewIrset = new IRSET(Parent);
	  IRESULT Iresult;
	  SIZE_T N;
	  N = Parent->GetMainMdt()->LookupByKey(Term);
	  if (N > 0) {
	    Iresult.SetMdtIndex(Parent->GetMainMdt()->LookupByKey(Term));
	    Iresult.SetHitCount(1);
	    Iresult.SetScore(1);
	    Iresult.SetMdt(*(Parent->GetMainMdt()));
#ifdef DO_HIGHLIGHTING  
	    FCT Fct;
	    Iresult.SetHitTable(Fct);
#endif
	    NewIrset->AddEntry(Iresult, 1);
	  }
	  return NewIrset;
	}

	if (Attrlist.AttrGetRightTruncation()) {
	  Term += "*";
	}

	FieldName = "";            // Force it to initialize each time
	if (Attrlist.AttrGetFieldName(&S)) {
	  FieldName = S;
	} else {
	  FieldName = "";
	}

	FieldType = "text";        // Force it to initialize each time
	if (FieldName.GetLength() > 0) {
	  Parent->FieldTypes.GetValue(FieldName,&FieldType);
	  if(FieldType.GetLength() == 0) {
	    FieldType = "text";
	  }
	}
	
	// process for bounding rectangle
	STRINGINDEX x;
	INT i,tmp;
	CHR TBuf[256];
	DOUBLE N,So,E,W;
	DOUBLE fKey;

	if(FieldType.CaseEquals("gpoly")) {
	  Term.GetCString(TBuf,256);
	  tmp=strlen(TBuf);
	  for(i=0; i<tmp; i++) {
	    if(TBuf[i]==',')
	      TBuf[i]=' ';
	  }
	  // Old term order
	  //	  sscanf(TBuf,"%lf %lf %lf %lf",&N,&So,&E,&W);

	  // Now expect canonical term order
	  sscanf(TBuf,"%lf %lf %lf %lf",&N,&W,&So,&E);
	  NewIrset=BoundingRectangle(N,So,W,E);
	  
	} else if(FieldType.CaseEquals("bounding")) {
	  Term.GetCString(TBuf,256);
	  tmp=strlen(TBuf);
	  for(i=0; i<tmp; i++) {
	    if(TBuf[i]==',')
	      TBuf[i]=' ';
	  }
	  // We require a different order for bounding box - two corners
 	  //	  sscanf(TBuf,"%lf %lf %lf %lf",&N,&W,&So,&E);
	  // New canonical order
	  sscanf(TBuf,"%lf %lf %lf %lf",&N,&W,&So,&E);
	  NewIrset=BoundingRectangle(N,So,W,E);
         
	} else if(FieldType.CaseEquals("date")) {
	  if(Attrlist.AttrGetRelation(&Relation)==GDT_FALSE)
	    Relation=3;
	  if(Attrlist.AttrGetStructure(&Structure)==GDT_FALSE)
	    Structure=5;
	  NewIrset=DoDateSearch(Term,FieldName,Relation,Structure);
	  
        } else if(FieldType.CaseEquals("num")) {
          if(Attrlist.AttrGetRelation(&Relation)==GDT_FALSE)
            Relation=3;
          Term.GetCString(TBuf,256);
          fKey=atof(TBuf);
          NewIrset=NumericSearch(fKey,FieldName,Relation);
          
        } else if(FieldType.CaseEquals("computed")) {
          if(Attrlist.AttrGetRelation(&Relation)==GDT_FALSE)
            Relation=3;
          Term.GetCString(TBuf,256);
          fKey=atof(TBuf);
          NewIrset=NumericSearch(fKey,FieldName,Relation);
          
	} else if(FieldType.CaseEquals("date-range")) {
	  if(Attrlist.AttrGetRelation(&Relation)==GDT_FALSE)
	    Relation=3;
	  if(Attrlist.AttrGetStructure(&Structure)==GDT_FALSE)
	    Structure=5;
	  NewIrset=DoDateSearch(Term,FieldName,Relation,Structure);
	  
	  //	} else if((x=Term.Search("RECT{"))>0 || FieldName=="BOUNDING"){
	} else if((x=spTerm.Search("RECT{"))>0 || FieldName=="BOUNDING"){
	  if(FieldName!="BOUNDING"){
	    x+=5;
	    Term.EraseBefore(x);
	    x=Term.Search('}');
	    if(x)
	      Term.EraseAfter(x-1);
	  }
	  Term.GetCString(TBuf,256);
	  tmp=strlen(TBuf);
	  for(i=0; i<tmp; i++) {
	    if (TBuf[i]==',')
	      TBuf[i]=' ';
	  }
	  sscanf(TBuf,"%lf %lf %lf %lf",&N,&So,&W,&E);
	  NewIrset=BoundingRectangle(N,So,W,E);
	} else {
	  if(Attrlist.GetValue(Bib1AttributeSet,2,&Relation)==GDT_FALSE)
	    Relation=3;
	  NewIrset = TermSearch(Term, FieldName,Relation);
	}
	
	if (Attrlist.AttrGetTermWeight(&S)) {
	  TermWeight = S.GetInt();
	} else {
	  TermWeight = 1;
	}
	if (!NewIrset)
	  NewIrset = new IRSET(Parent);
	NewIrset->ComputeScores(TermWeight);
	TempStack << NewIrset;
	//delete NewIrset;
      }
    }
  }
  TempStack >> NewIrset;
  //  NewIrset->SortByScore();
  
  return NewIrset;
}


PIRSET 
INDEX::AndSearch(const SQUERY& SearchQuery) {
  // Convert all operators to ANDs
  OPSTACK Stack, TempStack, NewStack;
  POPOBJ OpPtr;
  SQUERY NewQuery;
  PIRSET TmpResult;
  SearchQuery.GetOpstack(&Stack);
  while (Stack >> OpPtr) {
    TempStack << *OpPtr;
    delete OpPtr;
  }
  while (TempStack >> OpPtr) {
    if (OpPtr->GetOpType() == TypeOperator) {
      OpPtr->SetOperatorType(OperatorAnd);
    }
    NewStack << *OpPtr;
    delete OpPtr;
  }
  NewQuery.SetOpstack(NewStack);
  TmpResult = Search(NewQuery);
  return(TmpResult);
}


//private
INT 
INDEX::GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer, 
			 const INT Offset,
			 const INT BufferLen) {
  MDTREC Mdtrec;
  STRING FileName;
  PFILE Fp=(PFILE)NULL;
  INT x;
  long FileOffset;

  Parent->GetMainMdt()->GetMdtRecord(Gp, &Mdtrec);
  if (Offset != 0) {
    GPTYPE FileStart = Mdtrec.GetGlobalFileStart();
    GPTYPE LocalGp = Gp - FileStart;
    LocalGp += Offset;
    GPTYPE LocalStart = Mdtrec.GetLocalRecordStart();
    GPTYPE LocalEnd = Mdtrec.GetLocalRecordEnd();
    if (LocalGp <= LocalStart || LocalGp >= LocalEnd)
      return(0);
  }
  Mdtrec.GetFullFileName(&FileName);

  // Make sure we get a file name
  if (FileName.GetLength() > 0) {
    // And make sure the file actually can be opened for read
    Fp = Parent->ffopen(FileName, "rb");
    if (!Fp) {
      perror(FileName);
      RETURN_ZERO;
    }
  } else {
    RETURN_ZERO;
  }

  // Calculate it explicitly so I can see it when debugging
  FileOffset = (long) Gp - Mdtrec.GetGlobalFileStart() + Offset;
  fseek(Fp, FileOffset, SEEK_SET);
  x = fread(Buffer, 1, BufferLen, Fp);
  Parent->ffclose(Fp);
  Buffer[x] = '\0';
  return(x);
}


GDT_BOOLEAN 
INDEX::GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer,
			 const INT Offset) {
  INT x;
  x=GetIndirectBuffer(Gp,Buffer,Offset,StringCompLength);
  if (x>0)
    return GDT_TRUE;
  return GDT_FALSE;
}


GDT_BOOLEAN 
INDEX::GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer) {
  INT x;
  x=GetIndirectBuffer(Gp,Buffer,0,StringCompLength);
  if (x>0)
    return GDT_TRUE;
  return GDT_FALSE;
}


PIRSET 
INDEX::SoundexSearch(const STRING& QueryTerm, const STRING& FieldName) { 
  // to do this efficiently, we need a soundex index
  // binary search
  PFILE fpi = fopen(IndexFileName, "rb");
  if (!fpi) {
    perror(IndexFileName);
    EXIT_ERROR;
  }
  GPTYPE gp;
  INT ip, oip, maxip, low, high;
  INT x, z;
  CHR Buffer[StringCompLength+1];
  CHR Term[StringCompLength+1];
  INT done = 0;
  //  fseek(fpi, 0, 2);
  fseek(fpi, 0L, SEEK_END);
  maxip = (ftell(fpi) / sizeof(GPTYPE)) - 1;
  high = maxip;
  ip = high / 2;
  low = 0;
  INT hit;
  z = 0;
  STRING s1, s2, sx1, sx2;
  Term[0] = toupper(QueryTerm.GetChr(1));
  Term[1] = '\0';
  do {
    hit = 0;
    oip = ip;
    //    fseek(fpi, ip * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(ip * sizeof(GPTYPE)), SEEK_SET);
    x = fread((char*)&gp, 1, sizeof(GPTYPE), fpi); // explicit cast
    if (x) {
      GetIndirectBuffer(gp, Buffer, 0);
      
      z = StrNCaseCmp(Term, Buffer, 1);
      /*
	 if (z == 0) {
	 if (isalnum(Buffer[strlen(Term)])) {
	 z = -1;
	 }
	 }
	 */
      
      if (z == 0) {
	done = 1;
	hit = 1;
      }
      if (z < 0) {
	high = ip;
      }
      if (z > 0) {
	low = ip + 1;
      }
      ip = (low + high) / 2;
      if (ip < 0) {
	ip = 0;
      }
      if (ip > maxip) {
	ip = maxip;
      }
    } else {
      ip = 0;
      done = 1;
    }
  } while ( (!done) && (ip != oip) );
  
  // find beginning
  INT first = ip;
  INT match = 1;
  while ( (first > 0) && (match) ) {
    first--;
    //    fseek(fpi, first * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
    x = fread((char*)&gp, 1, sizeof(GPTYPE), fpi); // explicit cast
    if (x) {
      GetIndirectBuffer(gp, Buffer, 0);
      if (toupper(Buffer[0]) != Term[0]) {
	match = 0;
      }
    } else {
      match = 0;
    }
  }
  
  IRESULT iresult;
  PIRSET pirset = new IRSET(Parent);
  INT w, OK;
  
  do {
    x = fread((char*)&gp, 1, sizeof(GPTYPE), fpi); // explicit cast
    if (x) {
      GetIndirectBuffer(gp, Buffer, 0);
      OK = 0;
      if (FieldName.Equals("")) {
	OK = 1;
      } else {
	/*if (ValidateInField(gp, FieldName)) {
	  OK = 1;
	  }*/
	OK=1;
      }
      if (OK) {
	s1 = Buffer;
	s2 = QueryTerm;
	SoundexEncode(s1, &sx1);
	SoundexEncode(s2, &sx2);
	if (sx1.Equals(sx2)) {
	  // match!
	  w = Parent->GetMainMdt()->LookupByGp(gp);
	  iresult.SetMdtIndex(w);
	  iresult.SetHitCount(1);
	  iresult.SetScore(0);
	  iresult.SetMdt(*(Parent->GetMainMdt()));
	  pirset->AddEntry(iresult, 1);
	}
      }
    }
  } while (toupper(Buffer[0]) == Term[0]);
  fclose(fpi);
  return pirset;
}


INT 
INDEX::Match(const CHR *QueryTerm, const INT TermLength, 
	     const GPTYPE gp, const INT4 Offset) {
  CHR Buffer[StringCompLength+1];
  INT z;
  
  if (!GetIndirectBuffer(gp, Buffer, Offset))
    return -1;

  BufferClean(Buffer);

#ifdef DEBUG
  cout << "Comparing term " << QueryTerm << " with string ["
       << gp << "] >>" << Buffer;
#endif

  if ( QueryTerm[TermLength - 1] == '*' ) 
    z = StrNCaseCmp(QueryTerm, Buffer, TermLength - 1);
  else {
    z = StrNCaseCmp(QueryTerm, Buffer, TermLength);
    //    if ( z == 0 && isalnum(Buffer[TermLength]) )
    if ( z == 0 && IsAlnum(Buffer[TermLength]) )
      z = -1;
  }

  //  cout << " returning " << z << endl;
  return z;
}


// relations: 3 equals, 1 less than, 2 less than/equals, 5 greater than
// 4 greater than or equals,  6 not equals
PIRSET 
INDEX::TermSearch(DOUBLE QueryTerm, const STRING& FieldName)
{
  return(NumericSearch(QueryTerm,FieldName,3));
}


PIRSET 
INDEX::TermSearch(DOUBLE QueryTerm, const STRING& FieldName, INT4 Relation)
{
  return(NumericSearch(QueryTerm,FieldName,Relation));
}


PIRSET 
INDEX::TermSearch(const STRING& QueryTerm, const STRING& FieldName)
{
  return(TermSearch(QueryTerm, FieldName,3)); // default EQUALS
  
  //  return(BoundingRectangle(50.0,-50.0,-80.0,-50.0));
  
}


int 
gpcomp(const void* x, const void* y) {
  return(*((GPTYPE *)x)-*((GPTYPE *)y));
}


PIRSET 
INDEX::TermSearch(const STRING& QueryTerm, const STRING& FieldName, 
		  INT4 Relation) 
{				// binary search
  
  STRING FieldType, CheckName;
  INT w;
  FILE *fx=(FILE*)NULL;
  
  Parent->ComposeDbFn(&CheckName, ".num");
  
  fx=Parent->ffopen(CheckName,"r");
  if(fx){
    Parent->ffclose(fx);
    return(MultiTermSearch(QueryTerm, FieldName, Relation));
  }
  
  Parent->FieldTypes.GetValue(FieldName,&FieldType);
  if (FieldType.GetLength() == 0)
    FieldType = "TEXT";
  if(FieldType!="TEXT"){
    DOUBLE fKey;
    CHR TmpBuf[256];
    QueryTerm.GetCString(TmpBuf,256);
    fKey=atof(TmpBuf);
    return(NumericSearch(fKey,FieldName,Relation));
    
  }
  
  PFILE fpi = Parent->ffopen(IndexFileName, "rb");
  if (!fpi) {
    perror(IndexFileName);
    EXIT_ERROR;
  }
  GPTYPE gp;
  INT ip, oip, maxip, low, high;
  INT x, z, TermLength, OrigTermLength;
  CHR OrigTerm[StringCompLength+1], *Term;
  //  INT x, z;
  //  CHR Buffer[StringCompLength+1];
  //  CHR Term[StringCompLength+1];
  INT done = 0;
  //  fseek(fpi, 0, 2);
  fseek(fpi, 0L, SEEK_END);
  maxip = (ftell(fpi) / sizeof(GPTYPE)) - 1;
  high = maxip;
  ip = high / 2;
  low = 0;
  INT hit;
  z = 0;
  
  QueryTerm.GetCString(OrigTerm, sizeof(OrigTerm));
  OrigTermLength = QueryTerm.GetLength();
  
  //because of sorting unpleasantness, 
  //we must convert non alnums in phrases to spaces
  //for phrase searches we need to look past 
  //all stop words, and start with the first
  //indexed word. later we will check backwords in the data.
  INT PhraseEnd = OrigTermLength;
  INT n, PhraseBeg = 0, FoundBeg=0;
  if (OrigTerm[OrigTermLength - 1] == '*')
    PhraseEnd--;
  for (n=0; n < PhraseEnd; n++) {
    if (!IsAlnum(OrigTerm[n])) {
      //    if (!isalnum(OrigTerm[n])) {
      OrigTerm[n] = ' ';
      if (!FoundBeg && IsStopWord(OrigTerm+PhraseBeg, n - PhraseBeg)) 
	PhraseBeg = n + 1;
      else
	FoundBeg = 1;
    }
  }
  
  if (PhraseBeg >= OrigTermLength) {
    //its all stop words. return an empty IRSET.
    PIRSET pirset = new IRSET(Parent);
    return pirset;
  }
  Term = OrigTerm + PhraseBeg; 
  TermLength = OrigTermLength - PhraseBeg;
  
  do {
    hit = 0;
    oip = ip;
    //    fseek(fpi, ip * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(ip * sizeof(GPTYPE)), SEEK_SET);
    x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
    if (x) {
      z = Match(Term, TermLength, gp);
      if (z == 0) {
	done = 1;
	hit = 1;
      }
      else if (z < 0) {
	//	high = ip;
	high = ip-1;
      }
      else if (z > 0) {
	low = ip + 1;
      }
      ip = (low + high) / 2;
      if (ip < 0) {
	ip = 0;
      }
      if (ip > maxip) {
	ip = maxip;
      }
    } else {
      ip = 0;
      done = 1;
    }
    //  } while ( (!done) && (ip != oip) );
  } while ( (!done) && (high >= low) );
  
  
  if (!hit) {
    // no hits - return an empty irset
    PIRSET pirset = new IRSET(Parent);
    return pirset;
  }
  
  // bracket hits
  INT first, last;
  INT match, nomatch;
  
  // find first
  low = 0;
  high = ip;
  first = high / 2;
  match = ip;
  nomatch = 0;
  do {
    //    fseek(fpi, first * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
    x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
    if (x) 
      z = Match(Term, TermLength, gp);
    if (z == 0) {
      match = first;
      high = first;
    } else {
      nomatch = first;
      low = first + 1;
    }
    first = (low + high) / 2;
    if (first < 0) {
      first = 0;
    } else {
      if (first > ip) {
	first = ip;
      }
    }
  } while ( (match - nomatch) > 5 );
  first = match;
  do {
    if (first > 0) {
      first--;
    }
    //    fseek(fpi, first * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
    x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
    if (x) 
      z = Match(Term, TermLength, gp);
  } while ( (z == 0) && (first > 0) );
  if ( (z != 0) || (first > 0) ) {
    first++;
  }
  
  
  // find last
  low = ip;
  high = maxip;
  last = (high + low) / 2;
  match = ip;
  nomatch = maxip;
  do {
    //    fseek(fpi, last * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(last * sizeof(GPTYPE)), SEEK_SET);
    x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
    if (x) 
      z = Match(Term, TermLength, gp);
    if (z == 0) {
      match = last;
      low = last + 1;
    } else {
      nomatch = last;
      high = last;
    }
    last = (low + high) / 2;
    if (last < ip) {
      last = ip;
    } else {
      if (last > maxip) {
	last = maxip;
      }
    }
  } while ( (nomatch - match) > 5 );
  last = match;
  do {
    if (last < maxip) {
      last++;
    }
    //    fseek(fpi, last * sizeof(GPTYPE), 0);
    fseek(fpi, (long)(last * sizeof(GPTYPE)), SEEK_SET);
    x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
    if (x) 
      z = Match(Term, TermLength, gp);
  } while ( (z == 0) && (last < maxip) );
  if ( (z != 0) || (last < maxip) ) {
    last--;
  }
  
  //	first++;
  //	last--;
  
  // Build result set
  IRESULT iresult;
  MDT* ThisMdt;
  MDTREC mdtrec;
  GPTYPE GlobalRecEnd;
  PIRSET pirset = new IRSET(Parent);
  PGPTYPE gplist = new GPTYPE[last-first+1];
  //  INT w;
  INT OK;
  PFCT Pfct;
  FC Fc;
  //  fseek(fpi, first * sizeof(GPTYPE), 0);
  fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
  x = Parent->GpFread(gplist, 1, 
		      (last-first+1) * sizeof(GPTYPE), fpi) / sizeof(GPTYPE);
  fclose(fpi);
  
  // sort gplist
  qsort(gplist, (last-first)+1,sizeof(GPTYPE),gpcomp);
  pirset->Resize(pirset->GetTotalEntries() + x); // resize to ahead of time
  INT Offset = TermLength - OrigTermLength;
  
  INT TermLenNoStar;
  
  if (QueryTerm.GetChr(OrigTermLength) == '*') {
    TermLenNoStar = OrigTermLength - 1; // ignore "*" at end
  } else {
    TermLenNoStar = OrigTermLength;
  }
  
  Pfct = new FCT();
  INT CheckField=1;
  INT Total=0;
  INT Disk=0;
  INT CacheSize=0;
  GPTYPE *Cache=(GPTYPE*)NULL;
  FILE *fpf=(FILE*)NULL;
  
  if (FieldName.Equals("") || FieldName.GetLength()==0) {
    CheckField=0;
    Total=0;
  } else {
    STRING Fn;
    CheckField=1;
    Parent->DfdtGetFileName(FieldName, &Fn);
    fpf = Parent->ffopen(Fn, "rb");
    InCache=OutCache=Accesses=0;
    
    if (fpf) {
      //      fseek(fpf, 0, 2);
      fseek(fpf, 0L, SEEK_END);
      Total = ftell(fpf) / ( sizeof(GPTYPE) * 2 );
      rewind(fpf);
      CacheSize=CACHELIMIT;
      Cache=new GPTYPE[CacheSize*2];
      Parent->GpFread(Cache,sizeof(GPTYPE),CacheSize*2,fpf);
      Disk=0;
    } else {
      // field file not found - return an empty irset
      fprintf(stderr,"Field ");
      FieldName.Print(stderr);
      fprintf(stderr," not present in this index.\n"); 
      return pirset;
    }
  }
  
  for (ip=0; ip<x; ip++) {
    OK = 0;
    
    // Here's Jim's new code with rcache
    for (ip=0; ip<x; ip++) {
      OK = 0;
      if (TermLength != OrigTermLength) {
	if ( (Match(OrigTerm, OrigTermLength, gplist[ip], Offset)) == 0)  
	  gplist[ip] += Offset;
	else
	  continue;
      }
      
      if(CheckField==1) {
	if (Disk==1) {
	  if (DiskValidateInField(gplist[ip], fpf, Total)) {
	    OK=1;
	  }
	} else if (ValidateInField(gplist[ip], fpf, Total, Disk, 
				   Cache, CacheSize,0)) { 
	  OK = 1;
	}
      } else
	OK = 1;
      if (OK) {
	//      w = Parent->GetMainMdt()->LookupByGp(gplist[ip]);
	//make sure that phrases dont go past 
	//the end of the local record.
        ThisMdt = Parent->GetMainMdt();
	w = Parent->GetMainMdt()->GetMdtRecord(gplist[ip], &mdtrec);
	GlobalRecEnd = mdtrec.GetGlobalFileStart() + 
	  mdtrec.GetLocalRecordEnd();
	if  ( !((GlobalRecEnd - gplist[ip]) >= (TermLenNoStar - 1)) )
	  continue;
	// Skip deleted records
	if (mdtrec.GetDeleted() == GDT_TRUE)
	  continue;
	iresult.SetMdtIndex(w);
	iresult.SetHitCount(1);
	iresult.SetScore(0);
	iresult.SetMdt(*ThisMdt);
	Fc.SetFieldStart(gplist[ip]);
	//      Fc.SetFieldEnd(gplist[ip] + QueryLength - 1);
	Fc.SetFieldEnd(gplist[ip] + TermLength - 1);
	Pfct->Clear();
	Pfct->AddEntry(Fc);
#ifdef DO_HIGHLIGHTING  
	iresult.SetHitTable(*Pfct);
#endif
	pirset->FastAddEntry(iresult, 1);
      }
    }
    if(CheckField==1 && fpf!=NULL){
      //  printf("%d Accesses, %d InCache, %d OutCache (%f Efficiency)\n",
      //   Accesses,InCache,OutCache,(InCache/Accesses)*100);
      fclose(fpf);
    }
  }
  delete Pfct;
  if(CacheSize>0)
    delete Cache;
  delete [] gplist;
  pirset->SortByIndex();
  pirset->MergeEntries(1);
  return pirset;
}


void 
INDEX::DumpIndex(INT DebugSkip) {
  STRING CheckName,TmpIndexFileName;
  FILE *fx=(FILE*)NULL;
  INT kk;
  CHR buf[256];
  PFILE fpd=(PFILE)NULL;
  GPTYPE gp;
  MDTREC mdtrec;
  INT x, y, j;
  CHR Buffer[StringCompLength+1], Term[StringCompLength+1];
  STRING FileName;

  Parent->ComposeDbFn(&CheckName, ".num");
  
  fx = Parent->ffopen(CheckName,"r");
  if (fx) {
    if (!fgets(buf,256,fx)) {
      Parent->ffclose(fx);
      IndexNum=1;
    } else {
      Parent->ffclose(fx);
      IndexNum=atoi(buf);
    }
  } else
    IndexNum=1;

  for(kk=1; kk<=IndexNum; kk++) {

    printf("\nDumping chunk %i\n\n", kk);

    TmpIndexFileName=IndexFileName;
    if (IndexNum > 1) {
      sprintf(buf,".%d",kk);
      TmpIndexFileName.Cat(buf);
    }
    PFILE fpi = Parent->ffopen(TmpIndexFileName, "rb");
    if (!fpi) {
      perror(TmpIndexFileName);
      EXIT_ERROR;
    }
    Term[0] = '\0';
  
    if (DebugSkip > 0) {
      fseek(fpi, (long)(sizeof(GPTYPE)*DebugSkip), SEEK_SET);
      printf("Skipping %i SIStrings.\n", DebugSkip);
    }
  
    y = 0;
    while (Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi) > 0) {
      Parent->GetMainMdt()->GetMdtRecord(gp, &mdtrec);
      mdtrec.GetFullFileName(&FileName);
      if (FileName.GetLength() > 0) {
	fpd = fopen(FileName, "rb");
	if (!fpd) {
	  perror(FileName);
	  EXIT_ERROR;
	}
      } else {
	EXIT_ERROR;
      }
      printf("SIString#%i\t", DebugSkip+y);
      y++;
      //      fseek(fpd, gp - mdtrec.GetGlobalFileStart(), 0);
      fseek(fpd, (long)(gp - mdtrec.GetGlobalFileStart()), SEEK_SET);
      x = fread(Buffer, 1, StringCompLength, fpd);
      fclose(fpd);
    
      // Wipe the rest of the buffer clean
      for (j=x; j<StringCompLength; j++) {
	Buffer[j] = ' ';
      }
    
      BufferClean(Buffer);

      //star if the current entry is out of order.
      if( (StrNCaseCmp(Term, Buffer, StringCompLength)) > 0)
	printf("(*)");
    
      // store current term for comparison next time
      memcpy(Term, Buffer, StringCompLength);
    
      FileName.Print();
      printf(":%i:", gp);
      printf("%i\n", gp - mdtrec.GetGlobalFileStart());
      
      Buffer[x] = '\0';
      printf("-->%s<--\n\n", Buffer);
    }
    fclose(fpi);
  }
}


void 
INDEX::WriteCentroid(FILE* fp) 
{

  fprintf(fp, "  <centroid>\n");
	
  STRING CheckName,TmpIndexFileName;
  FILE *fx=(FILE*)NULL;
  INT kk;
  CHR buf[256];
  PFILE fpd=(PFILE)NULL;
  GPTYPE gp;
  MDTREC mdtrec;
  INT x, y, j;
  CHR Buffer[StringCompLength+1], Term[StringCompLength+1];
  STRING FileName;
	
  Parent->ComposeDbFn(&CheckName, ".num");
	
  fx = Parent->ffopen(CheckName,"r");
  if (fx) {
    if (!fgets(buf,256,fx)) {
      Parent->ffclose(fx);
      IndexNum=1;
    } else {
      Parent->ffclose(fx);
      IndexNum=atoi(buf);
    }
  } else
    IndexNum=1;

  char lastWord[255];
  lastWord[0] = '\0';
  int count = 0;
	
  for(kk=1; kk<=IndexNum; kk++) {
		
    TmpIndexFileName=IndexFileName;
    if (IndexNum > 1) {
      sprintf(buf,".%d",kk);
      TmpIndexFileName.Cat(buf);
    }

    PFILE fpi = Parent->ffopen(TmpIndexFileName, "rb");
    if (!fpi) {
      perror(TmpIndexFileName);
      EXIT_ERROR;
    }

    Term[0] = '\0';
    y = 0;
    while (Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi) > 0) {
      Parent->GetMainMdt()->GetMdtRecord(gp, &mdtrec);
      mdtrec.GetFullFileName(&FileName);
      if (FileName.GetLength() > 0) {
	fpd = fopen(FileName, "rb");
	if (!fpd) {
	  perror(FileName);
	  EXIT_ERROR;
	}
      } else {
	EXIT_ERROR;
      }

      y++;
      fseek(fpd, (long)(gp - mdtrec.GetGlobalFileStart()), SEEK_SET);
      x = fread(Buffer, 1, StringCompLength, fpd);
      fclose(fpd);
			
      // Wipe the rest of the buffer clean
      for (j=x; j<StringCompLength; j++) {
	Buffer[j] = ' ';
      }
			
      // convert all non-alphas to spaces.  If we add phrase
      // searching, this should be eliminated.
      /*
	for (j=0; j<StringCompLength; j++) {
	if (!isalnum(Buffer[j])) {
	Buffer[j] = ' ';
	}
	}
      */
      BufferClean(Buffer);
			
      //star if the current entry is out of order.
      if( (StrNCaseCmp(Term, Buffer, StringCompLength)) > 0) {
	//printf("(*)");
      }
			
      // store current term for comparison next time
      memcpy(Term, Buffer, StringCompLength);
			
      char* p = Buffer;
      while ( ! isspace(*p) ) {
	*p = tolower(*p);
	p++;
      }
      *p = '\0';
      if (strcmp(Buffer, lastWord) == 0) {
	count++;
      } else {
	if (lastWord[0] != '\0') {
	  // output word and frequency
	  fprintf(fp, "    <word freq=\"%i\">%s</word>\n",
		  count, lastWord);
	}
	count = 1;
	strcpy(lastWord, Buffer);
      }
    }
    fclose(fpi);
  }
  // output word and frequency
  fprintf(fp, "    <word freq=\"%i\">%s</word>\n",
	  count, lastWord);

  // Once the word centroid is written, dump out the numeric centroids
  // The easiest way is to walk through the DFD file, get the field
  // names and field types, then open the file containing the values
  INT xx,FieldCount,FieldExt;
  STRING FieldName,FieldType;
  FILE *fy;
  CHR *fname,*ftype;

  Parent->ComposeDbFn(&CheckName, ".dfd");
  fx = Parent->ffopen(CheckName,"r");
  if (fx) {
    if (!fgets(buf,256,fx)) {
      Parent->ffclose(fx);
      return;
    }
    FieldCount=atoi(buf);
  } else {
    return;
  }

  for (xx=0;xx<FieldCount;xx++) {
    // Read 8 lines per entry
    if (!fgets(buf,256,fx)) break; // Field number is first
    FieldExt = atoi(buf);
    if (!fgets(buf,256,fx)) break;
    if (!fgets(buf,256,fx)) break;
    if (!fgets(buf,256,fx)) break;
    if (!fgets(buf,256,fx)) break; // Field Name
    buf[strlen(buf)-1] = '\0';
    FieldName = buf;
    if (!fgets(buf,256,fx)) break;
    if (!fgets(buf,256,fx)) break;
    if (!fgets(buf,256,fx)) break; // Field type
    buf[strlen(buf)-1] = '\0';
    FieldType = buf;
    fname = FieldName.NewCString();
    ftype = FieldType.NewCString();
    // Skip the separate nested fields - just take the basic ones
    if (!strstr(fname,"METADATA_")) {
      if ((FieldType.Equals("NUM")) 
	  || (FieldType.Equals("COMPUTED"))) {
	DOUBLE MaxVal,MinVal;
	NUMERICLIST NumList;

	sprintf(buf,".%03d",FieldExt);
	Parent->ComposeDbFn(&CheckName, buf);
	NumList.SetFileName(CheckName);
	NumList.LoadTable(0,-1,VAL_BLOCK);
	INT4 Count = NumList.GetCount();

	// For this centroid, it makes no sense to try to build a
	// histogram, since we have no idea what kind of bins or 
	// spacing to create.  Maybe someday we will get clever.
	MaxVal = NumList.GetMaxValue();
	MinVal = NumList.GetMinValue();
	// output word and frequency
	fprintf(fp, "    <field name =\"%s\" type=\"%s\">\n",
		fname, ftype);
	fprintf(fp, "      <maximum>%.2f</maximum>\n",
		MaxVal);
	fprintf(fp, "      <minimum>%.2f</minimum>\n",
		MinVal);
	fprintf(fp, "    </field>\n");

      } else if (FieldType.Equals("DATE")) {

	DOUBLE MaxVal,MinVal;
	INT iMaxVal,iMinVal;
	SRCH_DATE DateMaxVal, DateMinVal;
	INTERVALLIST IntList;

	// Set up arrays to hold the dates, date ranges and spatial 
	// centroids. The centroids are really histograms, so we need to 
	// track counts.  For dates, we will start at 1800 and come forward
	// 250 years, to 2050.  If this leads to a Y2050 problem, it will
	// be because some fool is still using this software in 2050.
	
	INT StartYear=1800;
	INT HistLength = 250;

#if defined(WIN32) || defined (SGI_CC)
	INT YearHist[250];
#else
	INT YearHist[HistLength];
#endif
	INT i;
	for (i=0;i<HistLength;i++)
	  YearHist[i] = 0;

	sprintf(buf,".%03d",FieldExt);
	Parent->ComposeDbFn(&CheckName, buf);
	IntList.SetFileName(CheckName);
	IntList.LoadTable(0,-1,START_BLOCK);

	INT4 Count = IntList.GetCount();
	for(INT4 x=0; x<Count; x++) {
	  MinVal = IntList.GetStartValue(x);
	  MaxVal = IntList.GetEndValue(x);
	  DateMinVal = MinVal;
	  DateMinVal.TrimToYear();
	  DateMaxVal = MaxVal;
	  DateMaxVal.TrimToYear();
	  iMinVal = (INT)(DateMinVal.GetValue() - StartYear);
	  if (iMinVal >= 0) {
	    iMaxVal = (INT)(DateMaxVal.GetValue() - StartYear);
	    if (iMaxVal > (INT)(StartYear+HistLength))
	      iMaxVal = (INT)(StartYear+HistLength);
	    for (i=iMinVal;i<=iMaxVal;i++)
	      YearHist[i]++;
	  }
	}

	fprintf(fp, "    <field name =\"%s\" type=\"%s\">\n",
		fname, ftype);
	for (i=0;i<HistLength;i++)
	  if (YearHist[i] > 0)
	    fprintf(fp, "      <year freq=\"%d\">%d</year>\n",
		    YearHist[i],i+(INT)StartYear);
	fprintf(fp, "    </field>\n");

      } else if (FieldType.Equals("DATE-RANGE")) {
	DOUBLE MaxVal,MinVal;
	INT iMaxVal,iMinVal;
	SRCH_DATE DateMaxVal, DateMinVal;
	INTERVALLIST IntList;

	// Set up arrays to hold the dates, date ranges and spatial 
	// centroids. The centroids are really histograms, so we need to 
	// track counts.  For dates, we will start at 1800 and come forward
	// 250 years, to 2050.  If this leads to a Y2050 problem, it will
	// be because some fool is still using this software in 2050.
	
	INT StartYear=1800;
	INT HistLength = 250;

#if defined(WIN32) || defined (SGI_CC)
	INT YearHist[250];
#else
	INT YearHist[HistLength];
#endif
	INT i;
	for (i=0;i<HistLength;i++)
	  YearHist[i] = 0;

	sprintf(buf,".%03d",FieldExt);
	Parent->ComposeDbFn(&CheckName, buf);
	IntList.SetFileName(CheckName);
	IntList.LoadTable(0,-1,START_BLOCK);

	INT4 Count = IntList.GetCount();
	for(INT4 x=0; x<Count; x++) {
	  MinVal = IntList.GetStartValue(x);
	  MaxVal = IntList.GetEndValue(x);
	  DateMinVal = MinVal;
	  DateMinVal.TrimToYear();
	  DateMaxVal = MaxVal;
	  DateMaxVal.TrimToYear();
	  iMinVal = (INT)(DateMinVal.GetValue() - StartYear);
	  if (iMinVal >= 0) {
	    iMaxVal = (INT)(DateMaxVal.GetValue() - StartYear);
	    if (iMaxVal > (INT)(StartYear+HistLength))
	      iMaxVal = (INT)(StartYear+HistLength);
	    for (i=iMinVal;i<=iMaxVal;i++)
	      YearHist[i]++;
	  }
	}

	fprintf(fp, "    <field name =\"%s\" type=\"%s\">\n",
		fname, ftype);
	for (i=0;i<HistLength;i++)
	  if (YearHist[i] > 0)
	    fprintf(fp, "      <year freq=\"%d\">%d</year>\n",
		    YearHist[i],i+(INT)StartYear);
	fprintf(fp, "    </field>\n");

      } else if (FieldType.Equals("RANGE")) {
	DOUBLE MaxVal,MinVal;
	INTERVALLIST IntList;

	sprintf(buf,".%03d",FieldExt);
	Parent->ComposeDbFn(&CheckName, buf);
	IntList.SetFileName(CheckName);
	IntList.LoadTable(0,-1,START_BLOCK);

	INT4 Count = IntList.GetCount();
	MinVal = IntList.GetStartMinValue();
	MaxVal = IntList.GetEndMaxValue();

	// output word and frequency
	fprintf(fp, "    <field name =\"%s\" type=\"%s\">\n",
		fname, ftype);
	fprintf(fp, "      <maximum %.2f</maximum>\n",
		MaxVal);
	fprintf(fp, "      <minimum >%.2f</minimum>\n",
		MinVal);
	fprintf(fp, "    </field>\n");

      } else if (FieldType.Equals("GPOLY")) {
	GPTYPE GpS;
	sprintf(buf,".%03d",FieldExt);
	Parent->ComposeDbFn(&CheckName, buf);
	PFILE Fp = Parent->ffopen(CheckName, "rb");

	if (!Fp) {
	  perror(CheckName);
	  EXIT_ERROR;
	}

	INT Lat;
	INT Histogram[360][180];
	for (Lat=-90;Lat<90;Lat++) {
	  for (INT Lon=-180;Lon<180;Lon++) {
	    Histogram[Lon+180][Lat+90] = 0;
	  }
	}

	INT iNorth,iSouth,iEast,iWest;

	DOUBLE Vertices[4];
	INT npts;
	while (!feof(Fp)) {
	  Parent->GpFread(&GpS, 1, sizeof(GPTYPE), Fp);
	  if (fread(&npts,1,sizeof(INT), Fp) != sizeof(INT)) {
	    break;
	  }
	  if (fread(Vertices, sizeof(DOUBLE), 4, Fp) != 4) {
	    break;
	  }

	  iWest  = (INT)Vertices[0];
	  iNorth = (INT)Vertices[1];
	  iEast  = (INT)Vertices[2];
	  iSouth = (INT)Vertices[3];

	  for (Lat=iSouth;Lat<iNorth;Lat++) {
	    for (INT Lon=iWest;Lon<iEast;Lon++) {
	      Histogram[Lon+180][Lat+90]++;
	    }
	  }
	}

	fprintf(fp, "    <field name =\"%s\" type=\"%s\">\n",
		fname, ftype);
	for (Lat=iSouth;Lat<iNorth;Lat++) {
	  for (INT Lon=iWest;Lon<iEast;Lon++) {
	    fprintf(fp, "      <coordinate_bin freq=\"%d\">\n",Histogram[Lon+180][Lat+90]);
	    fprintf(fp, "        <longitude>%d</longitide>\n",Lon+180);
	    fprintf(fp, "        <latitude>%d</latitude>\n",Lat+90);
	    fprintf(fp, "      </coordinate_bin>\n");
	  }
	}
	fprintf(fp, "    </field>\n");
	//      cout << "Field #" << FieldExt << " is " << FieldName 
	// << ", type " 
	//	   << FieldType << endl;
      }
    }
    delete [] fname;
    delete [] ftype;
  }
  
  Parent->ffclose(fx);

  // Close off the centroid box
  fprintf(fp, "  </centroid>\n");

}


void 
INDEX::CollapseIndexFiles(INT MemMB)
{
  
  STRING TmpIndexFileName,OutFile;
  CHR Tmp[256];
  INT i,j,k,CurrSmallest,LocalIndexNum,First,Second;
  STRING Current;
  
  GDT_BOOLEAN val;
  FILEMAP map(Parent);
  
  STRING CheckName;
  Parent->ComposeDbFn(&CheckName, ".num");
  FILE *fa=Parent->ffopen(CheckName,"r");
  if (!fgets(Tmp,256,fa)) {
    Parent->ffclose(fa);
    return;
  }
  LocalIndexNum=atoi(Tmp);
  First=LocalIndexNum-2;
  Second=LocalIndexNum-1;
  
  
  MERGEUNIT A[2];
  
  Parent->ffclose(fa); 
  printf("Collapsing Final Sub-Indexes\n");
  // Parent->IndexingStatus(IndexingStatusMerging, 0, 0);
  OutFile=IndexFileName;
  OutFile.Cat(".tmp");
  FILE *fj=fopen(OutFile,"w");
  
  INT MCount;
  
  MemMB/=2;
  MemMB/=(sizeof(GPTYPE)+sizeof(INT)+StringCompLength+sizeof(CHR)); //size of a sistring record
  printf("%i Optimizer Entries\n", MemMB);
  for(i=First; i<=Second; i++){
    sprintf(Tmp,".%d",i);
    TmpIndexFileName=IndexFileName;
    TmpIndexFileName.Cat(Tmp);
     A[i-First].SetLoadLimit(MemMB);
    A[i-First].Initialize(TmpIndexFileName,Parent,&map,i-First);
  }
  
  for(;;){
    INT ActiveCount=0,ActiveItem=0;
    for(j=0; j<2; j++){ // count active items
      if(A[j].Empty()==GDT_FALSE){
        ++ActiveCount;
        ActiveItem=j;
      }
    }
    if(ActiveCount==1){         // if only 1 is left, we are done.  Flush it.
      A[ActiveItem].Flush(fj);
      break;                    // go do cleanup and close files
    }
    
    // find first active item of remaining several
    for(k=0; k<2; k++)
      if(A[k].Empty()==GDT_FALSE)
        break;
    // k is number of first active item
    A[k].GetSistring(&Current);
    CurrSmallest=k;
    for(++k;k<IndexNum; k++){   // loop through other active items
      if(A[k].Empty()==GDT_FALSE){
        val=A[k].Smallest(&Current);    // if true, current was smaller
        if(val==GDT_FALSE)
          CurrSmallest=k;
      }
      
    }
    // at this point, CurrSmallest is the one to write and reload
    
    A[CurrSmallest].Write(fj);
    
  }                             // loop
  // clean up old files
  for(i=First; i<=Second; i++){
    sprintf(Tmp,".%d",i);
    TmpIndexFileName=IndexFileName;
    TmpIndexFileName.Cat(Tmp);
    CHR *p=TmpIndexFileName.NewCString();
#ifdef VERBOSE
    printf("Deleting %s\n", p);
#endif
    unlink(p);
    delete [] p;
  }
  fclose(fj);
  TmpIndexFileName=IndexFileName;
  sprintf(Tmp,".%d",First);
  TmpIndexFileName.Cat(Tmp);
  printf("Creating ");
  TmpIndexFileName.Print();
  printf("\n");
  rename(OutFile,TmpIndexFileName);
  fa=Parent->ffopen(CheckName,"w");
  IndexNum--;
  fprintf(fa,"%d\n",IndexNum);
  fclose(fa);

}               


INDEX::~INDEX() {
  //  delete SetCache;
#ifdef DICTIONARY
  delete Dict;
#endif
}
