/* $Id: squery.cxx,v 1.10 2001/02/23 16:57:33 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1994-1999.

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
File:		squery.cxx
Version:	1.00
$Revision: 1.10 $
Description:	Class SQUERY - Search Query
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "squery.hxx"

/// Constructs an empty query with no synonym thesaurus open.
SQUERY::SQUERY() {
  Thesaurus = nullptr;
}


/// Copy-constructs Opstack/c_kwaqs_term from OtherSquery; the copy
/// starts with no thesaurus of its own (see BUGFIX #1) regardless of
/// whether OtherSquery has one open.
SQUERY::SQUERY(const SQUERY& OtherSquery) {
  Opstack = OtherSquery.Opstack;
  c_kwaqs_term = OtherSquery.c_kwaqs_term;
  Thesaurus = nullptr;
}


SQUERY&
SQUERY::operator=(const SQUERY& OtherSquery) {
  Opstack = OtherSquery.Opstack;
  // BUGFIX #1, continued: c_kwaqs_term was never copied at all here
  // (only Opstack and the buggy Thesaurus alias below were) -- found
  // while fixing the copy constructor above; a real, separate
  // incomplete-copy bug, confirmed by inspection (SetKWAQSTerm()/
  // GetKWAQSTerm() are real, meaningful accessors for this member, not
  // dead code).
  c_kwaqs_term = OtherSquery.c_kwaqs_term;
  // BUGFIX #1, continued: this used to be `Thesaurus =
  // OtherSquery.Thesaurus;`, aliasing the source's pointer -- both
  // objects then shared one THESAURUS* that only one CloseThesaurus()
  // call could safely free (confirmed heap-use-after-free under ASan),
  // and `this`'s own previous Thesaurus (if any) was silently leaked by
  // the overwrite. Matches the copy constructor's "starts with no
  // thesaurus" semantics: free this's own previous one first, then
  // leave it null rather than sharing the source's.
  if (Thesaurus)
    delete Thesaurus;
  Thesaurus = nullptr;
  return *this;
}


void 
SQUERY::SetOpstack(const OPSTACK& NewOpstack) {
  Opstack = NewOpstack;
}


void 
SQUERY::GetOpstack(POPSTACK OpstackBuffer) const {
  *OpstackBuffer = Opstack;
}


void 
SQUERY::SetTerm(const STRING& NewTerm) {
  STRLIST   ListTemp;
  STRING    StrTemp;
  STERM     Sterm;
  OPSTACK   Stack;
  OPERATOR  Operator;
  TOKENGEN *TermList;
  INT       x, y, ListLen;
  ATTRLIST *AttrlistPtr;
  GDT_BOOLEAN IsPhrase = GDT_FALSE;

  TermList = new TOKENGEN(NewTerm);
  //  TermList->SetQuoteStripping(GDT_TRUE);
  //  TermList->SetQuoteStripping(GDT_FALSE);
  y = TermList->GetTotalEntries();
  Operator.SetOperatorType(OperatorOr);

  for (x=1; x<=y; x++) {
    AttrlistPtr = new ATTRLIST();
    TermList->GetEntry(x, &StrTemp);

    // Check to see if we are parsing a phrase, and if the quotes are
    // on the whole term
    if ((StrTemp.GetChr(1) == '"') 
	&& (StrTemp.GetChr(StrTemp.GetLength()) == '"')) {
      IsPhrase = GDT_TRUE;
      StrTemp.EraseBefore(2);
    }

    /*
      ListTemp.Split("/", StrTemp);
      ListLen = ListTemp.GetTotalEntries();
      if (ListLen >= 2) {
      ListTemp.GetEntry(1, &StrTemp);	// get field name
      AttrlistPtr->AttrSetFieldName(StrTemp);
      ListTemp.GetEntry(2, &StrTemp);	// get remaining string
      }
    */

    // Only grab the first / for a field delimiter
    STRINGINDEX ptr;
    STRING TmpTerm;
    TmpTerm = StrTemp;
    ptr = TmpTerm.Search("/");
    if (ptr > 0) {
      TmpTerm.EraseAfter(ptr-1);
      AttrlistPtr->AttrSetFieldName(TmpTerm);
      //    cerr << "Field is " << TmpTerm << endl;
      StrTemp.EraseBefore(ptr+1);
    //    cerr << "Term is " << StrTemp << endl;
    }
    // Check to see if we are parsing a phrase, and if the quotes are
    // on the separated term
    if ((StrTemp.GetChr(1) == '"') && (StrTemp.GetChr(StrTemp.GetLength()) == '"')) {
      IsPhrase = GDT_TRUE;
      StrTemp.EraseBefore(2);
      StrTemp.EraseAfter(StrTemp.GetLength() - 1);
    }

    if (!(IsPhrase)) {
      ListTemp.Split(":", StrTemp);
      ListLen = ListTemp.GetTotalEntries();
      if (ListLen >= 2) {
	ListTemp.GetEntry(2, &StrTemp);	// get term weight
	AttrlistPtr->AttrSetTermWeight(StrTemp);
	ListTemp.GetEntry(1, &StrTemp);	// get remaining string
      }
    }

    if (StrTemp.GetChr(StrTemp.GetLength()) == '*') {
      AttrlistPtr->AttrSetRightTruncation(GDT_TRUE);
      StrTemp.EraseAfter(StrTemp.GetLength()-1);
    }

    Sterm.SetTerm(StrTemp);
    Sterm.SetAttributes(*AttrlistPtr);
    delete AttrlistPtr;
    Stack << Sterm;
    if (x > 1) {
      // if this is not the first term, push an OR
      Stack << Operator;
    }
  }
  SetOpstack(Stack);
  delete TermList;
}


