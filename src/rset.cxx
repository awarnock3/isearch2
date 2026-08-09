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
File:		rset.cxx
Version:	1.00
Description:	Class RSET - Search Result Set
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdio.h>

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"

RSET::RSET() {
  Table = new RESULT[100];
  TotalEntries = 0;
  MaxEntries = 100;
  HighScore = 0;
  LowScore = 0;
}


// BUGFIX #2: see the declaration in rset.hxx for why this is needed.
RSET::RSET(const RSET& OtherRset) {
  Table = new RESULT[100];
  TotalEntries = 0;
  MaxEntries = 100;
  HighScore = OtherRset.HighScore;
  LowScore = OtherRset.LowScore;
  Resize(OtherRset.TotalEntries);
  SIZE_T x;
  for (x = 0; x < OtherRset.TotalEntries; x++) {
    Table[x] = OtherRset.Table[x];
  }
  TotalEntries = OtherRset.TotalEntries;
}


// BUGFIX #2: see the declaration in rset.hxx for why this is needed.
RSET& RSET::operator=(const RSET& OtherRset) {
  if (&OtherRset == this) {
    return *this;
  }
  if (Table) {
    delete [] Table;
  }
  Table = new RESULT[100];
  TotalEntries = 0;
  MaxEntries = 100;
  HighScore = OtherRset.HighScore;
  LowScore = OtherRset.LowScore;
  Resize(OtherRset.TotalEntries);
  SIZE_T x;
  for (x = 0; x < OtherRset.TotalEntries; x++) {
    Table[x] = OtherRset.Table[x];
  }
  TotalEntries = OtherRset.TotalEntries;
  return *this;
}


// BUGFIX #4: was `fread((char*)Table, 1, FSize, fp)` -- a raw memory dump
// straight into Table, restoring RESULT's STRING fields' internal Buffer
// pointers as whatever raw bytes SaveTable happened to write, i.e.
// pointer values from a since-freed (or entirely different process's)
// heap. Confirmed real with a standalone repro: SaveTable() a populated
// RSET, LoadTable() it into a fresh RSET (forcing other heap activity in
// between), then read back an entry's Key -- aborted under ASan with
// heap-use-after-free in STRING::Copy()'s memcpy. Fixed by serializing
// each RESULT field through its own public getters/setters (the same
// text-based, newline-delimited approach used throughout this tree, e.g.
// RECORD::Write/Read) instead of copying RESULT's raw in-memory bytes.
void
RSET::LoadTable(const STRING& FileName) {
  PFILE fp = fopen(FileName, "rb");
  if (!fp) {
    perror(FileName);
    EXIT_ERROR;
  }
  else {
    TotalEntries = 0;
    STRING s;
    s.FGet(fp, 32);
    SIZE_T Count = (SIZE_T)s.GetLong();
    Resize(Count);
    RESULT r;
    STRING Field;
    SIZE_T x;
    for (x = 0; x < Count; x++) {
      s.FGet(fp, 32);
      r.SetDbNum(s.GetInt());
      Field.FGet(fp, DocumentKeySize);
      r.SetKey(Field);
      Field.FGet(fp, DocumentTypeSize);
      r.SetDocumentType(Field);
      Field.FGet(fp, DocPathNameSize);
      r.SetPathName(Field);
      Field.FGet(fp, DocFileNameSize);
      r.SetFileName(Field);
      s.FGet(fp, 32);
      r.SetRecordStart((GPTYPE)s.GetLong());
      s.FGet(fp, 32);
      r.SetRecordEnd((GPTYPE)s.GetLong());
      s.FGet(fp, 32);
      r.SetScore(s.GetFloat());
      AddEntry(r);
    }
    fclose(fp);
  }
}


// BUGFIX #4: see LoadTable above for why this no longer writes Table's
// raw bytes.
void
RSET::SaveTable(const STRING& FileName) {
  PFILE fp = fopen(FileName, "wb");
  if (!fp) {
    perror(FileName);
    EXIT_ERROR;
  }
  else {
    fprintf(fp, "%zu\n", TotalEntries);
    STRING Field;
    SIZE_T x;
    for (x = 0; x < TotalEntries; x++) {
      fprintf(fp, "%d\n", Table[x].GetDbNum());
      Table[x].GetKey(&Field);
      Field.Print(fp);
      fprintf(fp, "\n");
      Table[x].GetDocumentType(&Field);
      Field.Print(fp);
      fprintf(fp, "\n");
      Table[x].GetPathName(&Field);
      Field.Print(fp);
      fprintf(fp, "\n");
      Table[x].GetFileName(&Field);
      Field.Print(fp);
      fprintf(fp, "\n");
      fprintf(fp, "%u\n", Table[x].GetRecordStart());
      fprintf(fp, "%u\n", Table[x].GetRecordEnd());
      fprintf(fp, "%.17g\n", Table[x].GetScore());
    }
    fclose(fp);
  }
}


void 
RSET::AddEntry(const RESULT& ResultRecord) {
  DOUBLE S;
  S = ResultRecord.GetScore();
  if (S > HighScore) {
    HighScore = S;
  }
  if (S < LowScore) {
    LowScore = S;
  }
  if (TotalEntries == MaxEntries)
    Expand();
  Table[TotalEntries] = ResultRecord;
  TotalEntries = TotalEntries + 1;
}


