/* $Id: datesearch.cxx,v 1.17 2000/10/15 03:46:31 cnidr Exp $ */
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
File:		datesearch.cxx
Version:	$Revision: 1.17 $
Description:	Class INDEX - date searching methods
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "isearch.hxx"
/*
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "common.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "nfield.hxx"
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
*/
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "mergeunit.hxx"
#include "filemap.hxx"
#include "date.hxx"
#include "soundex.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"

#ifdef DICTIONARY
#include "dictionary.hxx"
#endif

// Flag to select searching on interval start dates or interval end dates
//const GDT_BOOLEAN SEARCH_START = GDT_TRUE;
//const GDT_BOOLEAN SEARCH_END   = GDT_FALSE;
const IntBlock SEARCH_START = START_BLOCK;
const IntBlock SEARCH_END   = END_BLOCK;

// Flag to select whether to require strict date matching (so all dates 
// in the interval must match the query conditions, or loose date matching
// (so we'll get a match if any date in the target interval matches the
// query conditions)
const GDT_BOOLEAN STRICT_MATCH = GDT_TRUE;
const GDT_BOOLEAN LOOSE_MATCH  = GDT_FALSE;

// Flag to select whether to match the query condition endpoint, or to
// use strict inequality
const GDT_BOOLEAN NOT_DURING = GDT_FALSE;
const GDT_BOOLEAN DURING     = GDT_TRUE;


// Search date fields for matches
// Will accept intervals or single dates, and varying precisions
// YYYY, YYYYMM or YYYYMMDD
PIRSET 
INDEX::DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
			 INT4 Relation, INT4 Structure) 
{
  GDT_BOOLEAN Strict = LOOSE_MATCH;
  PIRSET pirset=(PIRSET)NULL;

  pirset =
    DoDateSearch(QueryTerm, FieldName, Relation, Structure, Strict);

  return pirset;
}


// Search date fields for matches
// Will accept intervals or single dates, and varying precisions
// YYYY, YYYYMM or YYYYMMDD
PIRSET 
INDEX::DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
			 INT4 Relation, INT4 Structure, GDT_BOOLEAN Strict)
{
  PIRSET pirset=(PIRSET)NULL;
  STRING FieldType;

  Parent->FieldTypes.GetValue(FieldName,&FieldType);

  if ((FieldType.CaseEquals("DATE-RANGE")) 
      || (FieldType.CaseEquals("DATE"))) {
  
    // We have a number of cases to check.  First, we need to see if the
    // QueryTerm is a single date or an interval - we do this by looking at
    // Structure.  
  
    if (Structure == ZStructDateRange) {
      // User has specified a date range
      pirset = DateRangeSearch(QueryTerm, FieldName, Relation, Strict);
    
    } else if (Structure == ZGEOStructDateRange) {
      // User has specified a date range
      pirset = DateRangeSearch(QueryTerm, FieldName, Relation, Strict);
    
    } else if (Structure == ZGEOStructDateString) {
      // User has specified a date string.  We have to check to see
      // whether it is a single date or a date range.
      STRING      sCopy;
      STRINGINDEX blank;

      sCopy  = QueryTerm;
      sCopy.Trim();
      sCopy.TrimLeading();

      // Look for a blank separator
      blank   = sCopy.Search(" ");

      // Look for a slash separator
      if (blank == 0)
	blank   = sCopy.Search("/");

      // Look for a comma separator
      if (blank == 0)
	blank   = sCopy.Search(",");

      // If blank is still 0, no delimiters were found, so it must be a
      // single date.  Otherwise, it is a date range.
      if (blank == 0)
	pirset = SingleDateSearch(QueryTerm, FieldName, Relation, Strict);
      else
	pirset = DateRangeSearch(QueryTerm, FieldName, Relation, Strict);
    
    } else if (Structure == ZStructDate) {
      // User has specified a single date
      pirset = SingleDateSearch(QueryTerm, FieldName, Relation, Strict);
    
      //  } else if (Structure == ZStructDateTime) {
    
    } else if (Structure == ZStructYear) {
      // User has specified just a year
      pirset = SingleDateSearch(QueryTerm, FieldName, Relation, Strict);
    
    } else {
      pirset = SingleDateSearch(QueryTerm, FieldName, Relation, Strict);
    }
  }
  
  return(pirset);
}


