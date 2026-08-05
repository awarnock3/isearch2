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
File:		irset.cxx
Version:	1.01
Description:	Class IRSET - Internal Search Result Set
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "idbobj.hxx"
#include "result.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"


int 
IrsetIndexCompare(const void* x, const void* y) 
{
  INT4 Difference = ( (*((PIRESULT)y)).GetMdtIndex() - 
		     (*((PIRESULT)x)).GetMdtIndex() );
  if (Difference < 0) {
    return (-1);
  } else {
    if (Difference == 0) {
      return(0);
    } else {
      return 1;
    }
  }
}


IRSET::IRSET(const PIDBOBJ DbParent) 
{
  Init(DbParent);
}


void 
IRSET::Init(const PIDBOBJ DbParent) 
{
  Table = new IRESULT[1000];
  TotalEntries = 0;
  MaxEntries = 1000;
  Parent = DbParent;
  MinScore=999999.0;
  MaxScore=0.0;
  // BUGFIX #3: was `INT ScoreSort=0;`, declaring a local that shadowed
  // (and was never read past) the ScoreSort member instead of
  // initializing it, leaving the member uninitialized garbage until the
  // first SortByScore()/SortByIndex() call. No current caller reads
  // ScoreSort, so this had no observable effect, but it's still an
  // uninitialized member and the source of a standing -Wunused-variable
  // warning on every build.
  ScoreSort=0;		// 1 if sorted by score
}


// BUGFIX #2: see the declaration in irset.hxx for why this is needed.
// Deliberately does NOT base-construct via `OPERAND(OtherIrset)`: that
// would invoke OPERAND's own implicit copy constructor, which -- since
// ATTRLIST (OPERAND::Attributes' type) has the identical missing-copy-
// constructor defect one level down (see docs/BUG_CATALOG.md under
// src/operand.hxx) -- shallow-copies ATTRLIST's own Table and produces
// the exact same double-free, just one hop deeper. Confirmed by hitting
// it: the standalone repro below aborted in ATTRLIST::~ATTRLIST() until
// this was reworked to default-construct the OPERAND base instead and
// copy Attributes through GetAttributes()/SetAttributes(), both of
// which go through ATTRLIST::operator=, which -- unlike its copy
// constructor -- already deep-copies correctly.
IRSET::IRSET(const IRSET& OtherIrset) : OPERAND()
{
  Init(OtherIrset.Parent);
  ATTRLIST Attrs;
  OtherIrset.GetAttributes(&Attrs);
  SetAttributes(Attrs);
  INT x;
  for (x = 0; x < OtherIrset.TotalEntries; x++) {
    FastAddEntry(OtherIrset.Table[x], 0);
  }
  MinScore = OtherIrset.MinScore;
  MaxScore = OtherIrset.MaxScore;
  ScoreSort = OtherIrset.ScoreSort;
}


DOUBLE
IRSET::GetMaxScore(){
  return(MaxScore);
}


DOUBLE 
IRSET::GetMinScore(){
  return(MinScore);
}


// BUGFIX #4: was missing a self-assignment guard. On `x = x;`,
// OtherIrset *is* `*this`, so the original unconditionally deleted
// Table and re-Init()'d before ever reading OtherIrset.GetTotalEntries()
// -- by which point that count (reading the same, just-reset object)
// was 0, silently discarding every entry. Confirmed real with a
// standalone repro: a 1-entry IRSET self-assigned through its OPOBJ&
// interface dropped to 0 entries.
OPOBJ&
IRSET::operator=(const OPOBJ& OtherIrset) {
  if (&OtherIrset == this) {
    return *this;
  }
  if (Table) {
    delete [] Table;
  }
  Init(OtherIrset.GetParent());
  INT y = OtherIrset.GetTotalEntries();
  INT x;
  IRESULT iresult;
  for (x=1; x<=y; x++) {
    OtherIrset.GetEntry(x, &iresult);
    AddEntry(iresult, 0);
  }
  return *this;
}


OPOBJ* 
IRSET::Duplicate() const {
  IRSET* Temp = new IRSET(Parent);
  *Temp = *((OPOBJ*)this);
  return (OPOBJ*)Temp;
}


