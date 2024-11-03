/* $Id: numsearch.cxx,v 1.12 1999/03/26 00:27:22 cnidr Exp $ */
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
File:		numsearch.cxx
Version:	$Revision: 1.12 $
Description:	Class INDEX - numeric search methods
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
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
//#include "sw.hxx"
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


void 
INDEX::SortNumericFieldData()
{
  INT total;
  INT x;
  DFD DfdRecord;
  STRING FieldName,FieldType,Fn;
  INT4 Count;

  total = Parent->DfdtGetTotalEntries();

  for (x=1; x<=total; x++) {
    Parent->DfdtGetEntry(x,&DfdRecord);
    DfdRecord.GetFieldType(&FieldType);
    if (FieldType.CaseEquals("TEXT")) {
      continue;
    } else if (FieldType.CaseEquals("NUM")) {
      NUMERICLIST NumList;
      DfdRecord.GetFieldName(&FieldName);
      Parent->DfdtGetFileName(FieldName,&Fn);
      NumList.SetFileName(Fn);
      NumList.LoadTable(0,-1);
      Count = NumList.GetCount();
      if (Count > 1) {
	CHR *Fname;
	Fname = Fn.NewCString();
	unlink(Fname);
	delete Fname;

	NumList.Sort();
	NumList.WriteTable(0);
	NumList.SortByGP();
	NumList.WriteTable(Count);
      }

      /*
	// Debugging

	if (Count > 0) {
	cout << "\nDumping " << FieldName << endl;
	cout << "Debug output:" << endl;
	NUMERICLIST NL;
	NL.SetFileName(Fn);
	NL.LoadTable(0,-1,VAL_BLOCK);
	NL.Dump(0,Count);
	} else
	cout << "Empty field " << FieldName << endl;
      */

    } else if (FieldType.CaseEquals("COMPUTED")) {
      NUMERICLIST NumList;
      DfdRecord.GetFieldName(&FieldName);
      Parent->DfdtGetFileName(FieldName,&Fn);
      NumList.SetFileName(Fn);
      NumList.LoadTable(0,-1);
      Count = NumList.GetCount();
      if (Count > 1) {
	CHR *Fname;
	Fname = Fn.NewCString();
	unlink(Fname);
	delete Fname;

	NumList.Sort();
	NumList.WriteTable(0);
	NumList.SortByGP();
	NumList.WriteTable(Count);
      }

      /*
	// Debugging
	if (Count > 0) {
	cout << "\nDumping " << FieldName << endl;
	cout << "Debug output:" << endl;
	NUMERICLIST NL;
	NL.SetFileName(Fn);
	NL.LoadTable(0,-1,VAL_BLOCK);
	NL.Dump(0,Count);
	} else
	cout << "Empty field " << FieldName << endl;
      */

    } else if ((FieldType.CaseEquals("DATE-RANGE")) 
	       || (FieldType.CaseEquals("DATE"))) {
      INTERVALLIST IntList;
      DfdRecord.GetFieldName(&FieldName);
      Parent->DfdtGetFileName(FieldName,&Fn);
      IntList.SetFileName(Fn);
      IntList.LoadTable(0,-1);
      Count = IntList.GetCount();
      if (Count > 1) {
	CHR *Fname;
	Fname = Fn.NewCString();
	unlink(Fname);
	delete Fname;

	IntList.SortByStart();
	IntList.WriteTable(0);
	IntList.SortByEnd();
	IntList.WriteTable(Count);
	IntList.SortByGP();
	IntList.WriteTable(2*Count);
      }

      /*
	if (Count > 0) {
	cout << "\nDumping " << FieldName << endl;
	cout << "Debug output:" << endl;
	INTERVALLIST NL;
	NL.SetFileName(Fn);
	NL.LoadTable(0,-1,0);
	cout << Count << " records sorted by start:\n" << endl;
	NL.Dump(0,Count);

	NL.LoadTable(0,-1,Count);
	cout << Count << " records sorted by end:\n" << endl;
	NL.Dump(0,Count);

	} else
	cout << "Empty field " << FieldName << endl;
      */

    } else {
      continue;
    }
  }
  return;
}


