// $Id: intlist.hxx,v 1.7 2000/02/04 23:39:22 cnidr Exp $
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
File:		intlist.hxx
Version:	1.00
$Revision: 1.7 $
Description:	Class INTERVALLIST - Utilities for lists of numeric intervals
Author:		Archie Warnock (warnock@clark.net), derived from J. Fullton's
                NLIST class
@@@*/

#ifndef INTERVALLIST_HXX
#define INTERVALLIST_HXX

#include <stdlib.h>
#include <time.h>
#include <iostream.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"

// IntBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
enum IntBlock { START_BLOCK, END_BLOCK, PTR_BLOCK };


class INTERVALLIST 
  : public NUMERICLIST 
{
private:
    
  PINTERVALFLD table;      // the table of attribute/numeric data
  INT4         Count;      // count of items in table
  INT          Attribute;  // which USE attribute this table maps
  INT4         Pointer;    // current position
  INT4         MaxEntries; // current maximum size of table - see Resize()
  INT4         StartIndex;
  INT4         EndIndex;
  INT          Relation;   // what relation generated StartIndex, EndIndex
  STRING       FileName;   // the file which attributes/numeric data are in
  INT          Ncoords;    // Number of values in an entry
    
  //  These are called internally by Find
  //  SearchState  MemFind(DOUBLE Key, INT4 Relation, 
  //		       GDT_BOOLEAN ByStart, INT4 *Index);
  //  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
  //			GDT_BOOLEAN ByStart, INT4 *Index);
  SearchState  MemFind(DOUBLE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index);
  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
			IntBlock FindBlock, INT4 *Index);
  SearchState  MemFind(INT4 Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index);
  SearchState  DiskFind(STRING Fn, INT4 Key, INT4 Relation, 
			IntBlock FindBlock, INT4 *Index);
  SearchState  IntervalMatcher(DOUBLE Key, DOUBLE LowerBound, 
			       DOUBLE Mid, DOUBLE UpperBound,
			       INT4 Relation, IntType Type);
  SearchState  IntervalMatcher(INT4 Key, INT4 LowerBound, 
			       INT4 Mid, INT4 UpperBound,
			       INT4 Relation, IntType Type);

public:
  INTERVALLIST();
  INTERVALLIST(INT n);
  void   SortByStart();              // sort numeric field items
  void   SortByEnd();                // sort numeric field items
  void   SortByGP();                 // sort numeric field items
  DOUBLE GetStartMaxValue();
  DOUBLE GetStartMinValue();
  DOUBLE GetEndMaxValue();
  DOUBLE GetEndMinValue();
  INT4   LoadTable(INT4 Start, INT4 End);
  INT4   LoadTable(INT4 Start, INT4 End, IntBlock Offset);
  void   WriteTable();
  void   WriteTable(INT Offset);
  INT4   GetGlobalStart(INT4 i) { return(table[i].GetGlobalStart()); }
  DOUBLE GetStartValue(INT i)   { return(table[i].GetStartValue()); }
  DOUBLE GetEndValue(INT i)     { return(table[i].GetEndValue()); }
  void   SetCoords(INT n)       { Ncoords = n; }
  INT    GetCoords()            { return Ncoords; }
  // set file name to load from
  void   SetFileName(STRING s)  { FileName = s; }
  // set file name to load from
  void   SetFileName(CHR *s)    { FileName = s; }
  // get number of items in table
  INT4   GetCount() { return(Count); }
  // make room for more entries
  void   Expand() { Resize(Count + (50*Ncoords)); }
  // collapse size to total entries
  void   Cleanup() { Resize(Count); }
  void   Resize( INT4 Entries);      // resize table to Entries size

  // This calls the old DiskFind and, now, MemFind
  SearchState  Find(STRING Fn, DOUBLE Key, INT4 Relation, 
		    IntBlock FindBlock, INT4 *Index);
  SearchState  Find(STRING Fn, INT4 Key, INT4 Relation, 
		    IntBlock FindBlock, INT4 *Index);

  SearchState  Find(DOUBLE Key, INT4 Relation, IntBlock FindBlock, 
		    INT4 *Index);
  SearchState  Find(INT4 Key, INT4 Relation, IntBlock FindBlock, 
		    INT4 *Index);

  void         Dump();                     // dump numeric field data
  void         Dump(INT4 start, INT4 end); // dump numeric field data
  ~INTERVALLIST();
};

typedef INTERVALLIST* PINTERVALLIST;
#endif