PIRSET 
IRSET::Duplicate(){
  IRSET * Temp= new IRSET(Parent);
  *Temp=*((OPOBJ*)this);
  return(Temp);
}


void 
IRSET::MergeEntries(const INT AddHitCounts)
{
  INT x;
  DOUBLE y;
  INT CurrentItem=0;
  IRESULT *NewTable;
#ifdef DO_HIGHLIGHTING  
  FCT Fct;
#endif
  
  if (TotalEntries == 0)
    return;
  NewTable= new IRESULT[TotalEntries];
  
  NewTable[0]=Table[0];
  for (x=1; x<TotalEntries; x++) {
    if (NewTable[CurrentItem].GetMdtIndex() == Table[x].GetMdtIndex()) {
      if (AddHitCounts) {
	NewTable[CurrentItem].IncHitCount(Table[x].GetHitCount());
#ifdef DO_HIGHLIGHTING  
	NewTable[CurrentItem].AddToHitTable(Table[x]);
#endif
      }

      y = Table[x].GetScore();
      NewTable[CurrentItem].IncScore(y);
      if (y > MaxScore) {
	MaxScore = y;
      }
      if (y < MinScore) {
	MinScore = y;
      }

    } else {
      CurrentItem++;
      NewTable[CurrentItem]=Table[x];
    }
  }
  
  delete [] Table;
  Table=new IRESULT[CurrentItem+1];
  MaxEntries=CurrentItem+1;
  TotalEntries=CurrentItem+1;

  for(x=0; x<TotalEntries; x++) {
    Table[x]=NewTable[x];
    if (Table[x].GetScore()<MinScore)
      MinScore=Table[x].GetScore();
    if (Table[x].GetScore()>MaxScore)
      MaxScore=Table[x].GetScore();
  }

  delete [] NewTable;
}


void
IRSET::FastAddEntry(const IRESULT& ResultRecord, const INT AddHitCounts)
{
  if (TotalEntries == MaxEntries)
    Expand();
  Table[TotalEntries] = ResultRecord;
  TotalEntries = TotalEntries + 1;
  
}


void 
IRSET::AddEntry(const IRESULT& ResultRecord, const INT AddHitCounts) 
{
  INT x;
  DOUBLE y;
  // linear!
  for (x=0; x<TotalEntries; x++) {
    if (Table[x].GetMdtIndex() == ResultRecord.GetMdtIndex()) {
      if (AddHitCounts) {
	Table[x].IncHitCount(ResultRecord.GetHitCount());
#ifdef DO_HIGHLIGHTING  
	Table[x].AddToHitTable(ResultRecord);
#endif
      }

      y=ResultRecord.GetScore();
      Table[x].IncScore(y);
      if (y > MaxScore)
	MaxScore = y;
      if (y < MinScore)
	MinScore = y;
      return;
    }
   
  }
  
  if (TotalEntries == MaxEntries)
    Expand();
  Table[TotalEntries] = ResultRecord;
  TotalEntries = TotalEntries + 1;
}


void 
IRSET::GetEntry(const INT Index, PIRESULT ResultRecord) const {
  if ( (Index > 0) && (Index <= TotalEntries) ) {
    *ResultRecord = Table[Index-1];
  }
}


PRSET 
IRSET::GetRset()
{
  RSET *prset = new RSET();
  RESULT result;
  MDTREC mdtrec;
#ifdef DO_HIGHLIGHTING  
  FCT Fct;
#endif
  STRING s;
  INT x;
  for (x=0; x<TotalEntries; x++) {
    Parent->GetMainMdt()->GetEntry(Table[x].GetMdtIndex(), &mdtrec);
    if (mdtrec.GetDeleted() == GDT_FALSE) {
      mdtrec.GetKey(&s);
      result.SetKey(s);
      mdtrec.GetDocumentType(&s);
      result.SetDocumentType(s);
      mdtrec.GetPathName(&s);
      result.SetPathName(s);
      mdtrec.GetFileName(&s);
      result.SetFileName(s);
      result.SetRecordStart(mdtrec.GetLocalRecordStart());
      result.SetRecordEnd(mdtrec.GetLocalRecordEnd());
      result.SetScore(Table[x].GetScore());
      result.SetDbNum(Table[x].GetDbNum());
#ifdef DO_HIGHLIGHTING  
      Table[x].GetHitTable(&Fct);
      Fct.SubtractOffset(mdtrec.GetGlobalFileStart() + 
			 mdtrec.GetLocalRecordStart());
      result.SetHitTable(Fct);
#endif
      prset->AddEntry(result);
    }
  }
  return prset;
}