PIRSET 
INDEX::NumericSearch(const DOUBLE fKey, const STRING& FieldName, 
		     INT4 Relation) 
{	
  STRING      FieldType;
  PIRSET      pirset;
  SearchState Status=NO_MATCH;
  INT4        Start=-1, End=-1, Pointer=0, Value, ListCount;
  INT4        w;
  IRESULT     iresult;
  MDT*        ThisMdt;  
  STRING      Fn;
  NUMERICLIST List;

  Parent->FieldTypes.GetValue(FieldName,&FieldType);
  
  if (FieldType.GetLength() == 0)
    FieldType = "TEXT";
  if(FieldType == "TEXT")
    return((PIRSET)NULL);
  
  pirset=new IRSET(Parent);  
  
  /*  We'll fix the rset cache when we can feed the server name to it
  
      STRING DBName="",T1;
      CHR    TempBuffer[256];
      Parent->GetDBFileStem(DBName);

      sprintf(TempBuffer,"%f",fKey);
      T1 = TempBuffer;
      w = SetCache->Check(T1,Relation,FieldName,DBName);

      if(w > -1) {
      delete pirset;
      return(SetCache->Fetch(w));
      }
  */
  Parent->DfdtGetFileName(FieldName, &Fn);

  switch (Relation) {

  case ZRelEQ:			// equals
    // Start is the smallest index in the table 
    // for which fKey is <= to the table value
    Status = List.Find(Fn, fKey, ZRelGT, &End);

    if (Status == TOO_LOW)    // We ran off the bottom end without a match
      Status = NO_MATCH;
    if (Status == NO_MATCH)   // No matching values - bail out
      break;

    // End is the largest index in the table for which
    // fKey is >= to the table value;
    Status = List.Find(Fn, fKey, ZRelLT, &Start);

    if (Status == TOO_HIGH)   // We ran off the top
      Status = NO_MATCH;
    
    break;

  case ZRelLT:			// less than
  case ZRelLE:			// less than or equal to
    // Start at the beginning of the table
    Start=0;
    // End is the largest index in the table for which 
    // fKey is <= the table value
    Status = List.Find(Fn, fKey, Relation, &End);
    if (Status == TOO_LOW)    // We ran off the low end without a match
      Status = NO_MATCH;
    break;

  case ZRelGE:			// greater than or equal to
  case ZRelGT:			// greater than
    // Go to the end of the table
    End = -1;
    // Find the smallest index for which fKey is >= the table value
    Status = List.Find(Fn, fKey, Relation, &Start);
    if (Status == TOO_HIGH)   // We ran off the top end without a match
      Status = NO_MATCH;
    break;
  }
  // Bail out if we failed to find the value we were looking for
  if (Status == NO_MATCH)
    return pirset;

  List.SetFileName(Fn);
  if (Relation != ZRelEQ) {
    List.LoadTable(Start, End, VAL_BLOCK);

  } else if ( (Relation == ZRelEQ) && ( (End == -1) || (Start == -1) ) ) {
    if (Start != -1)
      Start++;
    if (End != -1)
      End--;

    List.LoadTable(Start, End, VAL_BLOCK);
    
  } else {
    // OK - it was equal and we found the value.  But Start points to the
    // first value less than the hit, and End points to the first value
    // greater than the hit, so we need to bump them to get to the right 
    // values
    List.LoadTable(Start+1, End-1, VAL_BLOCK); 
  }

  ListCount = List.GetCount();

  for(Pointer=0; Pointer<ListCount; Pointer++){
    ThisMdt = Parent->GetMainMdt();
    Value=List.GetGlobalStart(Pointer);
    w = Parent->GetMainMdt()->LookupByGp(Value);
    iresult.SetMdtIndex(w);
    iresult.SetHitCount(1);
    iresult.SetScore(0);
    iresult.SetMdt(*ThisMdt);
    pirset->FastAddEntry(iresult, 1);
  }

  pirset->SortByIndex();
  pirset->MergeEntries(1);
  //  SetCache->Add(T1,Relation,FieldName,DBName,pirset);
#ifdef DEBUG
  cout << "NumericSearch - " << pirset->GetTotalEntries()
    << " hits in " << FieldName << " for term=" << fKey
      << ", relation=" << Relation << endl;
#endif
  return(pirset);
}
