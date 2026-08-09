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

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		nlist.cxx
Version:	1.00
$Revision: 1.12 $
Description:	Class NUMERICLIST
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

/**
 * @file nlist.cxx
 * @brief Implements NUMERICLIST, a heap-allocated growable table of
 * NUMERICFLD entries used for numeric-attribute range search.
 *
 * Each entry pairs a numeric value with a global pointer (byte offset
 * into the document stream). Entries can be sorted by value (Sort())
 * or by global pointer (SortByGP()), and searched via Find(), which
 * always delegates to a binary search over the on-disk index file
 * (DiskFind()) — the in-memory fast path implied by Find()'s own
 * comments (MemFind()) was never implemented and unconditionally
 * returns NO_MATCH. DiskFind() reads either a value-sorted block
 * (the DOUBLE-keyed Find() overloads) or a separate global-pointer-
 * sorted block (the INT4-keyed overloads) from the same file, and
 * classifies each probed entry via Matcher() into a SearchState
 * (MATCH / TOO_LOW / TOO_HIGH / NO_MATCH). LoadTable()/WriteTable()
 * move the whole table to/from disk in one pass; ResetHitPosition()/
 * GetNextHitPosition() iterate the [StartIndex, EndIndex] hit window
 * left behind by a prior Find().
 *
 * NUMERICLIST is non-copyable (BUGFIX #1,
 * docs/BUG_CATALOG.md#srcnlisthxx) since it owns `table` and no call
 * site ever copied one. `src/intlist.hxx`'s INTERVALLIST derives from
 * this class and repeats the same shape one level up.
 */
#include <stdlib.h>
#include <iostream>

#include "nlist.hxx"

static INT
SortCmp(const void* x, const void* y);

static INT
SortCmpGP(const void* x, const void* y);


/**
 * @brief Constructs an empty list with the default 2-coordinate entry
 * shape.
 *
 * Allocates an initial 50*Ncoords-entry table (Ncoords = 2) and leaves
 * FileName empty; Attribute/Relation are zero-initialized (BUGFIX #2)
 * and StartIndex/EndIndex start at -1 (no hit window yet).
 */
NUMERICLIST::NUMERICLIST()
{
  Ncoords    = 2;
  table      = new NUMERICFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  // BUGFIX #2 (docs/BUG_CATALOG.md#srcnlisthxx): Attribute/Relation
  // were left indeterminate by both constructors -- every other member
  // was explicitly set. Matches the same "indeterminate primitive
  // member" category already fixed in RESULT's and NUMERICFLD's
  // constructors.
  Attribute  = 0;
  Relation   = 0;
}


/**
 * @brief Constructs an empty list whose entries hold @p n coordinate
 * values.
 * @param n Number of coordinate values per table entry (Ncoords).
 *
 * Same initialization as the default constructor otherwise, including
 * the BUGFIX #2 zero-initialization of Attribute/Relation.
 */
NUMERICLIST::NUMERICLIST(INT n)
{
  Ncoords    = n;
  table      = new NUMERICFLD[50*Ncoords];
  Count      = 0;
  MaxEntries = 50*Ncoords;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  // BUGFIX #2, second constructor -- see the default constructor above.
  Attribute  = 0;
  Relation   = 0;
}


/**
 * @brief Resets the hit-iteration cursor to the start of the current
 * hit window.
 *
 * Points Pointer at StartIndex, except when Relation == 1 (LT), where
 * it starts the cursor at 0 instead — see GetNextHitPosition() for how
 * the two cases are walked.
 */
void
NUMERICLIST::ResetHitPosition()
{
  if (Relation != 1)
    Pointer = StartIndex;
  else
    Pointer = 0;
}


/**
 * @brief Returns the next hit's global pointer from the current window
 * and advances the cursor, or -1 once exhausted.
 * @return table[Pointer].GetGlobalStart() for the hit at the old
 * cursor position, or -1 if there is no current window
 * (StartIndex == -1) or the cursor has run past it.
 *
 * For Relation == 1 (LT), the cursor first skips forward past
 * [StartIndex, EndIndex] before returning anything, so hits are drawn
 * from entries after EndIndex up to Count - 1 rather than from the
 * [StartIndex, EndIndex] window itself.
 */
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