PRSET 
IRSET::GetRset(INT4 Start, INT4 End) {
  RSET *prset = new RSET();
  RESULT result;
  MDTREC mdtrec;
#ifdef DO_HIGHLIGHTING  
  FCT Fct;
#endif
  STRING s;
  INT x;
  
  if(End>TotalEntries)
    End=TotalEntries;
  for (x=Start; x<End; x++) {
    
#ifdef DO_HIGHLIGHTING  
    Table[x].GetHitTable(&Fct);
    //	Fct.ConvertHits(mdtrec);
    result.SetHitTable(Fct);
#endif
    result.SetScore(Table[x].GetScore());
    result.SetDbNum(Table[x].GetDbNum());
    prset->AddEntry(result);
  }
  return prset;
}


void 
IRSET::Fill(INT Start, INT End, PRSET set) 
{
  RESULT result;
  MDTREC mdtrec;
  MDT* ThisMdt;

#ifdef DO_HIGHLIGHTING  
  FCT Fct;
#endif
  STRING s;
  INT x, y;
  if(End>TotalEntries)
    End=TotalEntries;
  //  for (y=0, x=Start-1; x<End; x++, y++) {
  for (y=0, x=Start; x<End; x++, y++) {
    set->GetEntry(x+1,&result);

    ThisMdt = Table[x].GetMdt();
    ThisMdt->GetEntry(Table[x].GetMdtIndex(), &mdtrec);

    //    Parent->GetMainMdt()->GetEntry(Table[x].GetMdtIndex(), &mdtrec);
    mdtrec.GetKey(&s);
    result.SetKey(s);
    mdtrec.GetDocumentType(&s);
    result.SetDocumentType(s);
    mdtrec.GetPathName(&s);
    result.SetPathName(s);
    mdtrec.GetFileName(&s);
    result.SetFileName(s);
    result.SetRecordStart(mdtrec.GetLocalRecordStart());
    result.SetRecordEnd(mdtrec.GetLocalRecordEnd());
    
#ifdef DO_HIGHLIGHTING  
    result.GetHitTable(&Fct);
    //Fct.ConvertHits(mdtrec);
    Fct.SubtractOffset(mdtrec.GetGlobalFileStart() + 
		       mdtrec.GetLocalRecordStart());
    result.SetHitTable(Fct);
#endif
    result.SetScore(Table[x].GetScore());
    //set->SetEntry(x+1,result);
    set->SetEntry(y+1,result);
  }
}


void 
IRSET::Expand() {
  //  Resize(TotalEntries+1000);
  // Really resize this
  Resize(TotalEntries*2);
}


void 
IRSET::CleanUp() {
  Resize(TotalEntries);
}


void 
IRSET::Resize(const INT Entries) {
  IRESULT *Temp = new IRESULT[Entries];
  INT RecsToCopy;
  INT x;
  if (Entries >= TotalEntries) {
    RecsToCopy = TotalEntries;
  } else {
    RecsToCopy = Entries;
    TotalEntries = Entries;
  }
  for (x=0; x<RecsToCopy; x++) {
    // Not sure if Temp[x] = Table[x] is good enough.
    Temp[x] = Table[x];
  }
  if (Table)
    delete [] Table;
  Table = Temp;
  MaxEntries = Entries;
}


INT 
IRSET::GetTotalEntries() const {
  return TotalEntries;
}


INT 
IRSET::GetHitTotal() const {
  INT x;
  INT Total = 0;
  for (x=0; x<TotalEntries; x++) {
    Total += Table[x].GetHitCount();
  }
  return Total;
}


void 
IRSET::Or(const OPOBJ& OtherIrset) {
  INT x;
  INT t = OtherIrset.GetTotalEntries();
  IRESULT OtherIresult;
  for (x=1; x<= t; x++) {
    OtherIrset.GetEntry(x, &OtherIresult);
    FastAddEntry(OtherIresult, 0);
  }
  SortByIndex();
  MergeEntries(0);
}


