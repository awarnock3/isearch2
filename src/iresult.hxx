/* $Id: iresult.hxx,v 1.5 1999/03/26 00:27:22 cnidr Exp $ */
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
File:		iresult.hxx
Version:	1.00
$Revision: 1.5 $
Description:	Class IRESULT - Internal Search Result
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef IRESULT_HXX
#define IRESULT_HXX

#include <math.h>

#include "defs.hxx"
#ifdef DO_HIGHLIGHTING
#include "fct.hxx"
#endif

// A search hit as tracked internally during scoring/merging (as
// opposed to RESULT, src/result.hxx, which is the externally-facing
// form materialized once a hit is finalized): just an MDT row index,
// running hit count and score, and -- for virtual databases -- which
// DbNum/MDT it came from.
class IRESULT {
public:
  IRESULT();
  // Copies every field, including Mdt; safe under self-assignment.
  IRESULT& operator=(const IRESULT& OtherIresult);
  void   SetMdtIndex(const INT NewMdtIndex);
  INT    GetMdtIndex() const;
  void   SetHitCount(const INT NewHitCount);
  void   IncHitCount();
  void   IncHitCount(const INT AddCount);
  INT    GetHitCount() const;
  void   SetScore(const DOUBLE NewScore);
  void   IncScore(const DOUBLE AddScore);
  DOUBLE GetScore() const;
#ifdef DO_HIGHLIGHTING
  void   SetHitTable(const FCT& NewHitTable);
  void   GetHitTable(PFCT HitTableBuffer) const;
  // Appends ResultRecord's HitTable entries onto this one's.
  void   AddToHitTable(const IRESULT& ResultRecord);
#endif
  // Saves a reference to NewMdt (not a copy or an owned pointer).
  void   SetMdt(MDT& NewMdt);
  MDT*   GetMdt() const;
  void   SetDbNum(const INT NewDbNum);
  INT    GetDbNum() const;
  ~IRESULT();
private:
  INT    MdtIndex;
  INT    HitCount;
  DOUBLE Score;
#ifdef DO_HIGHLIGHTING  
  PFCT   HitTable;
#endif
  INT    DbNum; // What virtual database did the record come from?
  MDT*   Mdt;   // Save the MDT pointer so we can retrieve the headline
                // from the right virtual database

};

typedef IRESULT* PIRESULT;

#endif