/**
 * @brief qsort() comparator ordering NUMERICFLD entries by ascending
 * numeric value.
 * @param x Pointer to the first NUMERICFLD (as `const void*`, per qsort()).
 * @param y Pointer to the second NUMERICFLD (as `const void*`, per qsort()).
 * @return Negative if x's value is less than y's, positive if greater,
 * 0 if equal.
 */
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


/**
 * @brief qsort() comparator ordering NUMERICFLD entries by ascending
 * global pointer.
 * @param x Pointer to the first NUMERICFLD (as `const void*`, per qsort()).
 * @param y Pointer to the second NUMERICFLD (as `const void*`, per qsort()).
 * @return Negative if x's global pointer is less than y's, positive if
 * greater, 0 if equal.
 */
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


/**
 * @brief Reallocates table to hold exactly @p Entries slots, copying
 * over as many existing entries as still fit.
 * @param Entries The new table capacity. If less than the current
 * Count, Count itself is lowered to match (excess entries are
 * dropped).
 */
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


/**
 * @brief Sorts the first Count table entries in place by ascending
 * numeric value, via SortCmp().
 */
void
NUMERICLIST::Sort()
{
  qsort((void *)table, Count, sizeof(NUMERICFLD),SortCmp);
}


/**
 * @brief Sorts the first Count table entries in place by ascending
 * global pointer, via SortCmpGP().
 */
void
NUMERICLIST::SortByGP()
{
  qsort((void *)table, Count, sizeof(NUMERICFLD),SortCmpGP);
}


/**
 * @brief Returns the largest numeric value among the first Count table
 * entries.
 * @return The maximum GetNumericValue() found; the value at table[0]
 * if Count <= 1.
 */
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


/**
 * @brief Returns the smallest numeric value among the first Count
 * table entries.
 * @return The minimum GetNumericValue() found; the value at table[0]
 * if Count <= 1.
 */
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


/**
 * @brief Classifies @p Key against the (A, B, C) neighborhood read
 * from disk during DiskFind()'s binary search.
 * @param Key The value being searched for.
 * @param A The value just below the candidate entry (meaningless when
 * @p Type is AT_START).
 * @param B The candidate entry's own value.
 * @param C The value just above the candidate entry (meaningless when
 * @p Type is AT_END).
 * @param Relation Which comparison is being satisfied (see ZRelGE /
 * ZRelGT / ZRelLE / ZRelLT in defs.hxx).
 * @param Type Where the candidate sits in the on-disk block: AT_START,
 * INSIDE, or AT_END.
 * @return MATCH if @p Key falls on the boundary DiskFind() is
 * currently probing for @p Relation, otherwise TOO_LOW or TOO_HIGH to
 * steer the next probe; NO_MATCH (after printing an error) if
 * @p Relation isn't one of the four handled cases.
 */
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


/**
 * @brief Integer-keyed overload of Matcher(DOUBLE, ...); same
 * classification logic over INT4 values.
 * @param Key The value being searched for.
 * @param A The value just below the candidate entry (meaningless when
 * @p Type is AT_START).
 * @param B The candidate entry's own value.
 * @param C The value just above the candidate entry (meaningless when
 * @p Type is AT_END).
 * @param Relation Which comparison is being satisfied (see ZRelGE /
 * ZRelGT / ZRelLE / ZRelLT in defs.hxx).
 * @param Type Where the candidate sits in the on-disk block: AT_START,
 * INSIDE, or AT_END.
 * @return MATCH, TOO_LOW, or TOO_HIGH as for the DOUBLE overload;
 * NO_MATCH if @p Relation isn't one of the four handled cases.
 */
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
/**
 * @brief Sets FileName to @p Fn and searches it on disk for @p Key
 * under @p Relation.
 * @param Fn On-disk numeric-index file name to search.
 * @param Key The value being searched for.
 * @param Relation Which comparison to satisfy (see ZRelGE / ZRelGT /
 * ZRelLE / ZRelLT in defs.hxx).
 * @param Index Out param: set to the matching table position, or -1 if
 * DiskFind() found no match.
 * @return The SearchState DiskFind() returned (MATCH, TOO_LOW,
 * TOO_HIGH, or NO_MATCH). The comment above describes an in-memory
 * fast path via MemFind() that was never implemented (see MemFind()'s
 * own doc comment) — this always goes straight to DiskFind().
 */