void 
IRSET::Concat(const OPOBJ& OtherIrset) {
  INT x;
  INT t = OtherIrset.GetTotalEntries();
  IRESULT OtherIresult;
  for (x=1; x<= t; x++) {
    OtherIrset.GetEntry(x, &OtherIresult);
    FastAddEntry(OtherIresult, 0);
  }
  //  SortByIndex();
  //  MergeEntries(0);
}


#ifdef DO_HIGHLIGHTING  
// CharProx added by Kevin Gamiel
//
// Example:  'dog' within 5 characters of 'cat'
//
// For(all entries in dog result-list)
//   For(all entries in cat result-list)
//     if(in same record)
//       for(each hit in dog hit table)
//         for(each hit in cat hit table)
//           if(hits within 5 characters of each other)
//             add current dog item to final result-list
//
void 
IRSET::CharProx(const OPOBJ& OtherIrset, const INT Distance) {
  IRESULT OtherIresult, MyIresult;
  FCT MyHitTable, OtherHitTable;
  IRSET MyResult(Parent);
  INT i,j,k,p;
  INT OtherSetTotalEntries = OtherIrset.GetTotalEntries();
  IRESULT* match;
  INT MyNumHits, OtherNumHits;
  FC MyFc, OtherFc;
  GDT_BOOLEAN IsMatch;
  
  for(i=1;i <= TotalEntries;i++) {
    GetEntry(i, &MyIresult);

    for(j=1;j <= OtherSetTotalEntries;j++) {
      OtherIrset.GetEntry(j, &OtherIresult);
      if(MyIresult.GetMdtIndex() == 
         OtherIresult.GetMdtIndex()) {
        MyIresult.GetHitTable(&MyHitTable);
        OtherIresult.GetHitTable(&OtherHitTable);
        MyNumHits = MyHitTable.GetTotalEntries();       
        OtherNumHits = OtherHitTable.GetTotalEntries(); 

        for(k=1;k <= MyNumHits;k++) {
          MyHitTable.GetEntry(k, &MyFc);

          for(p=1;p<=OtherNumHits;p++) {
            IsMatch = GDT_FALSE;
            OtherHitTable.GetEntry(p, &OtherFc);
            if(MyFc.GetFieldStart() < OtherFc.GetFieldStart()) {
	      if((OtherFc.GetFieldStart() - MyFc.GetFieldEnd() - 1) 
		 <= Distance)
		IsMatch = GDT_TRUE;
	    } else {
	      if((MyFc.GetFieldStart() - OtherFc.GetFieldEnd() - 1)
		 <= Distance)
		IsMatch = GDT_TRUE;
	    }
            if(IsMatch)
              MyResult.AddEntry(OtherIresult, 0);
          }
        }
      }
    }
  }
  
  delete [] Table;
  TotalEntries=MyResult.GetTotalEntries();
  MaxEntries=MyResult.MaxEntries;
  Table = MyResult.StealTable();
}
#endif

#ifndef MULTI
// AndNOT added by Glenn MacStravic
void 
IRSET::AndNot(const OPOBJ& OtherIrset) 
{
  IRESULT OtherIresult;
  IRSET MyResult(Parent);
  INT x = 1;
  INT count=0;
  INT t = OtherIrset.GetTotalEntries();
  IRESULT* match;
  SortByIndex();
  
  while (x <= t) {
    OtherIrset.GetEntry(x, &OtherIresult);
#ifndef __SUNPRO_CC
    match = (IRESULT*)bsearch(&OtherIresult, Table,
			      TotalEntries, sizeof(IRESULT), 
			      IrsetIndexCompare);
#else
    match = (IRESULT*)bsearch((char*)&OtherIresult, (char*)Table,
			      TotalEntries, sizeof(IRESULT), 
			      IrsetIndexCompare);
#endif
    if (match == nullptr) {
      MyResult.FastAddEntry(OtherIresult, 0);
      count++;
    }
    x++;
  }
  MyResult.SortByIndex();
  MyResult.MergeEntries(0);
  delete [] Table;
  TotalEntries=count;
  MaxEntries=MyResult.MaxEntries;
  Table = MyResult.StealTable();
}


