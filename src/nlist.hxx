// $Id: nlist.hxx,v 1.5 2000/02/04 23:39:56 cnidr Exp $
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
File:		nlist.hxx
Version:	1.00
$Revision: 1.5 $
Description:	Class NLIST - Utilities for lists of numeric values
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

#ifndef NUMERICLIST_HXX
#define NUMERICLIST_HXX

#include <stdlib.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"

// SearchState is used to indicate the results of the matcher 
enum SearchState { NO_MATCH=-1, TOO_LOW, TOO_HIGH, MATCH };

// IntType tells the matcher where we are in the block of values
enum IntType { AT_START, INSIDE, AT_END };

// NumBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
enum NumBlock { VAL_BLOCK, GP_BLOCK };


/// Owns a heap-allocated, growable table of NUMERICFLD entries used for
/// numeric-attribute range search. Non-copyable: no live call site ever
/// copies one (see BUGFIX #1), so the copy constructor and operator=
/// are both deleted rather than given deep-copy semantics.
class NUMERICLIST {
private:
  PNUMERICFLD  table;      // the table of attribute/numeric data
  INT4         Count;      // count of items in table
  INT          Attribute;  // which USE attribute this table maps
  INT4         Pointer;    // current position
  INT4         MaxEntries; // current maximum size of table - see Resize()
  INT4         StartIndex;
  INT4         EndIndex;
  INT          Relation;   // what relation generated StartIndex, EndIndex
  STRING       FileName;   // the file which attributes/numeric data are in
  INT          Ncoords;    // Number of values in an entry

  /// Binary-searches Fn's on-disk float-keyed table for the boundary matching Key under Relation.
  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index);
  /// Always returns NO_MATCH; the in-memory search path was never implemented.
  SearchState  MemFind(DOUBLE Key, INT4 Relation, INT4 *Index);

  /// Binary-searches Fn's on-disk global-pointer-keyed table for the boundary matching Key under Relation.
  SearchState  DiskFind(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index);
  /// Always returns NO_MATCH; the in-memory search path was never implemented.
  SearchState  MemFind(INT4 Key, INT4 Relation, INT4 *Index);

  /// Classifies Key against the (A, B, C) neighborhood read from disk during DiskFind()'s binary search.
  SearchState  Matcher(DOUBLE Key, DOUBLE A, DOUBLE B, DOUBLE C,
		       INT4 Relation, INT4 Type);
  /// Integer-keyed overload of Matcher(DOUBLE, ...); same classification logic.
  SearchState  Matcher(INT4 Key, INT4 A, INT4 B, INT4 C,
		       INT4 Relation, INT4 Type);

public:
  /// Constructs an empty list with the default 2-coordinate entry shape.
  NUMERICLIST();
  /// Constructs an empty list whose entries hold n coordinate values.
  NUMERICLIST(INT n);
  // BUGFIX #1 (docs/BUG_CATALOG.md#srcnlisthxx): NUMERICLIST owned
  // `table` (heap-allocated, freed in ~NUMERICLIST()) with no
  // user-declared copy constructor or operator= -- the compiler-
  // generated ones shallow-copied `table`, confirmed to cause a
  // heap-use-after-free (ASan) on copy. No live call site ever copies
  // a NUMERICLIST, so it's made explicitly non-copyable rather than
  // given deep-copy semantics.
  /// Deleted — see BUGFIX #1 above; no call site ever copies a NUMERICLIST.
  NUMERICLIST(const NUMERICLIST&) = delete;
  /// Deleted — copy-assignment counterpart of the deleted copy constructor above.
  NUMERICLIST& operator=(const NUMERICLIST&) = delete;
  /// Sets the relation code that produced the current hit window.
  void         SetRelation(INT r)      { Relation = r; };
  /// Returns the relation code that produced the current hit window.
  INT          GetRelation()           { return(Relation); };
  /// Returns the numeric value stored at table index i.
  DOUBLE       GetNumericValue(INT i)  { return(table[i].GetNumericValue()); };
  /// Returns the largest numeric value currently in the table.
  DOUBLE       GetMaxValue();
  /// Returns the smallest numeric value currently in the table.
  DOUBLE       GetMinValue();
  /// Returns the USE attribute this table maps.
  INT          GetAttribute()          { return(Attribute); };
  /// Sets the USE attribute this table maps.
  void         SetAttribute(INT x)     { Attribute = x; };
  /// Returns the number of entries currently in the table.
  INT4         GetCount()              { return(Count); };
  /// Grows the table by another 50*Ncoords entries.
  void         Expand()                { Resize(Count + (50*Ncoords)); };
  /// Shrinks the table's allocation down to exactly Count entries.
  void         Cleanup()               { Resize(Count); };
  /// Returns the global pointer (byte offset) stored at table index i.
  INT4         GetGlobalStart(INT4 i)  { return(table[i].GetGlobalStart()); };
  /// Sets the on-disk file name this list loads/searches against (STRING overload).
  void         SetFileName(STRING s)   { FileName = s; };
  /// Sets the on-disk file name this list loads/searches against (C-string overload).
  void         SetFileName(PCHR s)     { FileName = s; };
  /// Sets the number of coordinate values per entry.
  void         SetCoords(INT n)        { Ncoords = n; };
  /// Returns the number of coordinate values per entry.
  INT          GetCoords()             { return Ncoords; };

  /// Sets FileName to Fn and searches it on disk for a DOUBLE Key under Relation.
  SearchState  Find(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index);
  /// Sets FileName to Fn and searches it on disk for an INT4 Key (a global pointer) under Relation.
  SearchState  Find(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index);
  /// Searches the already-set FileName on disk for a DOUBLE Key under Relation.
  SearchState  Find(DOUBLE Key, INT4 Relation, INT4 *Index);
  /// Searches the already-set FileName on disk for an INT4 Key (a global pointer) under Relation.
  SearchState  Find(INT4 Key, INT4 Relation, INT4 *Index);
  /// Sorts table entries in place by ascending numeric value.
  void         Sort();
  /// Sorts table entries in place by ascending global pointer.
  void         SortByGP();
  /// Prints every table entry's global pointer and numeric value to stdout.
  void         Dump();
  /// Prints table entries [start, end) (clamped to bounds) to stdout.
  void         Dump(INT4 start, INT4 end);
  /// Reallocates table to hold exactly Entries slots, preserving existing entries up to the smaller of Entries and Count.
  void         Resize(INT4 Entries);
  /// Disabled scratch/test routine (compiled only under #ifdef NEVER); not part of the live API.
  void         TempLoad();
  /// Loads entries [Start, End] from FileName's single-block on-disk table into table, growing as needed.
  INT4         LoadTable(INT4 Start, INT4 End);
  /// Loads entries [Start, End] from FileName's VAL_BLOCK- or GP_BLOCK-sorted on-disk table into table, growing as needed.
  INT4         LoadTable(INT4 Start, INT4 End, NumBlock Offset);
  /// Overwrites FileName with the table's (global pointer, numeric value) pairs.
  void         WriteTable();
  /// Appends the table's (global pointer, numeric value) pairs to FileName at the given block Offset.
  void         WriteTable(INT Offset);
  /// Resets the hit-iteration cursor to the start of the current hit window.
  void         ResetHitPosition();
  /// Returns the next hit's global pointer from the current window, advancing the cursor, or -1 when exhausted.
  INT4         GetNextHitPosition();
  /// Destroys the list and frees its table.
  ~NUMERICLIST();
};

typedef NUMERICLIST* PNUMERICLIST;
#endif