void 
SQUERY::SetRpnTerm(const STRING& NewTerm) {
  TOKENGEN *TermList;
  INT       x, y;
  STRLIST   ListTemp;
  STRING    StrTemp;
  STERM     Sterm;
  OPSTACK   Stack;
  OPERATOR  Operator;
  ATTRLIST *AttrlistPtr;
  GDT_BOOLEAN IsPhrase = GDT_FALSE;

  TermList = new TOKENGEN(NewTerm);
  //  TermList->SetQuoteStripping(GDT_TRUE);
  y = TermList->GetTotalEntries();

  for (x=1; x<=y; x++) {
    TermList->GetEntry(x, &StrTemp);
    //quick hack fix -jem.
    if ( (StrTemp == "") || (StrTemp == " ") )
      continue;

    if ( (StrTemp ^= "OR") || (StrTemp ^= "AND") 
	 || (StrTemp ^= "ANDNOT") || (StrTemp ^= "NEAR") ) {
      if (StrTemp ^= "OR") {
	Operator.SetOperatorType(OperatorOr);
      }
      if (StrTemp ^= "AND") {
	Operator.SetOperatorType(OperatorAnd);
      }
      if (StrTemp ^= "ANDNOT") {
	Operator.SetOperatorType(OperatorAndNot);
      }
      if (StrTemp ^= "NEAR") {
	Operator.SetOperatorType(OperatorNear);
      }	
      Stack << Operator;

    } else {
      AttrlistPtr = new ATTRLIST();

      // Check to see if we are parsing a phrase, and if the quotes are
      // on the whole term
      if ((StrTemp.GetChr(1) == '"') 
	  && (StrTemp.GetChr(StrTemp.GetLength()) == '"')) {
	IsPhrase = GDT_TRUE;
	StrTemp.EraseBefore(2);
      }
      /*
      ListTemp.Split("/", StrTemp);
      if (ListTemp.GetTotalEntries() >= 2) {
	ListTemp.GetEntry(1, &StrTemp);	// get field name
        AttrlistPtr->AttrSetFieldName(StrTemp);
	ListTemp.GetEntry(2, &StrTemp);	// get remaining string
      }
      */

      // Only grab the first / for a field delimiter
      STRINGINDEX ptr;
      STRING TmpTerm;
      TmpTerm = StrTemp;
      ptr = TmpTerm.Search("/");
      if (ptr > 0) {
	TmpTerm.EraseAfter(ptr-1);
	AttrlistPtr->AttrSetFieldName(TmpTerm);
	//    cerr << "Field is " << TmpTerm << endl;
	StrTemp.EraseBefore(ptr+1);
	//    cerr << "Term is " << StrTemp << endl;
      }
      // Check to see if we are parsing a phrase, and if the quotes are
      // on the separated term
      if ((StrTemp.GetChr(1) == '"') 
	  && (StrTemp.GetChr(StrTemp.GetLength()) == '"')) {
	IsPhrase = GDT_TRUE;
	StrTemp.EraseBefore(2);
	StrTemp.EraseAfter(StrTemp.GetLength() - 1);
      }

      if (!(IsPhrase)) {
	ListTemp.Split(":", StrTemp);
	if (ListTemp.GetTotalEntries() >= 2) {
	  ListTemp.GetEntry(2, &StrTemp);	// get term weight
	  AttrlistPtr->AttrSetTermWeight(StrTemp);
	  ListTemp.GetEntry(1, &StrTemp);	// get remaining string
	}
      }

      if (StrTemp.GetChr(StrTemp.GetLength()) == '*') {
	AttrlistPtr->AttrSetRightTruncation(GDT_TRUE);
	StrTemp.EraseAfter(StrTemp.GetLength()-1);
      }
      Sterm.SetTerm(StrTemp);
      Sterm.SetAttributes(*AttrlistPtr);
      delete AttrlistPtr;
      Stack << Sterm;
    }
  }
  SetOpstack(Stack);
  delete TermList;
}


