/* $Id: irset.hxx,v 1.8 2000/02/04 23:21:07 cnidr Exp $ */
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
File:		irset.hxx
Version:	1.01
$Revision: 1.8 $
Description:	Class IRSET - Internal Search Result Set
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef IRSET_HXX
#define IRSET_HXX

class IRSET : public OPERAND {
public:
  IRSET(const PIDBOBJ DbParent);
  OPOBJ&  operator=(const OPOBJ& OtherIrset);
  void    Init(const PIDBOBJ DbParent);
  INT     GetOperandType() const { return TypeRset; };
  OPOBJ*  Duplicate() const;
  IRSET*  Duplicate();
  void    AddEntry(const IRESULT& ResultRecord, const INT AddHitCounts);
  void    FastAddEntry(const IRESULT& ResultRecord, const INT AddHitCounts);
  void    MergeEntries(const INT AddHitCounts);
  void    GetEntry(const INT Index, PIRESULT ResultRecord) const;
  PRSET   GetRset();
  PRSET   GetRset(INT4 Start, INT4 End);

  void    Fill(INT Start, INT End, PRSET set);
  void    Expand();
  void    CleanUp();
  void    Resize(const INT Entries);
  INT     GetTotalEntries() const;
  INT     GetHitTotal() const;
  void    ComputeScores(const INT TermWeight);
  void    Or(const OPOBJ& OtherIrset);
  void    And(const OPOBJ& OtherIrset);
  void    AndNot(const OPOBJ& OtherIrset);
  void    Concat(const OPOBJ& OtherIrset);
#ifdef DO_HIGHLIGHTING
  //Near is hardcoded to be 50 characters
  void    Near(const OPOBJ& OtherIrset) { CharProx(OtherIrset, 50); }
  void    CharProx(const OPOBJ& OtherIrset, const INT Distance);
#endif
  void    SortByScore();
  void    SortByIndex();
  void    SetParent(PIDBOBJ const NewParent);
  DOUBLE  GetMaxScore();
  DOUBLE  GetMinScore();
  IDBOBJ* GetParent() const;
  void    StoreDbNum(const INT DbNum);
  void    SetMdt(MDT& NewMdt);
  void    Dump();
  ~IRSET();

  //	void LoadTable(const STRING& FileName);
  //	void SaveTable(const STRING& FileName);
private:
  IRESULT* StealTable();
  IRESULT* Table;
  INT      TotalEntries;
  INT      MaxEntries;
  IDBOBJ*  Parent;
  INT      ScoreSort;
  DOUBLE   MaxScore,MinScore;
  // Add date & time of search
};

typedef IRSET* PIRSET;

#endif