// Faster AND implementation added by Glenn MacStravic
void 
IRSET::And(const OPOBJ& OtherIrset) 
{
  IRESULT OtherIresult;
  IRSET MyResult(Parent);
  INT x = 1;
  INT count=0;
  INT t = OtherIrset.GetTotalEntries();
  IRESULT* match;
  DOUBLE y;

  SortByIndex();
  while (x <= t) {
    OtherIrset.GetEntry(x, &OtherIresult);
#ifndef __SUNPRO_CC
    match = (IRESULT*)bsearch(&OtherIresult, Table,
			      TotalEntries, 
			      sizeof(IRESULT), IrsetIndexCompare);
    
#else
    match = (IRESULT*)bsearch((char*)&OtherIresult, (char*)Table,
			      TotalEntries, 
			      sizeof(IRESULT), IrsetIndexCompare);
    
#endif
    if (match != nullptr) {
#ifdef DO_HIGHLIGHTING  
      match->AddToHitTable(OtherIresult);
#endif
      match->IncHitCount(OtherIresult.GetHitCount());
      y = OtherIresult.GetScore();
      match->IncScore(y);

      if (y > MaxScore)
	MaxScore = y;
      if (y < MinScore)
	MinScore = y;

      MyResult.FastAddEntry(*match, 0);
      count++;
    }
    x++;
  }
  MyResult.SortByIndex();
  MyResult.MergeEntries(0);
  delete [] Table;
  TotalEntries=count;
  MaxEntries=MyResult.MaxEntries;
  if (MyResult.GetMaxScore() > MaxScore) {
    MaxScore = MyResult.GetMaxScore();
  }
  if (MyResult.GetMinScore() < MinScore) {
    MinScore = MyResult.GetMinScore();
  }
  Table = MyResult.StealTable();
}


#else
void 
IRSET::And(const OPOBJ& OtherIrset) {
  // not a very fast implementation
  INT y;
  GDT_BOOLEAN found;
  IRESULT OtherIresult;
  INT x = 0;
  IRSET *pTempIrset;
  
  RSET *Prset,*OtherPrset;
  RESULT MyResultRecord, OtherResultRecord;
  STRING MyPath,OtherPath;
  
  pTempIrset = (PIRSET) &OtherIrset;
  OtherPrset = pTempIrset->GetRset();
  Prset = GetRset();
  
  while (x < TotalEntries) {
    found = GDT_FALSE;
    Prset->GetEntry(x+1, &MyResultRecord);
    MyResultRecord.GetPathName(&MyPath);
    
    for (y=1; y<=OtherIrset.GetTotalEntries(); y++) {
      OtherPrset->GetEntry(y, &OtherResultRecord);
      OtherResultRecord.GetPathName(&OtherPath);
      
      if (MyPath == OtherPath) {
	found = GDT_TRUE;
	break;
      }
    }
    if (!found) {
      Table[x].SetMdtIndex(0);
    } 
    x++;
    
  }
  
  INT in, out, last;
  out = 0;
  last = TotalEntries;
  for (in=0;in<last;in++) {
    if (Table[in].GetMdtIndex() != 0) {
      Table[out] = Table[in];
      out++;
    } else {
      TotalEntries--;
    }
  }
}


void 
IRSET::AndNot(const OPOBJ& OtherIrset) {
  // not a very fast implementation
  INT y; 
  GDT_BOOLEAN found;
  IRESULT OtherIresult;
  INT x = 0;
  IRSET *pTempIrset;
  
  RSET *Prset,*OtherPrset;
  RESULT MyResultRecord, OtherResultRecord;
  STRING MyPath,OtherPath;
  
  pTempIrset = (PIRSET) &OtherIrset;
  OtherPrset = pTempIrset->GetRset();
  Prset = GetRset();
  
  while (x < TotalEntries) {
    found = GDT_FALSE;
    Prset->GetEntry(x+1, &MyResultRecord);
    MyResultRecord.GetPathName(&MyPath);
    
    for (y=1; y<=OtherIrset.GetTotalEntries(); y++) {
      OtherPrset->GetEntry(y, &OtherResultRecord);
      OtherResultRecord.GetPathName(&OtherPath);
      
      if (MyPath.Equals(OtherPath)) {
        found = GDT_TRUE;
        break;
      }
    }
    if (found) {
      Table[x].SetMdtIndex(0);
    } 
    x++;
    
  }
  
  INT in, out, last;
  out = 0;
  last = TotalEntries;
  for (in=0;in<last;in++) {
    if (Table[in].GetMdtIndex() != 0) {
      Table[out] = Table[in];
      out++;
    } else {
      TotalEntries--;
    }
  }
}