void
RSET::GetEntry(const INT Index, PRESULT ResultRecord) const {
  // BUGFIX #3: was `Index <= TotalEntries`, comparing a signed INT
  // against an unsigned SIZE_T directly (-Wsign-compare). Index > 0 is
  // already checked first, so the cast below is always value-preserving.
  if ( (Index > 0) && ((SIZE_T)Index <= TotalEntries) ) {
    *ResultRecord = Table[Index-1];
  }
}


// BUGFIX #6: was `return (Key1 == Key2);` -- STRING::operator== returns
// a boolean (1 if equal, 0 otherwise), never a negative value. A qsort
// comparator that can never return negative doesn't implement a valid
// ordering (undefined behavior per the C standard), so SortByKey() below
// didn't reliably sort at all. Fixed to use STRING::Cmp(), which returns
// a proper strcmp()-style negative/zero/positive result.
static int
RsetCompareKeys(const void* ResultPtr1, const void* ResultPtr2) {
  static STRING Key1;
  static STRING Key2;
  ((RESULT*)ResultPtr1)->GetKey(&Key1);
  ((RESULT*)ResultPtr2)->GetKey(&Key2);
  return Key1.Cmp(Key2);
}


void 
RSET::SortByKey() {
  qsort(Table, TotalEntries, sizeof(RESULT), RsetCompareKeys);
}


// BUGFIX #5: was `(int)(Score1*100 - Score2*100)` -- ascending (lowest
// score first), the opposite of IRSET::SortByScore's own comparator
// (IrsetScoreCompare, src/irset.cxx) for the same conceptual operation,
// which sorts descending -- the sensible order for a *search* result set
// (best matches first). It also truncated the difference through an int
// cast, which could round a small-but-real score difference to 0,
// treating genuinely unequal scores as equal. Fixed to sort descending
// using a sign check instead, matching IrsetScoreCompare's approach.
static int
RsetCompareScores(const void* ResultPtr1, const void* ResultPtr2) {
  DOUBLE Difference = ((RESULT*)ResultPtr2)->GetScore() - ((RESULT*)ResultPtr1)->GetScore();
  if (Difference < 0) {
    return -1;
  } else if (Difference > 0) {
    return 1;
  }
  return 0;
}


void 
RSET::SortByScore() {
  qsort(Table, TotalEntries, sizeof(RESULT), RsetCompareScores);
}


INT 
RSET::GetScaledScore(const DOUBLE UnscaledScore, const INT ScaleFactor) {
  DOUBLE Diff = HighScore - LowScore;
  if (Diff == 0) {
    //		Diff=1;
    return ScaleFactor;
  }
  return ( (INT)( ((UnscaledScore - LowScore) * ScaleFactor) / Diff ) );
}


void 
RSET::Expand() {
  Resize(TotalEntries+100);
}


void 
RSET::CleanUp() {
  Resize(TotalEntries);
}


void 
RSET::Resize(const SIZE_T Entries) {
  PRESULT Temp = new RESULT[Entries];
  SIZE_T RecsToCopy;
  SIZE_T x;
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


SIZE_T 
RSET::GetTotalEntries() {
  return TotalEntries;
}

//void RSET::Dump() const {
/*
	INT x, w;
	for (x=0; x<TotalEntries; x++) {
		w = Table[x].GetMdtIndex();
		cout<<w<<", ";
	}
	cout<<"\n";
*/
//}


void 
RSET::SetScoreRange(DOUBLE High, DOUBLE Low)
{
  HighScore=High;
  LowScore=Low;
}


void
RSET::SetEntry(const INT x, const RESULT& ResultRecord )
{
  // BUGFIX #7 (docs/BUG_CATALOG.md#srcrsetcxx): unlike GetEntry() (see
  // BUGFIX #3), this had no bounds check at all -- Table[x-1] was
  // written unconditionally for any caller-supplied x. Confirmed real
  // with a standalone repro: SetEntry() with an index past MaxEntries
  // produced a wild-pointer heap-buffer-overflow (RESULT::operator=,
  // called on the out-of-range slot, tried to free/reuse whatever
  // garbage STRING::Buffer pointer happened to be there). This file's
  // sole real caller (IRSET::Fill(), src/irset.cxx) is safe today only
  // because every call site first sizes `set` via GetRset() with the
  // exact same range Fill() then iterates, so this was a real,
  // exploitable contract gap on a public method, not (yet) an active
  // one -- the same "unenforced index contract" category already fixed
  // for src/rcache.cxx's Fetch() (BUGFIX #3) and every other indexed
  // accessor in this tree (DFT::GetEntry, FCT::GetEntry, this class's
  // own GetEntry). Fixed by matching GetEntry()'s bounds check and
  // convention: silently no-op when x is out of [1, TotalEntries].
  if (x < 1 || (SIZE_T)x > TotalEntries) {
    return;
  }
  DOUBLE S;
  S = ResultRecord.GetScore();
  if (S > HighScore) {
    HighScore = S;
  }
  if (S < LowScore) {
    LowScore = S;
  }
  Table[x-1] = ResultRecord;

}

RSET::~RSET() {
  if (Table)
    delete [] Table;
}
