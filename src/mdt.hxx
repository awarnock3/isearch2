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
File:		mdt.hxx
Version:	1.01
Description:	Class MDT - Multiple Document Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef MDT_HXX
#define MDT_HXX

#include "defs.hxx"
#include "string.hxx"
#include "mdtrec.hxx"
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

class GPREC {
public:
	GPTYPE GpStart;
	GPTYPE GpEnd;
	GPTYPE Index;
};

class KEYREC {
public:
	CHR Key[DocumentKeySize];
	GPTYPE Index;
};

// The Multiple Document Table: one entry per indexed document, backed
// by an on-disk .mdt record file plus two in-memory sort indexes
// (KeyIndex, GpIndex) mirrored to .mdk/.mdg on FlushMDTIndexes().
class MDT {
public:
	MDT(const STRING& DbFileStem, const GDT_BOOLEAN WrongEndian);
	// BUGFIX #1: MDT owns two heap-allocated arrays (KeyIndex, GpIndex)
	// and a raw FILE* (MdtFp), but declared no copy constructor and no
	// operator= at all -- not even a hand-written one -- so the
	// compiler-generated ones did a member-wise shallow copy of all
	// three. Confirmed to double-free GpIndex under ASan (one level
	// inside FlushMDTIndexes() -> SortGpIndex() -> qsort()); the shared
	// MdtFp would double-fclose() by the same mechanism. Made
	// explicitly non-copyable rather than deep-copied: nothing in the
	// tree copies an MDT by value today, and there's no well-defined
	// answer for what a copy of an open file handle should mean. See
	// docs/BUG_CATALOG.md.
	MDT(const MDT&) = delete;
	MDT& operator=(const MDT&) = delete;
//	void LoadTable(const STRING& FileName);	// This is now done automatically
//	void SaveTable(const STRING& FileName);	// in the constructor and destructor.
	void AddEntry(const MDTREC& MdtRecord);
	void IndexSortByIndex();
	SIZE_T RemoveDeleted();
	void GetEntry(const SIZE_T Index, MDTREC* MdtrecPtr) const;
	void SetEntry(const SIZE_T Index, const MDTREC& MdtRecord);
//	void Expand() { };
//	void CleanUp() { };
	void Resize(const SIZE_T Entries);
	void SortKeyIndex();
	SIZE_T LookupByKey(const STRING& Key);
	SIZE_T GetMdtRecord(const STRING& Key, MDTREC* MdtrecPtr);
	SIZE_T GetMdtRecord(const GPTYPE gp, MDTREC* MdtrecPtr);
	void SortGpIndex();
	SIZE_T LookupByGp(const GPTYPE Gp);
//	INT GetMdtIndex(const GPTYPE gp);	// Use LookupByGp() instead
	GPTYPE GetNextGlobal() const;
	SIZE_T GetTotalEntries() const;
	void GetUniqueKey(STRING* StringPtr);
	void Dump() const;
	INT GetChanged() const;
	void FlipIndexBytes();
	void FlushMDTIndexes();
	~MDT();
private:
	STRING FileStem;
	FILE* MdtFp;
	KEYREC* KeyIndex;
	GPREC* GpIndex;
	SIZE_T TotalEntries;
	GDT_BOOLEAN KeyIndexSorted;
	GDT_BOOLEAN GpIndexSorted;
	GDT_BOOLEAN Changed;
	SIZE_T MaxEntries;
	GDT_BOOLEAN MdtWrongEndian;
	GDT_BOOLEAN ReadOnly;
};

typedef MDT* PMDT;

#endif
