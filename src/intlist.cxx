/* $Id: intlist.cxx,v 1.18 2000/10/15 03:46:31 cnidr Exp $ */
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

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		intlist.cxx
Version:	1.00
$Revision: 1.18 $
Description:	Class INTERVALLIST
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
                Derived from class NLIST by J. Fullton
@@@*/

#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "intlist.hxx"

// Prototypes
static INT 
SortStartCmp(const void* x, const void* y);

static INT 
SortEndCmp(const void* x, const void* y);

static INT 
SortGPCmp(const void* x, const void* y);



/// Constructs an empty list with the default 3-coordinate entry shape.
INTERVALLIST::INTERVALLIST()
{
  Ncoords    = 3;
  table      = new INTERVALFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  // BUGFIX #2 (docs/BUG_CATALOG.md#srcintlisthxx): Attribute/Relation
  // (INTERVALLIST's own members, shadowing NUMERICLIST's identically-
  // named ones -- see the shadowing note in docs/BUG_CATALOG.md) were
  // left indeterminate by both constructors, the same "indeterminate
  // primitive member" category already fixed for the base class's own
  // copies in src/nlist.cxx's BUGFIX #2.
  Attribute  = 0;
  Relation   = 0;
}


/// Constructs an empty list whose entries hold n coordinate values.
INTERVALLIST::INTERVALLIST(INT n)
{
  Ncoords    = n;
  table      = new INTERVALFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  // BUGFIX #2, second constructor -- see the default constructor above.
  Attribute  = 0;
  Relation   = 0;
}


INT4
INTERVALLIST::LoadTable(INT4 Start, INT4 End)
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // This is only called when the table has been written out during the
  // index creation phase, before the file has been sorted (with both
  // components dumped)
  //
  // Return the number of items read
{
  FILE  *fp;
  INT4   high;
  INT4   x, i;
  DOUBLE value;
  INT4   nRecs=0;
  
  if (!(FileName.GetLength())) {
    fprintf(stderr, "FileName not set\n");
    RETURN_ZERO;
  }

  if (End == -1)
    End = 999999999;
  if (Start == -1)
    Start = 0;
  
  fp = fopen(FileName,"rb");
  if (fp) {
    fseek(fp, (long)( Start * (sizeof(INT4) + sizeof(DOUBLE)) ), SEEK_SET);

    nRecs = 0;
    for (i=Start; i<=End; i++) {
      x = fread((char*)&high, 1, sizeof(INT4), fp); // explicit cast
      if(x == 0)
	break;
      table[Count].SetGlobalStart(high);
      x = fread((char*)&value, 1, sizeof(DOUBLE), fp); // explicit cast
      table[Count].SetStartValue(value);
      x = fread((char*)&value, 1, sizeof(DOUBLE), fp); // explicit cast
      table[Count].SetEndValue(value);
      Count++;
      nRecs++;
      if(Count == MaxEntries)
	Expand();
    }
    fclose(fp);
    Cleanup();
  }
  return nRecs;
}