#endif

IRESULT* 
IRSET::StealTable() {
  IRESULT* TempTablePtr = Table;
  Table = new IRESULT[2];
  TotalEntries = 0;
  MaxEntries = 2;
  
  return TempTablePtr;
}


void 
IRSET::ComputeScores(const INT TermWeight) {
  if (TotalEntries == 0) {
    return;
  }
  INT x;
  DOUBLE DocsInRs = TotalEntries;
  DOUBLE DocsInDb = Parent->GetMainMdt()->GetTotalEntries();
  DOUBLE InvDocFreq = DocsInDb / DocsInRs;
  DOUBLE SumSqScores = 0;
  DOUBLE SqrtSum;
  DOUBLE Score;
  for (x=0; x<TotalEntries; x++) {
    Score = Table[x].GetHitCount() * InvDocFreq;
    Table[x].SetScore(Score);
    SumSqScores += (Score * Score);
  }
  SqrtSum = sqrt(SumSqScores);
  if (SqrtSum == 0.0) {
    SqrtSum = 1.0;
  }
  for (x=0; x<TotalEntries; x++) {
    Score=Table[x].GetScore() / SqrtSum * TermWeight;
    if(Score>MaxScore)
      MaxScore=Score;
    if(Score<MinScore)
      MinScore=Score;
    Table[x].SetScore(Score);
  }
}


int 
IrsetScoreCompare(const void* x, const void* y) {
  DOUBLE Difference = ( (*((PIRESULT)y)).GetScore() - 
		       (*((PIRESULT)x)).GetScore() );
  if (Difference < 0) {
    return (-1);
  } else {
    if (Difference == 0) {
      return ( (*((PIRESULT)x)).GetMdtIndex() - 
	      (*((PIRESULT)y)).GetMdtIndex() );
    } else {
      return 1;
    }
  }
 
}


void 
IRSET::SortByScore() {
  qsort(Table, TotalEntries, sizeof(IRESULT), IrsetScoreCompare);
  ScoreSort=1;
}


void 
IRSET::SortByIndex() {
  qsort(Table, TotalEntries, sizeof(IRESULT), IrsetIndexCompare);
  ScoreSort=0;
}


void 
IRSET::SetParent(PIDBOBJ const NewParent) {
  Parent = NewParent;
}


PIDBOBJ 
IRSET::GetParent() const {
  return Parent;
}


void
IRSET::StoreDbNum(const INT DbNum) {
  for (INT x=0;x<TotalEntries;x++) {
    Table[x].SetDbNum(DbNum);
  }
}


void
IRSET::SetMdt(MDT& NewMdt) {
  for (INT x=0;x<TotalEntries;x++) {
    Table[x].SetMdt(NewMdt);
  }
}


void 
IRSET::Dump() {
  INT x;
  RSET *prset;
  RESULT ResultRecord;
  STRING PathName, FileName, ResultKey;

  prset=GetRset(0,TotalEntries);
  Fill(0,TotalEntries,prset);
  
  cerr << endl;
  cerr << "IRSET Dump:" << endl;

  for (x=1;x<=TotalEntries; x++) {
    prset->GetEntry(x,&ResultRecord);
    ResultRecord.GetPathName(&PathName);
    ResultRecord.GetFileName(&FileName);
    cerr << "   -Result#" << x << " ";
    cerr << PathName;
    cerr << FileName << endl;

    ResultRecord.GetKey(&ResultKey);
    cerr << "    Key=";
    cerr << ResultKey;
    cerr << " [";
    cerr << ResultRecord.GetRecordStart();
    cerr << ", ";
    cerr << ResultRecord.GetRecordEnd();
    cerr << "]" << endl;
  }
}


IRSET::~IRSET() {
  if (Table)
    delete [] Table;
}
