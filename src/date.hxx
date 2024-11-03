// $Id: date.hxx,v 1.4 1998/05/12 16:48:57 cnidr Exp $
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
File:		date.hxx
Version:	$Revision: 1.4 $
Description:	Class SRCH_DATE - Isearch Date data structures class
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/

#ifndef DATE_HXX
#define DATE_HXX

#include "defs.hxx"
#include "string.hxx"

// DATE Routines

const DOUBLE YEAR_LOWER   =        99.0;
const DOUBLE MONTH_LOWER  =     99999.0;
const DOUBLE DAY_LOWER    =   9999999.0;

const DOUBLE YEAR_UPPER   =     10000.0;
const DOUBLE MONTH_UPPER  =   1000000.0;
const DOUBLE DAY_UPPER    = 100000000.0;

const DOUBLE DATE_ERROR   = -99999999.0;
const DOUBLE DATE_UNKNOWN = -99999998.0;

const DOUBLE DATE_PRESENT = 100000001.0;

enum Date_Precision { BAD_DATE=-1, YEAR_PREC, MONTH_PREC, DAY_PREC };
enum Date_Match { MATCH_ERROR=-1, BEFORE, BEFORE_DURING, DURING_EQUALS, 
		  DURING_AFTER, AFTER };

class SRCH_DATE

{
  friend class DATERANGE;
public:
  SRCH_DATE();
  SRCH_DATE(const SRCH_DATE& OtherDate);
  SRCH_DATE(const DOUBLE FloatVal);
  SRCH_DATE(const STRING& String);
  SRCH_DATE(const CHR* CString);
  SRCH_DATE(const LONG LongVal);
  SRCH_DATE(const INT IntVal);

  SRCH_DATE&          operator=(const SRCH_DATE& OtherDate);
  DOUBLE         GetValue() const { return d_date; }
  Date_Precision GetPrecision() const { return d_prec; }

  GDT_BOOLEAN IsYearDate();
  GDT_BOOLEAN IsMonthDate();
  GDT_BOOLEAN IsDayDate();
  GDT_BOOLEAN IsBogusDate();
  GDT_BOOLEAN IsValidDate();

  GDT_BOOLEAN TrimToMonth();
  GDT_BOOLEAN TrimToYear();

  GDT_BOOLEAN PromoteToMonthStart();
  GDT_BOOLEAN PromoteToMonthEnd();
  GDT_BOOLEAN PromoteToDayStart();
  GDT_BOOLEAN PromoteToDayEnd();

  void        GetTodaysDate();
  
  GDT_BOOLEAN IsBefore(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN Equals(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN IsDuring(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN IsAfter(const SRCH_DATE& OtherDate) const;

  ~SRCH_DATE();

private:
  // Methods
  void        SetPrecision();
  Date_Match  DateCompare(const SRCH_DATE& OtherDate) const;


  // Data
  DOUBLE            d_date;  // A float w/ format YYYY, YYYYMM or YYYYMMDD
  Date_Precision    d_prec;

};

typedef SRCH_DATE* PSRCH_DATE;

class DATERANGE
{
public:
  DATERANGE();
  DATERANGE(const SRCH_DATE& NewStart, const SRCH_DATE& NewEnd);
  DATERANGE(const SRCH_DATE& NewDate);
  DATERANGE(const STRING& DateString);
  DATERANGE(const CHR* DateString);

  SRCH_DATE  GetStart() { return d_start; }
  SRCH_DATE  GetEnd() { return d_end; }

  void  SetStart(const SRCH_DATE& NewStart)   { d_start = NewStart; }
  void  SetStart(const DOUBLE NewStart)  { d_start = NewStart; }
  void  SetStart(const CHR* NewStart)    { d_start = NewStart; }
  void  SetStart(const STRING& NewStart) { d_start = NewStart; }

  void  SetEnd(const SRCH_DATE& NewEnd)       { d_end = NewEnd; }
  void  SetEnd(const DOUBLE NewEnd)      { d_end = NewEnd; }
  void  SetEnd(const CHR* NewEnd)        { d_end = NewEnd; }
  void  SetEnd(const STRING& NewEnd)     { d_end = NewEnd; }

  GDT_BOOLEAN Contains(const SRCH_DATE& TestDate) const;

  ~DATERANGE();

private:
  SRCH_DATE d_start;
  SRCH_DATE d_end;

};

typedef DATERANGE* PDATERANGE;
#endif
