// $Id: date.cxx,v 1.6 2000/07/21 10:12:33 cnidr Exp $
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
File:		date.cxx
Version:	$Revision: 1.6 $
Description:	Class SRCH_DATE - Isearch Date data structure class
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdlib.h>
#include <time.h>

#include "defs.hxx"
#include "string.hxx"
#include "date.hxx"

// Constructors
SRCH_DATE::SRCH_DATE()
{
  // BUGFIX #1: d_date/d_prec had no in-class initializer and were
  // never set here, so a default-constructed SRCH_DATE (e.g.
  // DATERANGE's own default constructor, also empty, default-
  // constructs two of these) started with indeterminate values.
  // DATE_ERROR/BAD_DATE mirror the sentinel this same file already
  // uses elsewhere for "no date" (see DATERANGE's parsing
  // constructors' not-a-range fallback below).
  d_date = DATE_ERROR;
  d_prec = BAD_DATE;
}

SRCH_DATE::SRCH_DATE(const SRCH_DATE& OtherDate)
{
  d_date = OtherDate.d_date;
  d_prec = OtherDate.d_prec;
}


SRCH_DATE::SRCH_DATE(const DOUBLE FloatVal)
{
  d_date = FloatVal;
  SetPrecision();
}


SRCH_DATE::SRCH_DATE(const STRING& StringVal)
{
  d_date = StringVal.GetFloat();
  SetPrecision();
}


SRCH_DATE::SRCH_DATE(const CHR* CStringVal)
{
  d_date = atof(CStringVal);
  SetPrecision();
}


SRCH_DATE::SRCH_DATE(const LONG LongVal)
{
  d_date = (DOUBLE)LongVal;
  SetPrecision();
}


SRCH_DATE::SRCH_DATE(const INT IntVal)
{
  d_date = (DOUBLE)IntVal;
  SetPrecision();
}


SRCH_DATE::~SRCH_DATE() 
{
}


// Operators
SRCH_DATE&
SRCH_DATE::operator=(const SRCH_DATE& OtherDate)
{
  d_date = OtherDate.d_date;
  d_prec = OtherDate.d_prec;
  return *this;
}


// Precision conversion routines
GDT_BOOLEAN
SRCH_DATE::TrimToMonth()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Can't convert from lower precision
    status = GDT_FALSE;
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Lop off the last two digits & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)/100L );
    d_prec = MONTH_PREC;
    break;

  default:
    // Precision was probably unknown
    status = GDT_FALSE;
    break;
  }

  return(status);
}


GDT_BOOLEAN
SRCH_DATE::TrimToYear()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Already have the right precision
    break;

  case MONTH_PREC:
    // Lop off the last two digits & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)/100L );
    d_prec = YEAR_PREC;
    break;

  case DAY_PREC:
    // Lop off the last four digits & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)/10000L );
    d_prec = YEAR_PREC;
    break;

  default:
    // Precision was probably unknown
    status = GDT_FALSE;
    break;
  }

  return(status);
}


GDT_BOOLEAN
SRCH_DATE::PromoteToMonthStart()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Convert YYYY to YYYY01 & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 1L );
    d_prec = MONTH_PREC;
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Can't promote from higher precision
    status = GDT_FALSE;
    break;

  default:
    status = GDT_FALSE;
    break;
  }

  return(status);
}


GDT_BOOLEAN
SRCH_DATE::PromoteToMonthEnd()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Convert YYYY to YYYY12 & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 12L );
    d_prec = MONTH_PREC;
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Can't promote from higher precision
    status = GDT_FALSE;
    break;

  default:
    status = GDT_FALSE;
    break;
  }

  return(status);
}


GDT_BOOLEAN
SRCH_DATE::PromoteToDayStart()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Convert YYYY to YYYY0101 & reset precision
    PromoteToMonthStart();
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 1L );
    d_prec = DAY_PREC;
    break;

  case MONTH_PREC:
    // Convert YYYYMM to YYYYMM01 & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 1L );
    d_prec = DAY_PREC;
    break;

  case DAY_PREC:
    // Already have the right precision
    break;

  default:
    status = GDT_FALSE;
    break;
  }

  return(status);
}


