/* $Id: geosearch.cxx,v 1.3 1998/05/12 16:49:04 cnidr Exp $ */
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
File:		geosearch.cxx
Version:	$Revision: 1.3 $
Description:	Class INDEX - spatial search methods
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

PIRSET 
INDEX::BoundingRectangle(DOUBLE NorthBC,
			 DOUBLE SouthBC,
			 DOUBLE WestBC,
			 DOUBLE EastBC)
{
  
  // rationale:  if any of the 4 intervals constructed from
  // this set of points intersects a target interval in
  // the database, our BoundingRectangle overlaps the
  // target rectangle that contains the interval.
  
  // there are certain special cases we need to worry about.
  // One: a target that overlaps the International Date Line
  // I don't know how to deal with that except in the indexer, so
  // I won't worry with it right now.  It's fixable, though.
  // Two:  a query that overlaps the International Data line.
  // For that case, we break the query into two parts (8 intervals)
  // Search separately, and OR the results
  // We have to adjust the boundaries to make sure the "direction" is
  // correct.
  // don't be confused by terminology - the North *longitudinal* boundary
  // is the East/West longitudinal line defined by the coordinate points
  // that is furthest North.
  
  // we must add code to find all that are entirely enclosed by the query.
  
  
  GDT_BOOLEAN DateLineIntersection=GDT_FALSE;
  
  // remember the terminology note!
  
  PIRSET NorthLongitude;
  PIRSET SouthLongitude;
  PIRSET EastLatitude;
  PIRSET WestLatitude;
  STRING FieldName;
  
  
  // first, build a result set of all the totally enclosed
  // items in the database.
  // all < NorthQuery
  //  AND
  // all> SouthQuery
  //  AND
  // all < EastQuery
  //  AND
  // all > WestQuery
  
  PIRSET LessThanNorth;
  PIRSET MoreThanSouth;
  PIRSET LessThanEast;
  PIRSET MoreThanWest;
  
  FieldName="NORTHBC";
  LessThanNorth=NumericSearch(NorthBC,FieldName,1);
  
  FieldName="EASTBC";
  LessThanEast=NumericSearch(EastBC,FieldName,1);
  
  LessThanNorth->And(*LessThanEast);
  delete LessThanEast;
  
  FieldName="SOUTHBC";
  MoreThanSouth=NumericSearch(SouthBC,FieldName,4);
  
  FieldName="WESTBC";
  MoreThanWest=NumericSearch(WestBC,FieldName,4);
  
  MoreThanSouth->And(*MoreThanWest);
  delete MoreThanWest;
  
  LessThanNorth->And(*MoreThanSouth);
  delete MoreThanSouth;
  
  
  // Northernmost longitudinal boundary:
  
  if (WestBC > EastBC)	// we cross the DateLine
    DateLineIntersection=GDT_TRUE;
  else
    DateLineIntersection=GDT_FALSE;
  
  PIRSET WestDateLineNL;
  PIRSET EastDateLineNL;
  if (DateLineIntersection == GDT_TRUE) {
    WestDateLineNL
      =Interval(WestBC, 180.0, NorthBC, NorthBC);
    EastDateLineNL
      =Interval(-180.0, EastBC, NorthBC, NorthBC);
    WestDateLineNL->Or(*EastDateLineNL);
    NorthLongitude=WestDateLineNL;
    //    delete WestDateLineNL;
    delete EastDateLineNL;
    
  } else
    NorthLongitude=Interval(WestBC, EastBC, NorthBC, NorthBC);
  
  // we now have a north longitude result set.  
  // Southernmost longitudinal boundary:
  
  if (WestBC > EastBC)	// we cross the DateLine
    DateLineIntersection=GDT_TRUE;
  else
    DateLineIntersection=GDT_FALSE;
  
  PIRSET WestDateLineSL;
  PIRSET EastDateLineSL;
  if (DateLineIntersection == GDT_TRUE) {
    WestDateLineSL
      =Interval(WestBC, 180.0, SouthBC, SouthBC);
    EastDateLineSL
      =Interval(-180.0, EastBC, SouthBC, SouthBC);
    WestDateLineSL->Or(*EastDateLineSL);
    SouthLongitude=WestDateLineSL;
    //    delete WestDateLineSL;
    delete EastDateLineSL;
    
  } else
    SouthLongitude=Interval(WestBC, EastBC, SouthBC, SouthBC);
  
  // our north and south longitudinal boundary intersections
  // have been computed.
  //
  // now compute intersections for eastern and western latitudinal 
  // latitudinal boundaries.  I don't deal with the Poles.  Queries
  // can't overlap the Polar regions.  I can fix that...
  
  EastLatitude=Interval(EastBC, EastBC, SouthBC, NorthBC);
  WestLatitude=Interval(WestBC, WestBC, SouthBC, NorthBC);
  
  // OR these buzzards together for a set of hits...
  
  EastLatitude->Or(*WestLatitude);
  delete WestLatitude;
  EastLatitude->Or(*NorthLongitude);
  delete NorthLongitude;
  EastLatitude->Or(*SouthLongitude);
  delete SouthLongitude;
#ifdef DEBUG
  printf("%i Hits from Rect\n", EastLatitude->GetTotalEntries());
#endif
  EastLatitude->Or(*LessThanNorth);
  delete LessThanNorth;
  
  return(EastLatitude);	// our combined set of hits...
}