INT4
INTERVALLIST::LoadTable(INT4 Start, INT4 End, IntBlock Offset)
{
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // Return the actual number of items read in this load

  FILE  *fp;
  INT4   high;
  INT4   x, i;
  DOUBLE fStart, fEnd;
  INT4   nCount, nRecs=0;
  LONG   MoveTo;
  
  if (!(FileName.GetLength())) {
    fprintf(stderr, "FileName not set\n");
    RETURN_ZERO;
  }
  
  fp = fopen(FileName,"rb");
  if (fp) {
    // Bump past Count, then offset to the right starting point
    // Note - there are two tables in the file - one sorted by starting
    // value, one sorted by ending value.  There are Count entries in each
    // version.  
    x = fread((char*)&nCount, 1, sizeof(INT4), fp); // explicit cast

    if ((End >= nCount) || (End < 0))
      End = nCount-1;
    if (Start < 0)
      Start = 0;
  
    if (x != 0) {
      if (Offset == START_BLOCK) {
	MoveTo = sizeof(INT4)
	  + (Start) * ( sizeof(INT4) + (2*sizeof(DOUBLE)) );
      } else if (Offset == END_BLOCK) {
	MoveTo = sizeof(INT4)
	  + (nCount+Start) * ( sizeof(INT4) + (2*sizeof(DOUBLE)) );
      } else {
	MoveTo = sizeof(INT4)
	  + ((2*nCount)+Start) * ( sizeof(INT4) + (2*sizeof(DOUBLE)) );
      }

#ifdef DEBUG
      cerr << "Moving " << MoveTo << " bytes into the file and reading "
	   << nCount << " elements starting at table[" << Count << "]"
	   << endl;
#endif

      fseek(fp, MoveTo, SEEK_SET);

      nRecs = 0;
      for (i=Start; i<=End; i++) {
	x = fread((char*)&high, 1, sizeof(INT4), fp);     // explicit cast
	table[Count].SetGlobalStart(high);
	if (x == 0)
	  break;
	x = fread((char*)&fStart, 1, sizeof(DOUBLE), fp); // explicit cast
	table[Count].SetStartValue(fStart);
	x = fread((char*)&fEnd, 1, sizeof(DOUBLE), fp);   // explicit cast
	table[Count].SetEndValue(fEnd);

#ifdef DEBUG
	cerr << "table[" << Count << "]=(" << high << ", " << fStart
	     << ", " << fEnd << ")" << endl;
#endif

	Count++;
	nRecs++;
	if(Count == MaxEntries)
	  Expand();
      }
    }
    fclose(fp);
    Cleanup();
  }
  return nRecs;
}


void 
INTERVALLIST::WriteTable()
{
  FILE  *fp;
  INT4   x;
  INT4   Val;
  DOUBLE fStart, fEnd;
  
  if (!(FileName.GetLength())) {
    fprintf(stderr, "FileName not set\n");
    return;
  }

  fp = fopen(FileName,"wb");
  if (fp) {
    for (x=0; x<Count; x++) {
      Val    = table[x].GetGlobalStart();
      fStart = table[x].GetStartValue();
      fEnd   = table[x].GetEndValue();
      fwrite((char*)&Val,    1, sizeof(INT4),   fp); // explicit cast
      fwrite((char*)&fStart, 1, sizeof(DOUBLE), fp); // explicit cast
      fwrite((char*)&fEnd,   1, sizeof(DOUBLE), fp); // explicit cast
    }
  }
  fclose(fp);
}


void 
INTERVALLIST::WriteTable(INT Offset)
{
  FILE  *fp;
  INT4   x;
  INT4   Val;
  DOUBLE fStart, fEnd;
  LONG   MoveTo, nBytes;
  
  if (!(FileName.GetLength())) {
    fprintf(stderr, "FileName not set\n");
    return;
  }
  fp = fopen(FileName,"a+b");

  if (fp) {
    // First, write out the count
    if (Offset == 0) {
      fwrite((char*)&Count, 1, sizeof(INT4), fp);
    }

    // Now, go to the specified offset and write the table
    nBytes = sizeof(INT4) + (2 * sizeof(DOUBLE));
    MoveTo = (LONG)((Offset*nBytes) + sizeof(INT4));

    //    cout << "Offsetting " << MoveTo << " bytes into the file." << endl;

    if (fseek(fp, MoveTo, SEEK_SET) == 0) {
      for (x=0; x<Count; x++) {
	Val    = table[x].GetGlobalStart();
	fStart = table[x].GetStartValue();
	fEnd   = table[x].GetEndValue();
	fwrite((char*)&Val,    1, sizeof(INT4),   fp);
	fwrite((char*)&fStart, 1, sizeof(DOUBLE), fp);
	fwrite((char*)&fEnd,   1, sizeof(DOUBLE), fp);
	//	cout << "Wrote " << nBytes << " bytes." << endl;
	//	cout << "gp=" << Val;
	//	cout << ", [" << fStart;
	//	cout << ", " << fEnd << "]" << endl;

      }
    }
  }
  fclose(fp);
}


