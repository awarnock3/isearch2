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
File:		rset.hxx
Version:	1.00
Description:	Class RSET - Search Result Set
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef RSET_HXX
#define RSET_HXX

#include "defs.hxx"   // BUGFIX #1: was commented out; INT/SIZE_T/DOUBLE below need it.
#include "string.hxx" // BUGFIX #1: was commented out; STRING below needs it.
#include "result.hxx" // BUGFIX #1: was commented out; RESULT/PRESULT below need it.

// A dynamically resizing array of RESULT entries (a materialized,
// user-facing search result set), sortable by key or score.
class RSET {
public:
	RSET();
	// BUGFIX #2: added -- previously absent, so copy-constructing an RSET
	// (e.g. `RSET b = a;`, pass/return by value) used the compiler-
	// generated shallow copy of Table, and both copies' destructors then
	// deleted the same heap array: confirmed double-free/use-after-free
	// under ASan. Deep-copies entries the same way operator= does, below.
	RSET(const RSET& OtherRset);
	// BUGFIX #2: added for the same reason -- previously absent entirely
	// (not even a shallow one), so `b = a;` used the compiler-generated
	// shallow copy-assignment, confirmed to double-free the same way.
	RSET& operator=(const RSET& OtherRset);
	// Replaces this table's contents with FileName's contents, previously
	// written by SaveTable(). See BUGFIX #4 for why this matters: this
	// isn't a raw memory dump of Table (RESULT's STRING fields own heap
	// buffers that can't survive one), but a field-by-field serialization
	// through RESULT's own public getters/setters.
	void LoadTable(const STRING& FileName);
	void SaveTable(const STRING& FileName);
	void AddEntry(const RESULT& ResultRecord);
	// Copies the Index'th entry (1-based) into *ResultRecord; leaves
	// *ResultRecord untouched if Index is out of [1, GetTotalEntries()]
	// range.
	void GetEntry(const INT Index, PRESULT ResultRecord) const;
	// Sorts entries ascending by key.
	void SortByKey();
	// Sorts entries descending by score (best match first), matching
	// IRSET::SortByScore's convention for the same conceptual operation.
	void SortByScore();
	INT GetScaledScore(const DOUBLE UnscaledScore, const INT ScaleFactor);
	void Expand();
	void CleanUp();
	void Resize(const SIZE_T Entries);
	// 1-based, like GetEntry.
	void SetEntry(const INT x, const RESULT& ResultRecord );
	SIZE_T GetTotalEntries();
	void SetScoreRange(DOUBLE High, DOUBLE Low);
//	void Dump() const;
	~RSET();
private:
	PRESULT Table;
	SIZE_T TotalEntries;
	SIZE_T MaxEntries;
	DOUBLE HighScore, LowScore;
};

typedef RSET* PRSET;

#endif
