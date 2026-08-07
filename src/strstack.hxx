/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1995. 

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


// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef STRSTACK_HXX
#define STRSTACK_HXX

#include "gdt.h"
// BUGFIX #1: these were commented out, even though STRING/STRLIST are
// used below (STRSTACK::Push's parameter, StackList's type) -- this
// header only compiled standalone because every real includer happens
// to pull in string.hxx/strlist.hxx first. See docs/BUG_CATALOG.md#srcstrstackhxx.
#include "strlist.hxx"
#include "string.hxx"

// A LIFO stack of STRING values, backed by STRLIST (a 1-based, array-
// like list): Push()/Pop() grow/shrink via a CurrIndex cursor rather
// than actually adding/removing STRLIST nodes, so popped slots are
// reused (overwritten) by later pushes instead of being freed.
class STRSTACK {
public:
	STRSTACK();
	void Push(const STRING &Value);
	// Copies the top entry into *Value and pops it. Returns GDT_FALSE
	// (leaving *Value untouched) if the stack is empty.
	GDT_BOOLEAN Pop(STRING  *Value);
	INT GetTotalEntries(void);
	// Copies the top entry into *Value without popping it. Returns
	// GDT_FALSE (leaving *Value untouched) if the stack is empty.
	GDT_BOOLEAN Examine(STRING *Value);
	GDT_BOOLEAN IsEmpty(void);
private:
	INT CurrIndex;
	STRLIST StackList;
};
#endif
