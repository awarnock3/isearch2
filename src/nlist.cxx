/* $Id: nlist.cxx,v 1.12 2000/02/04 23:39:56 cnidr Exp $ */
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
File:		nlist.cxx
Version:	1.00
$Revision: 1.12 $
Description:	Class NUMERICLIST
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

#include <stdlib.h>
#include <iostream>

#include "nlist.hxx"

static INT 
SortCmp(const void* x, const void* y);

static INT 
SortCmpGP(const void* x, const void* y);


NUMERICLIST::NUMERICLIST()
{
  Ncoords    = 2;
  table      = new NUMERICFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
}


NUMERICLIST::NUMERICLIST(INT n)
{
  Ncoords    = n;
  table      = new NUMERICFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
}


void 
NUMERICLIST::ResetHitPosition()
{
  if (Relation != 1)
    Pointer = StartIndex;
  else
    Pointer = 0;
}


INT4 
NUMERICLIST::GetNextHitPosition()
{
  INT4 Value;

  if(StartIndex == -1)
    return(-1);
  if(Relation != 1) {
    if(Pointer>EndIndex)
      return(-1);
    Value=table[Pointer].GetGlobalStart();
    ++Pointer;
    return(Value);
  } else {
    if(Pointer >= StartIndex)
      while(Pointer <= EndIndex)
	++Pointer;
    if(Pointer >= Count)
      return(-1);
    Value=table[Pointer].GetGlobalStart();
    ++Pointer;
    return(Value);
  }
}