DOUBLE 
INTERVALLIST::GetStartMaxValue() {
  INT4 x;
  DOUBLE y,yy;
  y = GetStartValue(0);
  for(x=1; x<Count; x++) {
    yy= GetStartValue(x);
    if (yy > y)
      y = yy;
  }
  return y;
}


DOUBLE 
INTERVALLIST::GetStartMinValue()
{
  INT4 x;
  DOUBLE y,yy;
  y = GetStartValue(0);
  for(x=1; x<Count; x++) {
    yy= GetStartValue(x);
    if (yy < y)
      y = yy;
  }
  return y;
}


DOUBLE 
INTERVALLIST::GetEndMaxValue() {
  INT4 x;
  DOUBLE y,yy;
  y = GetEndValue(0);
  for(x=1; x<Count; x++) {
    yy= GetEndValue(x);
    if (yy > y)
      y = yy;
  }
  return y;
}


DOUBLE 
INTERVALLIST::GetEndMinValue() {
  INT4 x;
  DOUBLE y,yy;
  y = GetEndValue(0);
  for(x=1; x<Count; x++) {
    yy= GetEndValue(x);
    if (yy < y)
      y = yy;
  }
  return y;
}


void 
INTERVALLIST::SortByStart()
{
  qsort((void *)table,Count, sizeof(INTERVALFLD),SortStartCmp);
}


void 
INTERVALLIST::SortByEnd()
{
  qsort((void *)table,Count, sizeof(INTERVALFLD),SortEndCmp);
}


void 
INTERVALLIST::SortByGP()
{
  qsort((void *)table,Count, sizeof(INTERVALFLD),SortGPCmp);
}