SearchState
NUMERICLIST::Find(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(Fn, Key, Relation, Index);
  return Status;
}


/**
 * @brief Searches the already-set FileName on disk for @p Key under
 * @p Relation.
 * @param Key The value being searched for.
 * @param Relation Which comparison to satisfy.
 * @param Index Out param: set to the matching table position, or -1 if
 * no match.
 * @return The SearchState DiskFind() returned.
 */
SearchState
NUMERICLIST::Find(DOUBLE Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, Index);
  return Status;
}


/**
 * @brief Always returns NO_MATCH; the in-memory search path was never
 * implemented.
 * @param Key Unused.
 * @param Relation Unused.
 * @param Index Unused.
 * @return NO_MATCH, unconditionally. Never called from anywhere in
 * this class — Find() always goes straight to DiskFind().
 */
SearchState
NUMERICLIST::MemFind(DOUBLE Key, INT4 Relation, INT4 *Index)
{
  return NO_MATCH;
}


/**
 * @brief Binary-searches @p Fn's on-disk float-keyed table for the
 * boundary matching @p Key under @p Relation.
 * @param Fn On-disk numeric-index file to search; opened read-only for
 * the duration of the call.
 * @param Key The value being searched for.
 * @param Relation Which comparison to satisfy (passed through to
 * Matcher()).
 * @param Index Out param: set to the matching table position on
 * MATCH, or -1 on any other outcome (file-open/read failure, TOO_HIGH
 * at the last entry, TOO_LOW at the first entry, or the search space
 * exhausted without a match).
 * @return MATCH, TOO_LOW, TOO_HIGH, or NO_MATCH, per Matcher()'s
 * classification of each probed entry during the binary search.
 */
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
    if (fread((char*)&Total,1,sizeof(INT4),Fp) != sizeof(INT4)) {
      fclose(Fp);
      *Index = -1;
      return NO_MATCH;
    }

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
	if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&LowerBound, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}
      }
	
      if (fread((char *)&GpS, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
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
	
      if(Type != AT_END) {
	if (fread((char *)&Dummy, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
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
/**
 * @brief Sets FileName to @p Fn and searches it on disk for @p Key (a
 * global pointer) under @p Relation.
 * @param Fn On-disk numeric-index file name to search.
 * @param Key The global-pointer value being searched for.
 * @param Relation Which comparison to satisfy.
 * @param Index Out param: set to the matching table position, or -1 if
 * DiskFind() found no match.
 * @return The SearchState DiskFind() returned.
 */
SearchState
NUMERICLIST::Find(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  SetFileName(Fn);

  Status =
    DiskFind(Fn, Key, Relation, Index);
  return Status;
}


/**
 * @brief Searches the already-set FileName on disk for @p Key (a
 * global pointer) under @p Relation.
 * @param Key The global-pointer value being searched for.
 * @param Relation Which comparison to satisfy.
 * @param Index Out param: set to the matching table position, or -1 if
 * no match.
 * @return The SearchState DiskFind() returned.
 */
SearchState
NUMERICLIST::Find(INT4 Key, INT4 Relation, INT4 *Index)
{
  SearchState Status;

  Status =
    DiskFind(FileName, Key, Relation, Index);
  return Status;
}


/**
 * @brief Always returns NO_MATCH; the in-memory search path was never
 * implemented.
 * @param Key Unused.
 * @param Relation Unused.
 * @param Index Unused.
 * @return NO_MATCH, unconditionally.
 */
SearchState
NUMERICLIST::MemFind(INT4 Key, INT4 Relation, INT4 *Index)
{
  return NO_MATCH;
}


/**
 * @brief Binary-searches @p Fn's on-disk global-pointer-keyed table
 * for the boundary matching @p Key under @p Relation.
 * @param Fn On-disk numeric-index file to search; opened read-only for
 * the duration of the call.
 * @param Key The global-pointer value being searched for.
 * @param Relation Which comparison to satisfy (passed through to
 * Matcher()).
 * @param Index Out param: set to the matching table position on
 * MATCH, or -1 on any other outcome.
 * @return MATCH, TOO_LOW, TOO_HIGH, or NO_MATCH, per Matcher()'s
 * classification of each probed entry.
 *
 * Reads from the file's second (global-pointer-sorted) block —
 * offsets here are computed relative to `Total + X`, not `X` as in the
 * float-keyed overload above, to land past the first block.
 */
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
    if (fread((char*)&Total,1,sizeof(INT4),Fp) != sizeof(INT4)) {
      fclose(Fp);
      *Index = -1;
      return NO_MATCH;
    }

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
	if (fread((char *)&GpLower, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}
      }
	
      if (fread((char *)&GpValue, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
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
	GpLower = GpValue;
	
      if(Type != AT_END) {
	if (fread((char *)&GpUpper, 1, sizeof(INT4), Fp) != sizeof(INT4) ||
	    fread((char *)&Dummy, 1, sizeof(DOUBLE), Fp) != sizeof(DOUBLE)) {
	  fclose(Fp);
	  *Index = -1;
	  return NO_MATCH;
	}
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


/**
 * @brief Prints every table entry's global pointer and numeric value
 * to stdout.
 */
void
NUMERICLIST::Dump()
{
  INT4 x;

  for(x=0; x<Count; x++) {
    printf("start: %i", table[x].GetGlobalStart());
    printf(" value: %.2f\n", table[x].GetNumericValue());
  }
}


/**
 * @brief Prints table entries [start, end) to stdout, clamped to the
 * table's actual bounds.
 * @param start First index to print; clamped up to 0 if negative.
 * @param end One past the last index to print; clamped down to Count
 * if larger.
 */
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


/**
 * @brief Loads entries [Start, End] from FileName's single-block
 * on-disk table into table, growing the table as needed.
 * @param Start First index to load; -1 means "from the beginning" (0).
 * @param End Last index to load (inclusive); -1 means "to the end"
 * (capped at a large sentinel, so the read loop runs until fread()
 * hits EOF).
 * @return The number of entries actually read.
 */
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


/**
 * @brief Loads entries [Start, End] from FileName's VAL_BLOCK- or
 * GP_BLOCK-sorted on-disk table into table, growing the table as
 * needed.
 * @param Start First index to load; clamped up to 0 if negative.
 * @param End Last index to load (inclusive); clamped to the file's own
 * entry count (read from the file) if out of range.
 * @param Offset Which of the file's two sorted blocks to read from
 * (VAL_BLOCK or GP_BLOCK).
 * @return The number of entries actually read.
 */
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


/**
 * @brief Overwrites FileName with the table's (global pointer,
 * numeric value) pairs, in current table order.
 */
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


/**
 * @brief Appends the table's (global pointer, numeric value) pairs to
 * FileName, seeking to the given block @p Offset first.
 * @param Offset Block index within the file to seek to before writing;
 * if 0, the table's Count is also written first (as the block's
 * header).
 */
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


/**
 * @brief Destroys the list and frees its table.
 */
NUMERICLIST::~NUMERICLIST()
{
  if (table)
    delete [] table;
}


#ifdef NEVER
/**
 * @brief Disabled scratch/test routine — compiled only under
 * `#ifdef NEVER`, so it is not part of the live API despite its
 * header declaration. Writes and reads back a small fixed test file
 * ("test.62") to exercise the on-disk format by hand.
 */
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


// Disabled ad hoc test driver (compiled only under #ifdef NEVER) for
// exercising TempLoad()/DiskFind()/LoadTable() by hand; not part of
// the library's API.
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