// DateRangeSearch searches a field of dates for values within the
// range specified by the user's query term.  QueryTerm has the form
// "YYYY[MM[DD]] YYYY[MM[DD]]"
PIRSET 
INDEX::DateRangeSearch(const STRING& QueryTerm, const STRING& FieldName, 
		       INT4 Relation, GDT_BOOLEAN Strict)
{
  PIRSET    pirset=(PIRSET)NULL;
  PIRSET    other_pirset=(PIRSET)NULL;
  PIRSET    another_pirset=(PIRSET)NULL;
  DATERANGE QueryDate=QueryTerm;
  SRCH_DATE QueryStartDate, QueryEndDate;
  
  QueryStartDate = QueryDate.GetStart();
  QueryEndDate   = QueryDate.GetEnd();
  
  switch(Relation) {

  case ZRelBefore:
    // MDStartDate < QueryStartDate
    pirset =
      SingleDateSearchBefore(QueryStartDate, FieldName, SEARCH_START, 
			     NOT_DURING);
    break;

  case ZRelBefore_Strict:
    // MDEndDate   < QueryStartDate
    pirset =
      SingleDateSearchBefore(QueryStartDate, FieldName, SEARCH_END, 
			     NOT_DURING);
    break;

  case ZRelBeforeDuring:
    // MDStartDate <= QueryEndDate
    pirset =
      SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelBeforeDuring_Strict:
    // MDEndDate   <= QueryEndDate
    pirset =
      SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuring:
    // MDStartDate inside QueryDateInterval
    // || MDEndDate inside QueryDateInterval
    pirset =
      DateRangeSearchContains(QueryDate, FieldName, SEARCH_START, 
			      DURING);
    other_pirset =
      DateRangeSearchContains(QueryDate, FieldName, SEARCH_END, 
			      DURING);
    pirset->Or(*other_pirset);
    delete other_pirset;
    break;

  case ZRelOverlaps:
    // See if there are any metadata records with starting dates
    // inside QueryDateInterval
    pirset =
      SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_START, 
			    DURING);
    INT nhits;
    nhits = pirset->GetTotalEntries();
    cerr << "Got " << nhits << " with start dates after " 
	 << QueryStartDate.GetValue() << endl;
    /*
    other_pirset =
      SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_START, 
			    DURING);
    nhits = other_pirset->GetTotalEntries();
    cerr << "Got " << nhits << " with start dates before " 
	 << QueryEndDate.GetValue() << endl;
    //pirset->And(*other_pirset);

    other_pirset =
      SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_END, 
			    DURING);
    nhits = other_pirset->GetTotalEntries();
    cerr << "Got " << nhits << " with end dates after " 
	 << QueryStartDate.GetValue() << endl;

    another_pirset =
      SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_END, 
			    DURING);
    nhits = another_pirset->GetTotalEntries();
    cerr << "Got " << nhits << " with end dates before " 
	 << QueryEndDate.GetValue() << endl;
    //    other_pirset->And(*another_pirset);
    //    pirset->Or(*other_pirset);
    */

    delete other_pirset;
    break;

  case ZRelDuring_Strict:
    // MDStartDate inside QueryDateInterval
    // && MDEndDate inside QueryDateInterval
    pirset =
      DateRangeSearchContains(QueryDate, FieldName, SEARCH_START, 
			      NOT_DURING);
    other_pirset =
      DateRangeSearchContains(QueryDate, FieldName, SEARCH_END, 
			      NOT_DURING);
    pirset->And(*other_pirset);
    delete other_pirset;
    break;

  case ZRelDuringAfter:
    // MDEndDate   >= QueryStartDate
    pirset =
      SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_END, 
			    DURING);
    break;

  case ZRelDuringAfter_Strict:
    // MDStartDate >= QueryStartDate
    pirset =
      SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_START, 
			    DURING);
    break;

  case ZRelAfter:
    // MDEndDate   > QueryEndDate
    pirset =
      SingleDateSearchAfter(QueryEndDate, FieldName, SEARCH_END, 
			    NOT_DURING);
    break;

  case ZRelAfter_Strict:
    // MDStartDate > QueryEndDate
    pirset =
      SingleDateSearchAfter(QueryEndDate, FieldName, SEARCH_START, 
			    NOT_DURING);
    break;

  default:
    break;
  }

  return pirset;
}


