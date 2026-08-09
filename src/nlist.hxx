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

  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index);
  SearchState  MemFind(DOUBLE Key, INT4 Relation, INT4 *Index);

  SearchState  DiskFind(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index);
  SearchState  MemFind(INT4 Key, INT4 Relation, INT4 *Index);

  SearchState  Matcher(DOUBLE Key, DOUBLE A, DOUBLE B, DOUBLE C, 
		       INT4 Relation, INT4 Type);
  SearchState  Matcher(INT4 Key, INT4 A, INT4 B, INT4 C, 
		       INT4 Relation, INT4 Type);

public:
  NUMERICLIST();
  NUMERICLIST(INT n);
  // BUGFIX #1 (docs/BUG_CATALOG.md#srcnlisthxx): NUMERICLIST owned
  // `table` (heap-allocated, freed in ~NUMERICLIST()) with no
  // user-declared copy constructor or operator= -- the compiler-
  // generated ones shallow-copied `table`, confirmed to cause a
  // heap-use-after-free (ASan) on copy. No live call site ever copies
  // a NUMERICLIST, so it's made explicitly non-copyable rather than
  // given deep-copy semantics.
  NUMERICLIST(const NUMERICLIST&) = delete;
  NUMERICLIST& operator=(const NUMERICLIST&) = delete;
  void         SetRelation(INT r)      { Relation = r; };
  INT          GetRelation()           { return(Relation); };
  DOUBLE       GetNumericValue(INT i)  { return(table[i].GetNumericValue()); };
  DOUBLE       GetMaxValue();
  DOUBLE       GetMinValue();
  // return the USE attribute for this table
  INT          GetAttribute()          { return(Attribute); };
  // set the USE attribute
  void         SetAttribute(INT x)     { Attribute = x; }; 
  // get number of items in table
  INT4         GetCount()              { return(Count); };
  // make room for more entries
  void         Expand()                { Resize(Count + (50*Ncoords)); };
  // collapse size to total entries
  void         Cleanup()               { Resize(Count); };
  INT4         GetGlobalStart(INT4 i)  { return(table[i].GetGlobalStart()); };
  // set file name to load from
  void         SetFileName(STRING s)   { FileName = s; };
  // set file name to load from
  void         SetFileName(PCHR s)     { FileName = s; };
  void         SetCoords(INT n)        { Ncoords = n; };
  INT          GetCoords()             { return Ncoords; };

  SearchState  Find(STRING Fn, DOUBLE Key, INT4 Relation, INT4 *Index);
  SearchState  Find(STRING Fn, INT4 Key, INT4 Relation, INT4 *Index);
  SearchState  Find(DOUBLE Key, INT4 Relation, INT4 *Index);
  SearchState  Find(INT4 Key, INT4 Relation, INT4 *Index);
  void         Sort();                     // sort numeric field items
  void         SortByGP();                 // sort by global ptr
  void         Dump();                     // dump all numeric field data
  void         Dump(INT4 start, INT4 end); // dump numeric field data
  void         Resize(INT4 Entries);       // resize table to Entries size
  void         TempLoad();                 // test routine
  INT4         LoadTable(INT4 Start, INT4 End);
  INT4         LoadTable(INT4 Start, INT4 End, NumBlock Offset);
  void         WriteTable();
  void         WriteTable(INT Offset);
  void         ResetHitPosition();
  INT4         GetNextHitPosition();
  ~NUMERICLIST();
};

typedef NUMERICLIST* PNUMERICLIST;
#endif
