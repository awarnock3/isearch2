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
File:		reclist.hxx
Version:	1.00
Description:	Class RECLIST - Database Record List
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef RECLIST_HXX
#define RECLIST_HXX
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// BUGFIX #1: was commented out; INT (defs.hxx) and RECORD/PRECORD
// (record.hxx) below need it. string.hxx isn't restored alongside them
// -- nothing in this header uses STRING directly, and record.hxx
// already brings it in transitively for anything that does.
#include "defs.hxx"
#include "record.hxx"

// A resizable array of RECORD entries used to build up a database's
// record list. GetEntry() is 1-based (Index==1 is the first entry) --
// RECLIST's own convention, independent of STRING's.
class RECLIST {
public:
	RECLIST();
	// BUGFIX #2: RECLIST owned a heap-allocated Table array but declared
	// no copy constructor or operator=, so the compiler-generated ones
	// did a shallow pointer copy -- confirmed to double-free Table under
	// ASan. Both now deep-copy Table (sized to the source's MaxEntries),
	// TotalEntries, and MaxEntries. See docs/BUG_CATALOG.md.
	RECLIST(const RECLIST& OtherReclist);
	RECLIST& operator=(const RECLIST& OtherReclist);
	void AddEntry(const RECORD& RecordEntry);
	void GetEntry(const INT Index, PRECORD RecordEntry) const;
	void Expand();
	void CleanUp();
	void Resize(const INT Entries);
	INT GetTotalEntries() const;
	~RECLIST();
private:
	PRECORD Table;
	INT TotalEntries;
	INT MaxEntries;
};

typedef RECLIST* PRECLIST;

#endif