// SingleDateSearch searches a field of single dates for values matching
// the date specified by the query term.  Variable precision strings are
// handled - "YYYY", "YYYYMM" or "YYYYMMDD"
PIRSET 
INDEX::SingleDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
			INT4 Relation, GDT_BOOLEAN Strict)
{
  PIRSET      pirset=(PIRSET)NULL;
  PIRSET      pirset1=(PIRSET)NULL;
  SRCH_DATE   QueryDate;

  QueryDate = QueryTerm;

  switch(Relation) {

  case ZRelBefore:
  case ZRelLT:
    // MDStartDate < QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, 
			     NOT_DURING);
    break;

  case ZRelBefore_Strict:
    // MDEndDate   < QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, 
			     NOT_DURING);
    break;

  case ZRelBeforeDuring:
  case ZRelLE:
    // MDStartDate <= QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelBeforeDuring_Strict:
    // MDEndDate   <= QueryDate 
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuring:
  case ZRelEQ:
    // Match => some date in the target is inside the query
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, DURING);
    pirset1 =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, DURING);
    pirset->And(*pirset1);
    delete pirset1;
    break;

  case ZRelDuring_Strict:
    // Match => the entire target is inside the query
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, DURING);
    pirset1 =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, DURING);
    pirset->And(*pirset1);
    delete pirset1;
    break;

  case ZRelDuringAfter:
  case ZRelGE:
    // MDEndDate   >= QueryDate
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuringAfter_Strict:
    // MDStartDate >= QueryDate 
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelAfter:
  case ZRelGT:
    // MDEndDate   > QueryDate
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, 
			    NOT_DURING);
    break;

  case ZRelAfter_Strict:
    // MDStartDate > QueryDate 
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, 
			    NOT_DURING);
    break;

  default:
    break;
  }

  return pirset;
}

// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// true, otherwise, the interval end dates are searched.  Dates which equal
// the query date match if fIncludesEndpoint is true, otherwise the
// inequality is strict.
PIRSET 
INDEX::SingleDateSearchBefore(const SRCH_DATE& QueryDate, 
			      const STRING& FieldName, 
			      IntBlock FindBlock,
			      GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      YYYY=(PIRSET)NULL;
  PIRSET      YYYYMM=(PIRSET)NULL;
  PIRSET      YYYYMMDD=(PIRSET)NULL;
  SRCH_DATE   Y, YM, YMD;
  INT         Precision;
  SRCH_DATE   Today;
  PIRSET      p_today=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;
  DOUBLE      fPresent=DATE_PRESENT;

  Today.GetTodaysDate();

  Y   = QueryDate;
  YM  = QueryDate;
  YMD = QueryDate;

  Precision = QueryDate.GetPrecision();
  switch(Precision) {

  case YEAR_PREC:
    if (fIncludeEndpoint) {
      YM.PromoteToMonthEnd();
      YMD.PromoteToDayEnd();
    } else {
      YM.PromoteToMonthStart();
      YMD.PromoteToDayStart();
    }
    break;

  case MONTH_PREC:
    Y.TrimToYear();
    if (fIncludeEndpoint)
      YMD.PromoteToDayEnd();
    else
      YMD.PromoteToDayStart();
    break;

  case DAY_PREC:
    Y.TrimToYear();
    YM.TrimToMonth();
    break;

  default:
    return YYYY;
  }

  // Search the YYYY dates
  //  cout << "Search the YYYY dates" << endl;
  YYYY =
    YSearchBefore(Y, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYY->Dump();
#endif

  // Search the YYYYMM dates
  //  cout << "Search the YYYYMM dates" << endl;
  YYYYMM =
    YMSearchBefore(YM, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYYMM->Dump();
#endif

  // Search the YYYYMMDD dates
  //  cout << "Search the YYYYMMDD dates" << endl;
  YYYYMMDD =
    YMDSearchBefore(YMD, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYYMMDD->Dump();
#endif

  if (YYYYMM->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMM);
  delete YYYYMM;
  if (YYYYMMDD->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMMDD);
  delete YYYYMMDD;

  // If the query date comes after today, or is equal to today, and if
  // we're searching the interval ending points, then all of the records
  // with ending date = Present match the query and we need to get them 
  // and OR them into the result set.
  if (((Today.Equals(QueryDate)) && (FindBlock == END_BLOCK) 
       && (fIncludeEndpoint))
      || ((Today.IsBefore(QueryDate)) && (FindBlock == END_BLOCK))) {
    p_today =
      DateSearch(DAY_UPPER, FieldName, ZRelGT, SEARCH_END);
    if (p_today->GetTotalEntries() > 0) {
#ifdef DEBUG
      p_today->Dump();
#endif
      YYYY->Or(*p_today);
    }
    delete p_today;
  }

  return YYYY;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYY are matched.
PIRSET
INDEX::YSearchBefore(const SRCH_DATE& DateY, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateY.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the year dates, and maybe some negative 
    // values, if there were errors stored in the index.  We need to
    // remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYY dates - that is, values > 99.0, then take the Boolean AND
    // (intersection) of the two result sets.  We can use strict inequality 
    // because no YYYY value will be equal to the lower bound value.
    LowerBound =
      DateSearch(YEAR_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYYMM are matched.
PIRSET
INDEX::YMSearchBefore(const SRCH_DATE& DateYM, const STRING& FieldName,
		      IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYM.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMM dates, and maybe some YYYY
    // values.  We need to remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYYMM dates - that is, values > 99999.0, then take the Boolean
    // AND (intersection) of the two result sets.  We can use strict 
    // inequality because no YYYYMM value will be equal to the lower bound 
    // value.
    LowerBound =
      DateSearch(MONTH_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYYMMDD are matched.
PIRSET
INDEX::YMDSearchBefore(const SRCH_DATE& DateYMD, const STRING& FieldName,
		       IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYMD.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMMDD dates, and maybe some YYYY
    // and YYYYMM values.  We need to remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYYMMDD dates - that is, values > 9999999.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYYMMDD value will be equal to the 
    // lower bound value.
    LowerBound =
      DateSearch(DAY_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.
PIRSET 
INDEX::SingleDateSearchAfter(const SRCH_DATE& QueryDate, 
			     const STRING& FieldName, 
			     IntBlock FindBlock,
			     GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      YYYY=(PIRSET)NULL;
  PIRSET      YYYYMM=(PIRSET)NULL;
  PIRSET      YYYYMMDD=(PIRSET)NULL;
  SRCH_DATE   Y, YM, YMD;
  INT         Precision;
  //  PIRSET      p_today=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;
  //  DOUBLE      fPresent=DATE_PRESENT;

  //  Today.GetTodaysDate();

  Y   = QueryDate;
  YM  = QueryDate;
  YMD = QueryDate;

  Precision = QueryDate.GetPrecision();

  switch(Precision) {

  case YEAR_PREC:
    if (fIncludeEndpoint) {
      YM.PromoteToMonthStart();
      YMD.PromoteToDayStart();
    } else {
      YM.PromoteToMonthEnd();
      YMD.PromoteToDayEnd();
    }
    break;

  case MONTH_PREC:
    Y.TrimToYear();
    if (fIncludeEndpoint)
      YMD.PromoteToDayStart();
    else
      YMD.PromoteToDayEnd();
    break;

  case DAY_PREC:
    Y.TrimToYear();
    YM.TrimToMonth();
    break;

  default:
    return YYYY;
  }

  // Search the YYYY dates
  YYYY =
    YSearchAfter(Y, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYY->Dump();
#endif

  // Search the YYYYMM dates
  YYYYMM =
    YMSearchAfter(YM, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYYMM->Dump();
#endif

  // Search the YYYYMMDD dates
  YYYYMMDD =
    YMDSearchAfter(YMD, FieldName, FindBlock, fIncludeEndpoint);
#ifdef DEBUG
  YYYYMMDD->Dump();
#endif

  if (YYYYMM->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMM);
  delete YYYYMM;
  if (YYYYMMDD->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMMDD);
  delete YYYYMMDD;

  return YYYY;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYYMMDD are matched.
PIRSET
INDEX::YMDSearchAfter(const SRCH_DATE& DateYMD, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET    pirset=(PIRSET)NULL;
  PIRSET    UpperBound=(PIRSET)NULL;
  DOUBLE    fValue;
  DOUBLE    fPresent=DATE_PRESENT;
  SRCH_DATE Today;

  Today.GetTodaysDate();

  fValue = DateYMD.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // The result set contains hits with ending date=Present (assuming
    // the index contains records with Present dates).  The question is
    // whether they should be left in the result set or not.
    //
    // If we're searching interval starting dates, it doesn't matter what 
    // the ending date is, and only ending dates can have the value Present.
    //
    // If the query date comes before the present date, then those records
    // match the query and we're done.
    //
    // If the query date is after the present date, records with ending 
    // date = Present don't match the query and we have to trim them off.
    // Note we only have to do this if we're searching interval ending 
    // dates.
    //
    if (((Today.IsBefore(DateYMD)) && (FindBlock == END_BLOCK)) 
	|| ((Today.Equals(DateYMD)) && (FindBlock == END_BLOCK))) {
      if (fIncludeEndpoint)
	UpperBound =
	  DateSearch(DAY_UPPER, FieldName, ZRelLT, FindBlock);
      else
	UpperBound =
	  DateSearch(DAY_UPPER, FieldName, ZRelLE, FindBlock);

      //      if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //      delete UpperBound;
    }
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYYMM are matched.
PIRSET
INDEX::YMSearchAfter(const SRCH_DATE& DateYM, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET UpperBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYM.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMM dates, and maybe some bigger
    // values. We need to remove those from the result set.  
    //
    // We will search for all values less than the upper bound value
    // for YYYYMM dates - that is, values < 1000000.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYYMM value will be equal to the 
    // upper bound value.
    UpperBound =
      DateSearch(MONTH_UPPER, FieldName, ZRelLT, FindBlock);

    //    if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //    delete UpperBound;
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYY are matched.
PIRSET
INDEX::YSearchAfter(const SRCH_DATE& DateY, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET UpperBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateY.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYY dates, and maybe some bigger
    // values.  We need to remove those from the result set.  
    //
    // We will search for all values less than the upper bound value
    // for YYYY dates - that is, values < 10000.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYY value will be equal to the 
    // upper bound value.
    UpperBound =
      DateSearch(YEAR_UPPER, FieldName, ZRelLT, FindBlock);

    //    if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //    delete UpperBound;
  }

  return pirset;
}


// This is for searching interval data to match dates inside the
// interval specified by the user's query
PIRSET 
INDEX::DateRangeSearchContains(const DATERANGE& QueryDate, 
			      const STRING& FieldName,
			      IntBlock FindBlock,
			      GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      pirset  = (PIRSET)NULL;
  PIRSET      pirset1 = (PIRSET)NULL;
  SRCH_DATE   QueryStartDate, QueryEndDate;
  DATERANGE   TmpDate = QueryDate;
  //INT         Precision;
  SRCH_DATE   Today;
  PIRSET      p_today=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;
  DOUBLE      fPresent=DATE_PRESENT;

  Today.GetTodaysDate();

  QueryStartDate = TmpDate.GetStart();
  QueryEndDate   = TmpDate.GetEnd();

  // FindBlock says whether we're looking for indexed start dates or
  // end dates (or even global pointer).
  // 
  // To be in the interval, the target date has to be before the ending
  // date of the query interval
  pirset =
    SingleDateSearchBefore(QueryEndDate, FieldName, FindBlock,
				 fIncludeEndpoint);

  // And the target date has to be after the starting date of the query
  pirset1 =
    SingleDateSearchAfter(QueryStartDate, FieldName, FindBlock,
				fIncludeEndpoint);

  // For a match, the target date has to be in both result sets
  pirset->And(*pirset1);
  delete pirset1;

  return pirset;
}


// This is the routine which does all the heavy lifting in the date
// indexes
PIRSET 
INDEX::DateSearch(const DOUBLE fKey, const STRING& FieldName, 
		  INT4 Relation, IntBlock FindBlock) 
{
  STRING       FieldType;
  PIRSET       pirset;
  SearchState  Status=NO_MATCH;
  INT4         Start=0, End=-1, Pointer=0, ListCount, Value;
  INT4         w;
  IRESULT      iresult;
  STRING       Fn;
  INTERVALLIST List;
  MDT*        ThisMdt;  

  pirset=new IRSET(Parent);
  if (!pirset)
    return (PIRSET)NULL; // new failed...

  Parent->FieldTypes.GetValue(FieldName,&FieldType);
  
  /*  We'll fix the rset cache when we can feed the server name to it
      STRING DBName="",T1;
      CHR    TempBuffer[256];
      sprintf(TempBuffer,"%f",fKey);
      T1 = TempBuffer;
      w = SetCache->Check(T1,Relation,FieldName,DBName);

      if(w > -1) {
      delete pirset;
      return(SetCache->Fetch(w));
      }
  */

  Parent->DfdtGetFileName(FieldName, &Fn);

#ifdef DEBUG
  INT4 Dummy;
  List.SetFileName(Fn);
  switch (FindBlock) {
  case START_BLOCK:
    List.LoadTable(-1,-1,START_BLOCK);
    break;
  case END_BLOCK:
    List.LoadTable(-1,-1,END_BLOCK);
    break;
  case PTR_BLOCK:
    List.LoadTable(-1,-1,PTR_BLOCK);
  }
  List.Dump();
#endif


  if ((FieldType.CaseEquals("DATE-RANGE")) 
      || (FieldType.CaseEquals("DATE"))) {


    switch(Relation) {
    case ZRelEQ:		// equals
      // Start is the smallest index in the table 
      // for which fKey is <= to the table value
      Status = List.Find(Fn, fKey, ZRelLE, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)   // No matching values - bail out
	break;
      // End is the largest index in the table for which
      // fKey is >= to the table value;
      Status = List.Find(Fn, fKey, ZRelGE, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelLT:		// less than
    case ZRelLE:		// less than or equal to
      // Start at the beginning of the table
      Start=0;
      // End is the largest index in the table for which 
      // fKey is <= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelEnclosedWithin:
      Status = List.Find(Fn, fKey, ZRelGT, FindBlock, &End);
      if (Status == TOO_LOW)   // We ran off the low end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)
	break;
      Status = List.Find(Fn, fKey, ZRelLT, FindBlock, &Start);
      if (Status == TOO_HIGH)    // We ran off the top end without a match
	Status = NO_MATCH;
      break;

    case ZRelGE:		// greater than or equal to
    case ZRelGT:		// greater than
      // Go to the end of the table
      End=-1;
      // Find the smallest index for which fKey is >= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      break;
    }
  }

  // Bail out if we didn't find the value we were looking for
  if (Status == NO_MATCH)
    return pirset;

  List.SetFileName(Fn);

  switch (FindBlock) {
  case START_BLOCK:
    List.LoadTable(Start,End,START_BLOCK);
    break;

  case END_BLOCK:
    List.LoadTable(Start,End,END_BLOCK);
    break;

  case PTR_BLOCK:
    List.LoadTable(Start,End,PTR_BLOCK);
    break;
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

  return(pirset);
}
