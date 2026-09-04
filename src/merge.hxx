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
File:		merge.hxx
Version:	1.00
Description:	Class MERGE
Author:		Jon Magid, jem@cnidr.org
@@@*/

#ifndef MERGE_HXX
#define MERGE_HXX

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "defs.hxx"
#include "strstack.hxx"

/**
 * @brief Heapsort utilities used for external/internal merge sorting
 * (hsort/buildHeap for a generic width-parameterized buffer,
 * GpHsort/buildGpHeap specialized for a GPTYPE array), plus a MERGE
 * class that is declared but never implemented or called anywhere in
 * this tree -- see docs/BUG_CATALOG.md#srcmergecxx for the bugs found
 * and fixed in this header and its .cxx.
 */

// BUGFIX #2 (docs/BUG_CATALOG.md#srcmergecxx): merge.cxx never
// included this header at all, so the compiler never had a chance to
// catch either of this file's own defects: GPTYYPE (a typo for
// GPTYPE, the only occurrence of that misspelling anywhere in the
// tree) and buildHeap()'s declaration below not matching its actual
// definition (see BUGFIX #3). Neither was reachable since nothing
// else in this tree includes merge.hxx or calls into MERGE/hsort/
// buildHeap/buildGpHeap/GpHsort (confirmed via grep), so fixing this
// header in place -- unlike a header with real external callers --
// changes no other file's visible contract.
class MERGE {
public:
	// MERGE::AddChunk is declared but has never been defined anywhere
	// in this tree, and nothing calls it -- vestigial. Left as a
	// declaration only; implementing it would mean inventing behavior
	// with no specification and no caller to validate it against.
	void AddChunk(PCHR MemoryData, INT MemoryDataLength,
		GPTYPE *MemoryIndex, INT MemoryIndexLength,
		GPTYPE GlobalStart);
/*	void AppendChunk(MERGEFP mfp, INT MemoryDataLength,
		GPTYPE *MemoryIndex, INT MemoryIndexLength,
		GPTYPE GlobalStart);
	void FinishRun(MERGEFP mfp);
*/

private:
	STRSTACK MergeFiles;

};

void hsort(void *data, size_t nel, size_t width,
	int (*compar) (const void *, const void *));


// BUGFIX #3 (docs/BUG_CATALOG.md#srcmergecxx): declared here as taking
// `int *data`, but merge.cxx's actual definition takes `void *data`
// (matching the generic, width-parameterized byte arithmetic the
// function body does internally -- the same qsort-style contract
// hsort()/buildGpHeap()'s own declarations already use). Since
// merge.cxx never included this header (see BUGFIX #2), the two never
// sat in the same translation unit for the compiler to catch the
// mismatch. Fixed by matching the header to what's actually
// implemented, not the other way around: `int *data` would mean
// reinterpreting arbitrary-width elements as ints, which is wrong for
// any width other than sizeof(int).
void buildHeap(void *data, size_t nel, size_t width,
	int (*compar) (const void *, const void * ), int position, int reverse=0);



void buildGpHeap(GPTYPE *data, size_t heapsize, 
	int (*compar)(const void *, const void *), int position, int reverse);
void GpHsort(GPTYPE *data, size_t nel, int (*compar) 
	(const void *, const void *));

#endif