static INT 
SortStartCmp(const void* x, const void* y) 
{
  DOUBLE a;
  a=((*((PINTERVALFLD)x)).GetStartValue()) -
    ((*((PINTERVALFLD)y)).GetStartValue()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT 
SortEndCmp(const void* x, const void* y) 
{
  DOUBLE a;
  a=((*((PINTERVALFLD)x)).GetEndValue()) -
    ((*((PINTERVALFLD)y)).GetEndValue()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT 
SortGPCmp(const void* x, const void* y) 
{
  INT4 a;
  a=((*((PINTERVALFLD)x)).GetGlobalStart()) -
    ((*((PINTERVALFLD)y)).GetGlobalStart()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


SearchState
INTERVALLIST::IntervalMatcher(DOUBLE Key, DOUBLE LowerBound, DOUBLE Mid, 
			      DOUBLE UpperBound, INT4 Relation, IntType Type)
  // Return TOO_LOW if the key value is less than the lower bound
  // Return TOO_HIGH if the key value is greater than the upper bound
  // Return MATCH if the key value is between the bounds
{
	
  switch (Relation)
    {
    case ZRelGE:		        // greater than or equals
      if ((Mid>=Key) && (Type==AT_START || Key>LowerBound))
	return(MATCH);		        // exact place - lower boundary
      else if (Key<=LowerBound)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelGT:		        // greater than
      if ((Mid>Key) && (Type==AT_START || Key>=LowerBound))
	return(MATCH);		        // exact place - lower boundary
      else if (Key<LowerBound)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelLE:		        // less than or equals
      //      if ((Mid<=Key) && (Type==AT_END || UpperBound>Key))
      if (Key>UpperBound)
	return(TOO_HIGH);
      else if ((Type==AT_END) && (Mid<=Key))
	return(MATCH);
      else if ((Mid<=Key) && (Key<UpperBound))
	return(MATCH);		        // exact place - upper boundary
      else if (Key>=UpperBound)
	return(TOO_HIGH);
      else
	return(TOO_LOW);

    case ZRelLT:		        // less than
      if ((Mid<Key) && (Type==AT_END || Key<=UpperBound))
	return(MATCH);		        // exact place - upper boundary
      else if (Key>UpperBound)
	return(TOO_HIGH);
      else
	return(TOO_LOW);
    }

  fprintf(stderr, "Hideous Matching Error\n");
  return(NO_MATCH);
}


SearchState
INTERVALLIST::IntervalMatcher(INT4 Key, INT4 LowerBound, INT4 Mid, 
			      INT4 UpperBound, INT4 Relation, IntType Type)
  // Return TOO_LOW if the key value is less than the lower bound
  // Return TOO_HIGH if the key value is greater than the upper bound
  // Return MATCH if the key value is between the bounds
{
	
  switch (Relation)
    {
    case ZRelGE:		        // greater than or equals
      if ((Mid>=Key) && (Type==AT_START || LowerBound<Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>=Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelGT:		        // greater than
      if ((Mid>Key) && (Type==AT_START || LowerBound<=Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelLE:		        // less than or equals
      //      if ((Mid<=Key) && (Type==AT_END || UpperBound>Key))
      if (UpperBound<Key)
	return(TOO_HIGH);
      else if ((Type==AT_END) && (Mid<=Key))
	return(MATCH);
      else if ((Mid<=Key) && (UpperBound>Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<=Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);

    case ZRelLT:		        // less than
      if ((Mid<Key) && (Type==AT_END || UpperBound>=Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);
    }

  fprintf(stderr, "Hideous Matching Error\n");
  return(NO_MATCH);
}


// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState 
INTERVALLIST::Find(STRING Fn, DOUBLE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(Fn, Key, Relation, FindBlock, Index);
  return Status;
}


SearchState 
INTERVALLIST::Find(STRING Fn, INT4 Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(FileName, Key, Relation, FindBlock, Index);
  return Status;
}


SearchState 
INTERVALLIST::Find(DOUBLE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, FindBlock, Index);
  return Status;
}


SearchState 
INTERVALLIST::Find(INT4 Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, FindBlock, Index);
  return Status;
}


SearchState 
INTERVALLIST::MemFind(DOUBLE Key, INT4 Relation, 
		      IntBlock FindBlock, INT4 *Index)
{
  return NO_MATCH;
}


SearchState 
INTERVALLIST::MemFind(INT4 Key, INT4 Relation, 
		      IntBlock FindBlock, INT4 *Index)
{
  return NO_MATCH;
}


SearchState 
INTERVALLIST::DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index)
{

  INT4   Nrecs;
  DOUBLE LowerBound, UpperBound;
  PFILE  Fp = fopen(Fn, "rb");
  INT    ElementSize;

  // These count entries, not offsets
  INT Total;
  INT Low = 0;
  INT High, X, OX;
  
  SearchState State;
  IntType     Type=INSIDE;
  INT4        GpS, Dummy;
  DOUBLE      NumericValue;
  DOUBLE      Hold;         // This is the interval boundary - we don't use it
  INT4        Offset;       // Offset needed to read the element
  DOUBLE      StartValue, EndValue;

  if (!Fp) {
    perror(Fn);
    *Index = -1;
    return NO_MATCH;

  } else {

    if (fread((char*)&Total,1,sizeof(INT4),Fp) != sizeof(INT4)) {
      fclose(Fp);
      *Index = -1;
      return NO_MATCH;
    }
    ElementSize = sizeof(INT4) + 2*sizeof(DOUBLE);
    High = Total - 1;
    X = High / 2;

    switch(FindBlock) {

    case START_BLOCK:

      // Get the starting value
      Offset = sizeof(INT4);
      fseek(Fp, (long)Offset, SEEK_SET);
      if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
          fread((char *)&StartValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
          fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
        fclose(Fp);
        *Index = -1;
        return NO_MATCH;
      }

      // Get the ending value
      Offset = sizeof(INT4) + High * ElementSize;
      fseek(Fp, (long)Offset, SEEK_SET);
      if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
          fread((char *)&EndValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
          fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
        fclose(Fp);
        *Index = -1;
        return NO_MATCH;
      }
#ifdef DEBUG
      cerr << "DiskFind: START_BLOCK - Looking for " << (INT)Key << " between " 
	   << (INT)StartValue << " and " << (INT)EndValue << endl;
#endif
      State = IntervalMatcher(Key, StartValue, StartValue, EndValue, 
				Relation, Type);
      if (State != MATCH) {
#ifdef DEBUG
	cerr << "Didn't match" << endl;
#endif
	*Index = -1;
	return State;
      }
#ifdef DEBUG
      else {
	cerr << "Matched" << endl;
      }
#endif

      // Search the index sorted by starting interval values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  Offset = sizeof(INT4) + (X-1) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=INSIDE; // We're inside the interval

	} else if (X <= 0) {
	  Offset = sizeof(INT4);
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_START; // We're at the lower boundary

	} else if (X >= High) {
	  Offset = sizeof(INT4) + (High) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START) {
	  if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&LowerBound, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	    fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  LowerBound = NumericValue;

	if (Type != AT_END) {
	  if (fread((char *)&Dummy, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&UpperBound, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  UpperBound = NumericValue;

#ifdef DEBUG
	cerr << "Comparing key " << (INT)Key << " with " << (INT)LowerBound 
	     << ", " << (INT)NumericValue << " and " << (INT)UpperBound << endl;
#endif

	State = IntervalMatcher(Key, LowerBound, NumericValue, UpperBound, 
				Relation, Type);
#ifdef DEBUG
	cerr << State << endl;
#endif

	if (State == MATCH) {
	  // We got a match
#ifdef DEBUG
	  cerr << "Got match at index " << X << endl;
	  Offset = sizeof(INT4) + (X-2) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) break;
	  cerr << "The previous value is " << (INT)NumericValue;
	  if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) break;
	  cerr << ", the matched value is " << (INT)NumericValue;
	  if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) break;
	  cerr << " and the next value is " << (INT)NumericValue;	  
#endif

	  fclose(Fp);
	  *Index = X;
	  return MATCH;
	} else if ((State == TOO_HIGH) && (Type == AT_END)) {
	  // We didn't get a match, but we ran off the upper end, so
	  // the key is bigger than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  cerr << "No match at index " << X << ", State=TOO_HIGH" << endl;
	  return State;
	} else if ((State == TOO_LOW) && (Type == AT_START)) {
	  // We didn't get a match, but we ran off the lower end, so 
	  // the key is smaller than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  cerr << "No match at index " << X << ", State=TOO_LOW" << endl;
	  return State;
	} else if (Low >= High) {
	  // If Low is >= High, there aren't any more values to check
	  // so we're done whether we got a match or not, and if we got
	  // here, there wasn't a match.  This probably won't happen - 
	  // at least, we expect that these conditions will be caught
	  // by one of the preceeding, but it pays to be safe.
	  fclose(Fp);
	  *Index = -1;
	  cerr << "No match at index " << X << ", Low>High" << endl;
	  return NO_MATCH;
	}

	if (State == TOO_LOW) {
	  High = X;
	} else {
	  Low = X + 1;
	}

	X = (Low + High) / 2;
	if (X < 0) {
	  X = 0;
	} else {
	  if (X >= Total) {
	    X = Total - 1;
	  }
	}
      } while (X != OX);
      break;
      //    } else {
    case END_BLOCK:
      // Search the index sorted by ending interval values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  Offset = sizeof(INT4) + (Total+X-1) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=INSIDE; // We're inside the interval

	} else if (X <= 0) {
	  Offset = sizeof(INT4) + (Total+X) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_START; // We're at the lower boundary

	} else if (X >= High) {
	  Offset = sizeof(INT4) + (Total+X-1) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START) {
	  if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&LowerBound, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	    fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  LowerBound = NumericValue;

	if (Type != AT_END) {
	  if (fread((char *)&Dummy, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&UpperBound, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  UpperBound = NumericValue;
      
#ifdef DEBUG
	cerr << "DiskFind: END_BLOCK - Comparing key " << Key << " with " << LowerBound 
	     << ", " << NumericValue << " and " << UpperBound << endl;
#endif

	State = IntervalMatcher(Key, LowerBound, NumericValue, 
				UpperBound, Relation, Type);
#ifdef DEBUG
	cerr << State << endl;
#endif

	if (State == MATCH) {
	  // We found a match
	  fclose(Fp);
	  *Index = X;
	  return MATCH;
	} else if ((State == TOO_HIGH) && (Type == AT_END)) {
	  // We didn't get a match, but we ran off the upper end, so
	  // the key is bigger than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  return State;
	} else if ((State == TOO_LOW) && (Type == AT_START)) {
	  // We didn't get a match, but we ran off the lower end, so 
	  // the key is smaller than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  return State;
	} else if (Low >= High) {
	  // If Low is >= High, there aren't any more values to check
	  // so we're done whether we got a match or not, and if we got
	  // here, there wasn't a match.  This probably won't happen - 
	  // at least, we expect that these conditions will be caught
	  // by one of the preceeding, but it pays to be safe.
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}

	if (State == TOO_LOW) {
	  High = X;
	} else {
	  Low = X + 1;
	}

	X = (Low + High) / 2;
	if (X < 0) {
	  X = 0;
	} else {
	  if (X >= Total) {
	    X = Total - 1;
	  }
	}
      } while (X != OX);
      break;

    case PTR_BLOCK:
      // Search the index sorted by global pointer values
      break;
    }
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


SearchState 
INTERVALLIST::DiskFind(STRING Fn, INT4 Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index)
{

  INT4        Nrecs;
  PFILE       Fp = fopen(Fn, "rb");
  INT         ElementSize;
  SearchState State;
  IntType     Type=INSIDE;
  INT4        GpS;
  INT4        LowerBound, UpperBound, NumericValue;
  DOUBLE      Dummy;
  DOUBLE      Hold;         // This is the interval boundary - we don't use it
  INT4        Offset;       // Offset needed to read the element


  // These count entries, not offsets
  INT Total;
  INT Low = 0;
  INT High, X, OX;
  
  if (!Fp) {
    perror(Fn);
    *Index = -1;
    return NO_MATCH;

  } else {

    if (fread((char*)&Total,1,sizeof(INT4),Fp) != sizeof(INT4)) {
      fclose(Fp);
      *Index = -1;
      return NO_MATCH;
    }
    ElementSize = sizeof(INT4) + 2*sizeof(DOUBLE);
    High = Total - 1;
    X = High / 2;

    //    if (ByStart) {
    switch(FindBlock) {

    case START_BLOCK:
      // Search the index sorted by starting interval values
      break;

    case END_BLOCK:
      // Search the index sorted by ending interval values
      break;

    case PTR_BLOCK:
      // Search the index sorted by global pointer values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  Offset = sizeof(INT4) + ((2*Total)+(X-1)) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=INSIDE; // We're inside the interval

	} else if (X <= 0) {
	  Offset = sizeof(INT4) + ((2*Total)+X) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_START; // We're at the lower boundary

	} else if (X >= High) {
	  Offset = sizeof(INT4) + ((2*Total)+(X-1)) * ElementSize;
	  fseek(Fp, (long)Offset, SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START) {
	  if (fread((char *)&LowerBound, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	if (fread((char *)&NumericValue, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	    fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  LowerBound = NumericValue;

	if (Type != AT_END) {
	  if (fread((char *)&UpperBound, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	      fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE) ||
	      fread((char *)&Hold, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	    fclose(Fp);
	    *Index = -1;
	    return NO_MATCH;
	  }
	}

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  UpperBound = NumericValue;

#ifdef DEBUG
	cerr << "Comparing key " << Key << " with " << LowerBound 
	     << ", " << NumericValue << " and " << UpperBound << endl;
#endif

	State = IntervalMatcher(Key, LowerBound, NumericValue, UpperBound, 
				Relation, Type);
#ifdef DEBUG
	cerr << State << endl;
#endif

	if (State == MATCH) {
	  // We got a match
	  fclose(Fp);
	  *Index = X;
	  return MATCH;
	} else if ((State == TOO_HIGH) && (Type == AT_END)) {
	  // We didn't get a match, but we ran off the upper end, so
	  // the key is bigger than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  return State;
	} else if ((State == TOO_LOW) && (Type == AT_START)) {
	  // We didn't get a match, but we ran off the lower end, so 
	  // the key is smaller than anything indexed
	  fclose(Fp);
	  *Index = -1;
	  return State;
	} else if (Low >= High) {
	  // If Low is >= High, there aren't any more values to check
	  // so we're done whether we got a match or not, and if we got
	  // here, there wasn't a match.  This probably won't happen - 
	  // at least, we expect that these conditions will be caught
	  // by one of the preceeding, but it pays to be safe.
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}

	if (State == TOO_LOW) {
	  High = X;
	} else {
	  Low = X + 1;
	}

	X = (Low + High) / 2;
	if (X < 0) {
	  X = 0;
	} else {
	  if (X >= Total) {
	    X = Total - 1;
	  }
	}
      } while (X != OX);
      break;
    }
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


void 
INTERVALLIST::Dump()
{
  INT4 x;
  INT4 a;
  DOUBLE b,c;

  for(x=0; x<Count; x++) {
    a = table[x].GetGlobalStart();
    b = table[x].GetStartValue();
    c = table[x].GetEndValue();
    printf("ptr: %i [%f,%f]\n", a, b, c);
  }
}


void 
INTERVALLIST::Dump(INT4 start, INT4 end)
{
  INT4 x;
  INT4 a;
  DOUBLE b,c;

  if (start < 0)
    start = 0;
  if (end > Count)
    end = Count;

  for(x=start; x<end; x++) {
    a = table[x].GetGlobalStart();
    b = table[x].GetStartValue();
    c = table[x].GetEndValue();
    printf("ptr: %i [%f,%f]\n", a, b, c);
  }
}


void 
INTERVALLIST::Resize(INT4 Entries)
{
  PINTERVALFLD temp=new INTERVALFLD[Entries];
  INT4 CopyCount;
  INT4 x;

  if(Entries>Count)
    CopyCount=Count;
  else {
    CopyCount=Entries;
    Count=Entries;
  }
  for(x=0; x<CopyCount; x++) {
    temp[x]=table[x];
  }
  if(table)
    delete [] table;
  table=temp;
  MaxEntries=Entries;
}


INTERVALLIST::~INTERVALLIST()
{
  if(table)
    delete [] table;
}


/*  Inlined
  void 
  INTERVALLIST::SetCoords(INT n) {
  Ncoords = n;
  }


  INT 
  INTERVALLIST::GetCoords() {
  return Ncoords;
  }


  INT4 
  INTERVALLIST::GetGlobalStart(INT4 i) {
  return(table[i].GetGlobalStart());
  }


  DOUBLE 
  INTERVALLIST::GetStartValue(INT i) {
  return(table[i].GetStartValue());
  }


  DOUBLE 
  INTERVALLIST::GetEndValue(INT i) {
  return(table[i].GetEndValue());
  }


  void 
  INTERVALLIST::SetFileName(STRING s) {
  FileName = s;
  }


  void 
  INTERVALLIST::SetFileName(PCHR s) {
  FileName = s;
  }


  INT4 
  INTERVALLIST::GetCount() {
  return(Count);
  }


  void 
  INTERVALLIST::Expand() {
  Resize(Count + (50*Ncoords));
  }


  void 
  INTERVALLIST::Cleanup() { 
  Resize(Count);
  }
*/


#ifdef NEVER
main()
{
  NUMERICLIST list;
  STRING n;
  INT4 Start,End;
  DOUBLE val;
  n="test.62";
  printf("DOUBLE is %d bytes\n",sizeof(DOUBLE));
  list.TempLoad();
  End=list.DiskFind(n, (double)88.0,5); // 5 GT
  printf("\n===\n");
  Start=list.DiskFind(n,(double)88.0,1); // 1 LT
  
  if(End-Start<=1)
    printf("No Match!\n");
  list.SetFileName(n.NewCString());
  list.LoadTable(Start,End);
    
}
#endif