GDT_BOOLEAN
SRCH_DATE::PromoteToDayEnd()
{
  //LONG        TmpDate;
  GDT_BOOLEAN status=GDT_TRUE;

  switch (d_prec) {

  case YEAR_PREC:
    // Convert YYYY to YYYY1231 & reset precision
    PromoteToMonthEnd();
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 31L );
    d_prec = DAY_PREC;
    break;

  case MONTH_PREC:
    // Convert YYYYMM to YYYYMM31 & reset precision
    d_date = (DOUBLE) ( ((LONG) d_date)*100L + 31L );
    d_prec = DAY_PREC;
    break;

  case DAY_PREC:
    // Already have the right precision
    break;

  default:
    status = GDT_FALSE;
    break;
  }

  return(status);
}


// Boolean precision routines
GDT_BOOLEAN 
SRCH_DATE::IsYearDate()
{
  if (d_prec == YEAR_PREC)
    return(GDT_TRUE);
  return(GDT_FALSE);
}


GDT_BOOLEAN 
SRCH_DATE::IsMonthDate()
{
  if (d_prec == MONTH_PREC)
    return(GDT_TRUE);
  return(GDT_FALSE);
}


GDT_BOOLEAN 
SRCH_DATE::IsDayDate()
{
  if (d_prec == DAY_PREC)
    return(GDT_TRUE);
  return(GDT_FALSE);
}


GDT_BOOLEAN 
SRCH_DATE::IsBogusDate()
{
  if (d_prec == BAD_DATE)
    return(GDT_TRUE);
  return(GDT_FALSE);
}


GDT_BOOLEAN 
SRCH_DATE::IsValidDate()
{
  if (d_prec == BAD_DATE)
    return(GDT_FALSE);
  return(GDT_TRUE);
  //  return( !IsBogusDate() );
}

const size_t MAX_DATESTR_LEN = 9;
/// Fills this object with the current date
void
SRCH_DATE::GetTodaysDate()
{
  size_t     converted, ConvertLen=MAX_DATESTR_LEN;
  time_t     today;
  struct tm *t;
  CHR        Hold[MAX_DATESTR_LEN+1];

  today = time(nullptr);
  t = localtime(&today);

  converted = strftime(Hold,ConvertLen,"%Y%m%d",t);
  if ((converted == (size_t)0) || (converted == MAX_DATESTR_LEN)) {
    // BUGFIX #2: this branch used to fall straight through to the
    // unconditional `d_date = atof(Hold);` below, silently overwriting
    // the just-set error state -- and, since a failed strftime() (the
    // `converted == 0` case) leaves Hold's contents untouched, atof()
    // would then read Hold's uninitialized stack bytes as if they were
    // a C string. Returning here makes the error state actually stick.
    d_date = -1.0;
    d_prec = BAD_DATE;
    return;
  }

  d_date = atof(Hold);
  SetPrecision();

}

GDT_BOOLEAN 
SRCH_DATE::IsBefore(const SRCH_DATE& OtherDate) const
{
  if (DateCompare(OtherDate) == BEFORE)
    return GDT_TRUE;
  return GDT_FALSE;
}


GDT_BOOLEAN 
SRCH_DATE::Equals(const SRCH_DATE& OtherDate) const
{
  if (DateCompare(OtherDate) == DURING_EQUALS)
    return GDT_TRUE;
  return GDT_FALSE;
}


GDT_BOOLEAN 
SRCH_DATE::IsDuring(const SRCH_DATE& OtherDate) const
{
  if (DateCompare(OtherDate) == DURING_EQUALS)
    return GDT_TRUE;
  return GDT_FALSE;
}


GDT_BOOLEAN 
SRCH_DATE::IsAfter(const SRCH_DATE& OtherDate) const
{
  if (DateCompare(OtherDate) == AFTER)
    return GDT_TRUE;
  return GDT_FALSE;
}


// private
void
SRCH_DATE::SetPrecision()
{
  if ((d_date > YEAR_LOWER) && (d_date < YEAR_UPPER))
    d_prec = YEAR_PREC;
  else if ((d_date > MONTH_LOWER) && (d_date < MONTH_UPPER))
    d_prec = MONTH_PREC;
  else if ((d_date > DAY_LOWER) && (d_date < DAY_UPPER))
    d_prec = DAY_PREC;
  else
    d_prec = BAD_DATE;

  return;
}


