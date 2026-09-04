// $Id: fpt.cxx,v 1.8 1998/05/12 16:49:03 cnidr Exp $
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
File:		fpt.cxx
Version:	1.00
$Revision: 1.8 $
Description:	Class FPT - File Pointer Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <limits.h>
/*
#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "fprec.hxx"
*/
#include "fpt.hxx"


FPT::FPT() {
#ifdef _POSIX_OPEN_MAX
  Init(_POSIX_OPEN_MAX - 5);
#else
#ifndef _NFILE /* Still not defined... */
#define _NFILE 11 /* 16-5 */
#endif
  Init(_NFILE - 5);
#endif

  //	Init(2);
}


FPT::FPT(const INT TableSize) {
  Init(TableSize);
}


void 
FPT::Init(const INT TableSize) {
  Table = new FPREC[TableSize];
  MaximumEntries = TableSize;
  TotalEntries = 0;
}


INT 
FPT::Lookup(const STRING& FileName) {
  // Expand FileName
  STRING Fn;
  Fn = FileName;
  ExpandFileSpec(&Fn);
  // Loop through table
  INT x;
  STRING Tfn;
  for (x=0; x<TotalEntries; x++) {
    Table[x].GetFileName(&Tfn);
    if (Tfn == Fn) {
      return (x + 1);
    }
  }
  return 0;
}


INT 
FPT::Lookup(const FILE *FilePointer) {
  // Loop through table
  INT x;
  for (x=0; x<TotalEntries; x++) {
    if (FilePointer == Table[x].GetFilePointer()) {
      return (x + 1);
    }
  }
  return 0;
}


void 
FPT::HighPriority(const INT Index) {
  INT x, y;
  INT IndexPriority;
  IndexPriority = Table[Index-1].GetPriority();
  for (x=0; x<TotalEntries; x++) {
    if ((y=Table[x].GetPriority()) <= IndexPriority) {
      Table[x].SetPriority(y + 1);
    }
  }
  Table[Index-1].SetPriority(0);
}


void 
FPT::LowPriority(const INT Index) {
  INT x, y;
  INT Highest, IndexPriority;
  IndexPriority = Table[Index-1].GetPriority();
  Highest = IndexPriority;
  for (x=0; x<TotalEntries; x++) {
    if ((y=Table[x].GetPriority()) > IndexPriority) {
      Table[x].SetPriority(y - 1);
      if (y > Highest) {
	Highest = y;
      }
    }
  }
  Table[Index-1].SetPriority(Highest);
}


/**
 * @brief Opens FileName in the given mode, reusing a cached handle if
 * this table already has FileName open in the same mode.
 * @param FileName File to open.
 * @param Type fopen()-style mode string ("r", "w", "a", ...).
 * @return The (possibly cached) FILE*, or nullptr on a genuine
 * fopen() failure once every cache path has been exhausted.
 *
 * Evicts the table's least-recently-touched entry if it's already at
 * MaximumEntries capacity. The evicted entry is only physically
 * fclose()'d if it was already logically closed (GetClosed()) --
 * otherwise its FilePointer is silently overwritten below without
 * being closed, a known leak when every cached entry is still
 * logically open at eviction time (see docs/BUG_CATALOG.md).
 */
PFILE
FPT::ffopen(const STRING& FileName, const CHR *Type) {
  // Check if file is already open
  INT z;
  z = Lookup(FileName);
  if (z) {
    // If found, check OpenMode
    FPREC Fprec;
    STRING Fn, Om;
    PFILE Fp;
    Fprec = Table[z-1];
    Fp = Fprec.GetFilePointer();
    Fprec.GetFileName(&Fn);
    Fprec.GetOpenMode(&Om);
    // BUGFIX #2: `GDT_BOOLEAN Closed = Fprec.GetClosed();` used to be
    // read here and never consulted by any branch below (confirmed by
    // tracing every path: the "w"/"a" branches unconditionally
    // fclose()+reopen, the "r" branch always reuses the cached Fp, and
    // that reuse is safe regardless of Closed's value -- ffclose()
    // never physically closes an entry still reachable via Lookup(),
    // only CloseAll() (which also zeroes TotalEntries, hiding the slot
    // from Lookup()) or an eviction/mode-change (which replaces the
    // slot's FilePointer before anyone could reuse the stale one) do.
    // Dead read, not a missing check; removed. See docs/BUG_CATALOG.md.
    if (Om == Type) {
      // If same OpenMode, use the cached information
      if (Om.SearchReverse("w")) {
	fclose(Fp);
	Table[z-1].SetClosed(GDT_FALSE);
	HighPriority(z);
	Table[z-1].SetFilePointer(Fp=fopen(FileName, Type));
	return Fp;
      }
      if (Om.SearchReverse("a")) {
	fclose(Fp);
	Table[z-1].SetClosed(GDT_FALSE);
	HighPriority(z);
	Table[z-1].SetFilePointer(Fp=fopen(FileName, Type));
	return Fp;
      }
      if (Om.SearchReverse("r")) {
	Table[z-1].SetClosed(GDT_FALSE);
				//	fseek(Fp, 0, 0);
	fseek(Fp, 0L, SEEK_SET);
	HighPriority(z);
	return Fp;
      }
    } else {
      // If different OpenMode, update table
      fclose(Fp);
      Table[z-1].SetClosed(GDT_FALSE);
      HighPriority(z);
      Table[z-1].SetOpenMode(Type);
      Table[z-1].SetFilePointer(Fp=fopen(FileName, Type));
      return Fp;
    }
  } else {
    // If not found, open and cache the FilePointer
    INT NewEntry;
    if (TotalEntries >= MaximumEntries) {
      // If table at maximum size, purge oldest element
      NewEntry = 0;
      INT x;
      for (x=0; x<TotalEntries; x++) {
	if (Table[x].GetPriority() > Table[NewEntry].GetPriority()) {
	  NewEntry = x;
	}
      }
      if (Table[NewEntry].GetClosed()) {
	fclose(Table[NewEntry].GetFilePointer());
      }
    } else {
      // Otherwise, add an element
      TotalEntries++;
      NewEntry = TotalEntries - 1;
    }
    Table[NewEntry].SetFileName(FileName);
    Table[NewEntry].SetOpenMode(Type);
    Table[NewEntry].SetPriority(MaximumEntries+1);
    HighPriority(NewEntry+1);
    PFILE Fp;
    Table[NewEntry].SetClosed(GDT_FALSE);
    Table[NewEntry].SetFilePointer(Fp=fopen(FileName, Type));
    return Fp;
  }
  return 0;
}


INT 
FPT::ffclose(FILE *FilePointer) {
  INT z;
  z = Lookup(FilePointer);
  if (z) {
    Table[z-1].SetClosed(GDT_TRUE);
    LowPriority(z);
  } else {
    fclose(FilePointer);
  }
  return 0;
}


void 
FPT::CloseAll() {
  INT x;
  for (x=0; x<TotalEntries; x++) {
    if (Table[x].GetClosed() == GDT_TRUE) {
      fclose(Table[x].GetFilePointer());
    }
  }
  TotalEntries = 0;
}


FPT::~FPT() {
  CloseAll();
  delete [] Table;
}