void 
SQUERY::GetTerm(PSTRING StringBuffer) const {
  *StringBuffer = "";
  OPSTACK Stack;
  GetOpstack(&Stack);
  POPOBJ OpPtr;
  ATTRLIST Attrlist;
  STRING S;
  INT Count = 0;
  while (Stack >> OpPtr) {
    if (OpPtr->GetOpType() == TypeOperand) {
      if (Count > 0) {
	StringBuffer->Cat(" ");
      }
      Count++;
      OpPtr->GetAttributes(&Attrlist);
      if (Attrlist.AttrGetFieldName(&S)) {
	if (S.GetLength() > 0) {
	  S += "/";
	  *StringBuffer += S;
	}
      }
      OpPtr->GetTerm(&S);
      *StringBuffer += S;
      if (Attrlist.AttrGetRightTruncation()) {
	*StringBuffer += "*";
      }
      if (Attrlist.AttrGetTermWeight(&S)) {
	*StringBuffer += ":";
	*StringBuffer += S;
      }
    }
    delete OpPtr;
  }
}


void 
SQUERY::SetKWAQSTerm(STRING & kwaqs_string)
{
  c_kwaqs_term=kwaqs_string;
}


void 
SQUERY::GetKWAQSTerm(STRING *kwaqs_string) const
{
  *kwaqs_string=c_kwaqs_term;  
}


/// Opens (or replaces) this query's synonym thesaurus. Any previously
/// open thesaurus is freed first (see BUGFIX #4 -- calling this twice
/// without an intervening CloseThesaurus() used to leak the first one).
void
SQUERY::OpenThesaurus(const STRING& PathName, const STRING& FileName) {
  if (Thesaurus)
    delete Thesaurus;
  Thesaurus = new THESAURUS(PathName, FileName);
}


/// Closes and frees this query's thesaurus, if one is open; a no-op
/// otherwise.
void
SQUERY::CloseThesaurus() {
  // BUGFIX #3 (docs/BUG_CATALOG.md#srcsqueryhxx): Thesaurus was freed
  // but never reset to nullptr afterward, leaving it a dangling
  // pointer. This was already latent (a second CloseThesaurus() call,
  // or ExpandQuery()'s `if (!Thesaurus) return;` check, would then
  // operate on freed memory) but became load-bearing the moment
  // ~SQUERY() was fixed to free Thesaurus too (BUGFIX #2 below): every
  // normal OpenThesaurus()+CloseThesaurus()-paired SQUERY would
  // otherwise double-free at destruction.
  if (Thesaurus) {
    delete Thesaurus;
    Thesaurus = nullptr;
  }
}


void
SQUERY::ExpandQuery() {
  if (!Thesaurus)
    return;

  STRING   StringBuffer;
  OPSTACK  Stack;
  OPOBJ   *OpPtr;
  ATTRLIST Attrlist;
  STRING   S, ParentTerm, FieldName, ChildTerm;
  STRLIST  ChildTerms;
  INT      Count = 0;
  INT      nChildren;

  GetOpstack(&Stack);
  while (Stack >> OpPtr) {
    if (OpPtr->GetOpType() == TypeOperand) {
      if (Count > 0) {
	StringBuffer.Cat(" ");
      }
      Count++;

      OpPtr->GetTerm(&S);

      Thesaurus->GetParent(S,&ParentTerm);
      Thesaurus->GetChildren(ParentTerm,&ChildTerms);
      nChildren = ChildTerms.GetTotalEntries();
      
      OpPtr->GetAttributes(&Attrlist);
      if (Attrlist.AttrGetFieldName(&S)) {
	if (S.GetLength() > 0) {
	  FieldName = S;
	  FieldName += "/";
	}
      }
      
      for (INT i=1; i<= nChildren; i++) {
	ChildTerms.GetEntry(i,&ChildTerm);
	S = FieldName;
	S += ChildTerm;
	StringBuffer.Cat(S);
	StringBuffer.Cat(" ");
      }
    }
    delete OpPtr;
  }
  SetTerm(StringBuffer);
}


/// Frees Thesaurus if it's still open (see BUGFIX #2 -- a SQUERY that
/// called OpenThesaurus() and was destroyed without a matching
/// CloseThesaurus() used to leak it, and transitively its open file
/// handles). Safe to call after CloseThesaurus() too: Thesaurus is
/// nullptr in that case (BUGFIX #3), and `delete nullptr` is a no-op.
SQUERY::~SQUERY() {
  delete Thesaurus;
}