static INT 
SortCmp(const void* x, const void* y) 
{
  DOUBLE a;
  a=((*((PNUMERICFLD)x)).GetNumericValue()) -
    ((*((PNUMERICFLD)y)).GetNumericValue()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT 
SortCmpGP(const void* x, const void* y) 
{
  DOUBLE a;
  a=((*((PNUMERICFLD)x)).GetGlobalStart()) -
    ((*((PNUMERICFLD)y)).GetGlobalStart()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


void 
NUMERICLIST::Resize(INT4 Entries)
{
  PNUMERICFLD temp=new NUMERICFLD[Entries];
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


void 
NUMERICLIST::Sort()
{
  qsort((void *)table, Count, sizeof(NUMERICFLD),SortCmp);
}


void 
NUMERICLIST::SortByGP()
{
  qsort((void *)table, Count, sizeof(NUMERICFLD),SortCmpGP);
}


DOUBLE
NUMERICLIST::GetMaxValue()
{
  INT4 x;
  DOUBLE y,yy;
  y = table[0].GetNumericValue();
  for(x=1; x<Count; x++) {
    yy= table[x].GetNumericValue();
    if (yy > y)
      y = yy;
  }
  return y;
}


DOUBLE
NUMERICLIST::GetMinValue()
{
  INT4 x;
  DOUBLE y,yy;
  y = table[0].GetNumericValue();
  for(x=1; x<Count; x++) {
    yy= table[x].GetNumericValue();
    if (yy < y)
      y = yy;
  }
  return y;
}


SearchState
NUMERICLIST::Matcher(DOUBLE Key, DOUBLE A, DOUBLE B, DOUBLE C, 
	INT4 Relation, INT4 Type)
{
	
  switch (Relation) {
  case ZRelGE:			// greater than or equals
    if ((B>=Key) && (Type==-1 || A<Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>=Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelGT:			// greater than
    if ((B>Key) && (Type==-1 || A<=Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelLE:			// less than or equals
    if ((B<=Key) && (Type==0 || C>Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<=Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

  case ZRelLT:			// less than
    if ((B<Key) && (Type==0 || C>=Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);
  }

  cerr << "Hideous Matching Error" << endl;
  return(NO_MATCH);
}


SearchState
NUMERICLIST::Matcher(INT4 Key, INT4 A, INT4 B, INT4 C, 
		     INT4 Relation, INT4 Type)
{
	
  switch (Relation) {
  case ZRelGE:			// greater than or equals
    if ((B>=Key) && (Type==-1 || A<Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>=Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelGT:			// greater than
    if ((B>Key) && (Type==-1 || A<=Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelLE:			// less than or equals
    if ((B<=Key) && (Type==0 || C>Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<=Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

  case ZRelLT:			// less than
    if ((B<Key) && (Type==0 || C>=Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);
  }

  cerr << "Hideous Matching Error" << endl;
  return(NO_MATCH);
}


// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState
NUMERICLIST::Find(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(Fn, Key, Relation, Index);
  return Status;
}


SearchState
NUMERICLIST::Find(DOUBLE Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, Index);
  return Status;
}


SearchState
NUMERICLIST::MemFind(DOUBLE Key, INT4 Relation, INT4 *Index)
{
  return NO_MATCH;
}


SearchState
NUMERICLIST::DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index)
{
  //  INT4 B;
  PFILE  Fp = fopen(Fn, "rb");
  INT    ElementSize;

  if (!Fp) {
    perror(Fn);
    *Index = -1;
    return NO_MATCH;

  } else {

    INT         Total, Low, High, X, OX;
    SearchState State;
    INT         Type=0;
    DOUBLE      Hold;         // This is just a dummy - we don't use it
    INT4        Offset;       // Offset needed to read the element

    ElementSize = sizeof(INT4) + sizeof(DOUBLE);
    fread((char*)&Total,1,sizeof(INT4),Fp);

    Low = 0;
    High = Total - 1;
    X = High / 2;

    INT4   GpS, Dummy;
    DOUBLE NumericValue, LowerBound, UpperBound;
    do {
      OX = X;

      if ((X > 0) && (X < High)) {
	Offset = sizeof(INT4) + (X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=INSIDE;

      } else if (X <= 0) {
	Offset = sizeof(INT4) + X * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=AT_START;

      } else if (X >= High) {
	Offset = sizeof(INT4) + (X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=AT_END;
      }
	
      if (Type != AT_START) {
	fread((char *)&GpS, 1, sizeof(INT4), Fp);
	fread((char *)&LowerBound, 1, sizeof(DOUBLE), Fp);
      }
	
      fread((char *)&GpS, 1, sizeof(INT4), Fp);
      fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp);
	
      // If we're at the start, we need to read the first value into
      // NumericValue, but we don't want to leave LowerBound 
      // uninitialized.  This will also handle the case when we only
      // have two values in the index.
      if (Type == AT_START)
	LowerBound = NumericValue;
	
      if(Type != AT_END) {
	fread((char *)&Dummy, 1, sizeof(INT4), Fp);
	fread((char *)&UpperBound, 1, sizeof(DOUBLE), Fp);
      }
	
      // Similarly, if we're at the end and can't read in a new value
      // for UpperBound, we don't want it uninitialized, either.
      if (Type == AT_END)
	UpperBound = NumericValue;
	
      State = Matcher(Key, LowerBound, NumericValue, UpperBound,
		      Relation, Type);
	
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
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


// Search for the GP INT4 values
// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState
NUMERICLIST::Find(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(Fn, Key, Relation, Index);
  return Status;
}


SearchState
NUMERICLIST::Find(INT4 Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, Index);
  return Status;
}


SearchState
NUMERICLIST::MemFind(INT4 Key, INT4 Relation, INT4 *Index)
{
  return NO_MATCH;
}


SearchState
NUMERICLIST::DiskFind(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index)
{
  PFILE  Fp = fopen(Fn, "rb");
  INT    ElementSize;

  if (!Fp) {
    perror(Fn);
    *Index = -1;
    return NO_MATCH;

  } else {

    INT         Total, Low, High, X, OX;
    SearchState State;
    INT         Type=0;
    DOUBLE      Hold;         // This is just a dummy - we don't use it
    INT4        Offset;       // Offset needed to read the element

    ElementSize = sizeof(INT4) + sizeof(DOUBLE);
    fread((char*)&Total,1,sizeof(INT4),Fp);

    Low = 0;
    High = Total - 1;
    X = High / 2;

    //    INT4 GpS, Dummy;
    INT4   GpValue, GpLower, GpUpper;
    DOUBLE Dummy, NumericValue;

    do {
      OX = X;

      if ((X > 0) && (X < High)) {
	Offset = sizeof(INT4) + (Total+X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=INSIDE;

      } else if (X <= 0) {
	Offset = sizeof(INT4) + (Total+X) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=AT_START;

      } else if (X >= High) {
	Offset = sizeof(INT4) + (Total+X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=AT_END;
      }
	
      if (Type != AT_START) {
	fread((char *)&GpLower, 1, sizeof(INT4), Fp);
	fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp);
      }
	
      fread((char *)&GpValue, 1, sizeof(INT4), Fp);
      fread((char *)&NumericValue, 1, sizeof(DOUBLE), Fp);
	
      // If we're at the start, we need to read the first value into
      // NumericValue, but we don't want to leave LowerBound 
      // uninitialized.  This will also handle the case when we only
      // have two values in the index.
      if (Type == AT_START)
	GpLower = GpValue;
	
      if(Type != AT_END) {
	fread((char *)&GpUpper, 1, sizeof(INT4), Fp);
	fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp);
      }
	
      // Similarly, if we're at the end and can't read in a new value
      // for UpperBound, we don't want it uninitialized, either.
      if (Type == AT_END)
	GpUpper = GpValue;
	
      State = Matcher(Key, GpLower, GpValue, GpUpper, Relation, Type);
	
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
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


void 
NUMERICLIST::Dump()
{
  INT4 x;
  
  for(x=0; x<Count; x++) {
    printf("start: %i", table[x].GetGlobalStart());
    printf(" value: %.2f\n", table[x].GetNumericValue());
  }
}


void 
NUMERICLIST::Dump(INT4 start, INT4 end)
{
  INT4 x;

  if (start < 0)
    start = 0;
  if (end > Count)
    end = Count;

  for(x=start; x<end; x++) {
    printf("start: %i", table[x].GetGlobalStart());
    printf(" value: %.2f\n", table[x].GetNumericValue());
  }
}


INT4
NUMERICLIST::LoadTable(INT4 Start, INT4 End)
{
  FILE   *fp;
  GPTYPE  high;
  INT4    x, i;
  DOUBLE  value;
  INT4    nRecs=0;
  
  if ( !(FileName.GetLength()) ) {
    cerr << "FileName not set" << endl;
    RETURN_ZERO;
  }
  
  if (End==-1)
    End=999999999;
  if (Start==-1)
    Start=0;
  
  fp=fopen(FileName,"rb");
  if (fp) {
    nRecs = 0;
    fseek(fp, (long)(Start*(sizeof(GPTYPE)+sizeof(DOUBLE))), SEEK_SET);
    for (i=Start;i<=End;i++){
      x=fread((char*)&high,1,sizeof(GPTYPE),fp);
      if (x==0)
	break;
      x=fread((char*)&value,1,sizeof(DOUBLE),fp);
      table[Count].SetGlobalStart(high);
      table[Count].SetNumericValue(value);
      Count++;
      nRecs++;
      if(Count==MaxEntries)
	Expand();
    }
    fclose(fp);
    Cleanup();
  }
  return nRecs;
}


INT4
NUMERICLIST::LoadTable(INT4 Start, INT4 End, NumBlock Offset)
{
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // Return the actual number of items read in this load

  FILE  *fp;
  INT4   high;
  INT4   x, i;
  DOUBLE fValue;
  INT4   nCount, nRecs=0;
  LONG   MoveTo;
  
  if ( !(FileName.GetLength()) ) {
    cerr << "FileName not set" <<endl;
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
      if (Offset == VAL_BLOCK) {
	MoveTo = sizeof(INT4)
	  + (Start) * ( sizeof(INT4) + sizeof(DOUBLE) );
      } else if (Offset == GP_BLOCK) {
	MoveTo = sizeof(INT4)
	  + (nCount+Start) * ( sizeof(INT4) + sizeof(DOUBLE) );
      } else {
	MoveTo = 0;
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
	x = fread((char*)&fValue, 1, sizeof(DOUBLE), fp); // explicit cast
	table[Count].SetNumericValue(fValue);

#ifdef DEBUG
	cerr << "table[" << Count << "]=(" << high << ", " << fValue
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
NUMERICLIST::WriteTable()
{
  PFILE  fp;
  INT4   x;
  GPTYPE Val;
  DOUBLE fVal;
  
  if ( !(FileName.GetLength()) ) {
    cerr << "FileName not set" << endl;
    return;
  }
  fp=fopen(FileName,"wb");
  if (fp) {
    for(x=0; x<Count; x++) {
      Val = table[x].GetGlobalStart();
      fVal = table[x].GetNumericValue();
      fwrite((char*)&Val,1,sizeof(GPTYPE),fp);
      fwrite((char*)&fVal,1,sizeof(DOUBLE),fp);
    }
  }
  fclose(fp);
}


void 
NUMERICLIST::WriteTable(INT Offset)
{
  FILE  *fp;
  INT4   x;
  INT4   Val;
  DOUBLE fValue;
  LONG   MoveTo, nBytes;
  
  if ( !(FileName.GetLength()) ) {
    cerr << "FileName not set" << endl;
    return;
  }
  fp = fopen(FileName,"a+b");

  if (fp) {
    // First, write out the count
    if (Offset == 0) {
      fwrite((char*)&Count, 1, sizeof(INT4), fp);
    }

    // Now, go to the specified offset and write the table
    nBytes = sizeof(INT4) + sizeof(DOUBLE);
    MoveTo = (LONG)((Offset*nBytes) + sizeof(INT4));

    //    cout << "Offsetting " << MoveTo << " bytes into the file." << endl;

    if (fseek(fp, MoveTo, SEEK_SET) == 0) {
      for (x=0; x<Count; x++) {
	Val    = table[x].GetGlobalStart();
	fValue = table[x].GetNumericValue();
	fwrite((char*)&Val,    1, sizeof(INT4),   fp);
	fwrite((char*)&fValue, 1, sizeof(DOUBLE), fp);
	//	cout << "Wrote " << nBytes << " bytes." << endl;
	//	cout << "gp=" << Val;
	//	cout << ", [" << fStart;
	//	cout << ", " << fEnd << "]" << endl;

      }
    }
  }
  fclose(fp);
}


NUMERICLIST::~NUMERICLIST()
{
  if (table)
    delete [] table;
}


#ifdef NEVER
void 
NUMERICLIST::TempLoad()
{
  INT4 x,y=0;
  DOUBLE z;
  FILE *fp=fopen("test.62","w");
  
  for(x=0; x<100; x+=4){
    z=x;
    fwrite((char*)&x,1,sizeof(INT4),fp);
    x+=4;
    fwrite((char*)&z,1,sizeof(DOUBLE),fp);
    printf("Wrote %f\n",z);
    
  }
  fclose(fp);
  fp=fopen("test.62","r");
  while((y=fread((char*)&x,1,sizeof(INT4),fp))!=0) {
    printf("x: %d\n",x);
    fread((char*)&z,1,sizeof(DOUBLE),fp);
    printf("z(double): %f\n",z);
  }
  fclose(fp);
}


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

/*
#define GE 4
#define GT 5
#define LE 2
#define LT 1

#define ERROR -1
#define TOO_LOW 0
#define TOO_HIGH 1
#define MATCH 2
*/

