// $Id: intfield.hxx,v 1.3 1998/05/12 16:49:09 cnidr Exp $
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
File:		intfield.hxx
Version:	$Revision: 1.3 $
Description:	Class INTFIELD - Numeric interval data object
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.


#include "gdt.h"
#include "nfield.hxx"

#ifndef INTERVALFLD_HXX
#define INTERVALFLD_HXX

// One numeric-interval entry: a byte offset (GlobalStart) paired with
// a [StartValue, EndValue] range, indexed for range-overlap search
// (see INTERVALLIST, src/intlist.hxx, which sorts and range-queries
// arrays of these). Public interface is non-virtual accessors only;
// see docs/BUG_CATALOG.md#srcintfieldcxx for two pre-existing header
// design issues found but not fixed this turn (GENERAL step 4 freezes
// public signatures): GlobalStart/GetGlobalStart()/SetGlobalStart()
// here shadow NUMERICFLD's own same-named member/methods instead of
// reusing them (inherited GetNumericValue()/SetNumericValue() are the
// only NUMERICFLD members INTERVALFLD doesn't shadow, and go unused by
// every call site found), and operator= has a non-standard signature
// (returns by value, takes a non-const reference) that compiles but
// can't be chained or assigned from a temporary.
class INTERVALFLD : public NUMERICFLD {

public:
  INTERVALFLD();
  INTERVALFLD(const INTERVALFLD& OtherField);
  INTERVALFLD operator=(INTERVALFLD& OtherField);
  INT4   GetGlobalStart()        { return GlobalStart; }
  void   SetGlobalStart(INT4 x)  { GlobalStart = x; }
  DOUBLE GetStartValue()         { return StartValue; }
  void   SetStartValue(DOUBLE x) { StartValue = x; }
  DOUBLE GetEndValue()           { return EndValue; }
  void   SetEndValue(DOUBLE x)   { EndValue = x; }
  ~INTERVALFLD();

private:
  INT4   GlobalStart;
  DOUBLE StartValue;
  DOUBLE EndValue;
};

typedef INTERVALFLD* PINTERVALFLD;

#endif