Date_Match
SRCH_DATE::DateCompare(const SRCH_DATE& OtherDate) const
{
  SRCH_DATE      D1,D2;
  DOUBLE         d1,d2;
  Date_Precision p1, p2;

  D2 = OtherDate;

  d1 = d_date;
  d2 = D2.GetValue();

  p1 = d_prec;
  p2 = D2.GetPrecision();

  if ((p1 == BAD_DATE) || (p2 == BAD_DATE))
    return MATCH_ERROR;

  if (p1 > p2) {         // Trim OtherDate to match this object
    D1 = d1;
    switch(p2) {
    case YEAR_PREC:
      D1.TrimToYear();
      break;
    case MONTH_PREC:
      D1.TrimToMonth();
      break;
    default:
      break;
    }
    d1 = D1.GetValue();
  }

  else if (p1 < p2) {    // Trim this object to match OtherDate
    D2 = d2;             // but copy it first
    switch(p1) {
    case YEAR_PREC:
      D2.TrimToYear();
      break;
    case MONTH_PREC:
      D2.TrimToMonth();
      break;
    default:
      break;
    }
    d2 = D2.GetValue();
  }

  // Same precision - we can just compare
  if (d1 > d2) 
    return AFTER;
  else if (d1 < d2)
    return BEFORE;
  else
    return DURING_EQUALS;
}


////////////////////// Date Range Class Methods ////////////////
DATERANGE::DATERANGE()
{
}


DATERANGE::DATERANGE(const SRCH_DATE& NewStart, const SRCH_DATE& NewEnd)
{
  d_start = NewStart;
  d_end = NewEnd;
}


DATERANGE::DATERANGE(const SRCH_DATE& NewDate)
{
  // Put same date in start and end
  d_start = NewDate;
  d_end = NewDate;
}


DATERANGE::DATERANGE(const STRING& DateString)
{
  // String must be "YYYY[MM[DD]] YYYY[MM[DD]]"
  STRING Single;
  STRINGINDEX blank;

  Single = DateString;
  blank = Single.Search(" ");

  // It will be 0 if the blank was not found, so look for a /
  if (blank == 0)
    blank = Single.Search("/");

  // It will be 0 if the / was not found, so look for a comma
  if (blank == 0)
    blank = Single.Search(",");

  if (blank == 0) {
    // It must not be a date range after all
    d_start = DATE_ERROR;
    d_end = DATE_ERROR;
    return;
  }

  Single.EraseAfter(blank-1);
  d_start = Single;
  
  Single = DateString;
  Single.EraseBefore(blank+1);
  d_end = Single;
}


DATERANGE::DATERANGE(const CHR* DateString)
{
  // String must be "YYYY[MM[DD]] YYYY[MM[DD]]"
  STRING      Single;
  STRINGINDEX blank;

  Single  = DateString;
  blank   = Single.Search(" ");

  // It will be 0 if the blank was not found, so look for a /
  if (blank == 0)
    blank = Single.Search("/");

  // It will be 0 if the / was not found, so look for a comma
  if (blank == 0)
    blank = Single.Search(",");

  if (blank == 0) {
    // It must not be a date range after all
    d_start = DATE_ERROR;
    d_end = DATE_ERROR;
    return;
  }

  Single.EraseAfter(blank-1);
  d_start = Single;
  
  Single = DateString;
  Single.EraseBefore(blank+1);
  d_end  = Single;
  
}

GDT_BOOLEAN
DATERANGE::Contains(const SRCH_DATE& TestDate) const
{
  // BUGFIX #3: BEFORE/AFTER were swapped between the two clauses.
  // DateCompare(TestDate) compares *this* to TestDate, so
  // `d_start.DateCompare(TestDate) == BEFORE` means d_start is before
  // TestDate -- exactly the condition for TestDate being validly PAST
  // the range's start, not a reason to reject it. The old condition
  // (`d_start ... BEFORE || d_end ... AFTER`) was true for TestDate >
  // d_start OR TestDate < d_end, which -- for any normal range where
  // d_start <= d_end -- covers nearly every possible TestDate, so this
  // returned GDT_FALSE almost unconditionally. Confirmed by tracing a
  // concrete example: range [2020,2025], TestDate 2022 (squarely
  // inside) -- old code returned GDT_FALSE. The correct rejection
  // condition is TestDate before the range starts (d_start is AFTER
  // TestDate) or TestDate after the range ends (d_end is BEFORE
  // TestDate). Not reachable from any live call site today (no caller
  // of DATERANGE::Contains() was found in this tree), so this wasn't
  // an active-search regression, but a real defect in the class as
  // declared.
  if ((d_start.DateCompare(TestDate) == AFTER)
      || (d_end.DateCompare(TestDate) == BEFORE))
    return GDT_FALSE;
  return GDT_TRUE;
}


DATERANGE::~DATERANGE()
{
}