// this functions takes a pair of points forming an interval
// and match them to intervals in the database.
// option - pass the field names with the values.
// We assume that the West Longitude is <= East Longitude
// and South Latitude <=North Latitude

PIRSET 
INDEX::Interval(DOUBLE WestLongitude, DOUBLE EastLongitude,
		DOUBLE SouthLatitude, DOUBLE NorthLatitude)
{
  // goal - find intervals in the database that intersect this interval.
  
  
//  CHR TempBuffer[256];
  PIRSET ResultA;
  PIRSET ResultB;
  PIRSET ResultC;
  PIRSET ResultD;
  STRING Query,FieldName;
  
  // Put a cacheing structure here to avoid search duplication
  FieldName="WESTBC";
  ResultA=NumericSearch(EastLongitude,FieldName,2); //LTE
  
#ifdef DEBUG
  printf("Got %i entries <= %i in ", ResultA->GetTotalEntries(),
	 EastLongitude);
  FieldName.Print();
  print("\n");
#endif
  
  if (ResultA->GetTotalEntries() == 0) {
    // no hits rules out entire search
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultA);
  }  
  
  FieldName="EASTBC";
  ResultB=NumericSearch(WestLongitude,FieldName,4); //GTE
  
#ifdef DEBUG
  printf("Got %i entries >= %i in ", ResultB->GetTotalEntries(),
	 WestLongitude);
  FieldName.Print();
  printf("\n");
#endif
  
  if (ResultB->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultB);
  }
  
  // two valid result sets.  AND them
  
  ResultA->And(*ResultB);
  delete ResultB;
  if (ResultA->GetTotalEntries() == 0) {
    // no hits rules out entire search
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultA);
  }
#ifdef DEBUG
  print("Now have %i entries in range.\n", ResultA->GetTotalEntries());
#endif
  
  // ResultA Now contains a valid interval set
  
  FieldName="NORTHBC";
  ResultC=NumericSearch(SouthLatitude,FieldName,4); //GTE
  
#ifdef DEBUG
  printf("Got %i entries >= %i in ", ResultC->GetTotalEntries(),
	SouthLatitude);
  FieldName.Print();
  printf("\n");
#endif
  
  if (ResultC->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultC);
  }
  
  FieldName="SOUTHBC";
  
  ResultD=NumericSearch(NorthLatitude,FieldName,2); //LTE
  
#ifdef DEBUG
  printf("Got %i entries <= %i in ", ResultD->GetTotalEntries(),
	 NorthLatitude);
  FieldName.String();
  printf("\n");
#endif
  
  if (ResultD->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
    delete ResultC;
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultD);
  }
  ResultC->And(*ResultD);
#ifdef DEBUG
  printf("Now have %i entries in range.\n",
	 ResultC->GetTotalEntries());
#endif
  delete ResultD;
  
  if (ResultC->GetTotalEntries() == 0) {
    // no hits rules out entire search
#ifdef DEBUG
    printf("No hits in last search, so bailing out\n");
#endif
    return(ResultC);
  }
  
  // ResultA and ResultC contain our Lat/Long intrsections.  AND them
  ResultA->And(*ResultC);
  delete ResultC;
#ifdef DEBUG
  printf("Now have %i entries in range, heading back\n",
	 ResultA->GetTotalEntries());
#endif
  return(ResultA);		// full 'o hits (maybe)
  
}

