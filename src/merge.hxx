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

#include "defs.hxx"
#include "strstack.hxx"

class MERGE {
public:
	void AddChunk(PCHR MemoryData, INT MemoryDataLength,
		GPTYPE *MemoryIndex, INT MemoryIndexLength,
		GPTYYPE GlobalStart);
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


void buildHeap(int *data, size_t nel, size_t width,
	int (*compar) (const void *, const void * ), int position, int reverse=0);



void buildGpHeap(GPTYPE *data, size_t heapsize, 
	int (*compar)(const void *, const void *), int position, int reverse);
void GpHsort(GPTYPE *data, size_t nel, int (*compar) 
	(const void *, const void *));

#endif
