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

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef TOKENGEN_HXX
#define TOKENGEN_HXX

#include "gdt.h"
// BUGFIX #4 (docs/BUG_CATALOG.md#srctokengenhxx): these were commented
// out despite TOKENGEN declaring a STRLIST-typed member (TokenList) and
// a STRING-typed constructor parameter -- this header failed to compile
// as the sole #include in a translation unit (4 errors) before this
// fix, the same defect already fixed in src/fc.hxx's own BUGFIX #1 and
// many other files this project.
#include "strlist.hxx"
#include "string.hxx"

/// Splits a search-query string into tokens (words, quoted literals,
/// `{...}` groups, and the `( ) !`/`&& ||`-family operator tokens),
/// lazily parsed on first use (see DoParse()). Non-copyable: no live
/// call site ever copies one (see BUGFIX #1), so the copy constructor
/// and operator= are both deleted rather than given deep-copy
/// semantics.
class TOKENGEN {
public:
	TOKENGEN(const STRING &InString);
	// BUGFIX #1 (docs/BUG_CATALOG.md#srctokengenhxx): TOKENGEN owned
	// `InCharP` (a NewCString() duplicate, freed in ~TOKENGEN()) with no
	// user-declared copy constructor or operator= -- the compiler-
	// generated ones shallow-copied `InCharP`, confirmed to cause a real
	// double-free (ASan) on copy. No live call site ever copies a
	// TOKENGEN, so it's made explicitly non-copyable rather than given
	// deep-copy semantics.
	TOKENGEN(const TOKENGEN&) = delete;
	TOKENGEN& operator=(const TOKENGEN&) = delete;
	~TOKENGEN();
	void GetEntry(const SIZE_T Index, STRING* StringEntry);
	void SetQuoteStripping(GDT_BOOLEAN);
	SIZE_T GetTotalEntries(void);


private:
	CHR *nexttoken(CHR *input, STRING *token);
	STRLIST TokenList;
	GDT_BOOLEAN DoStripQuotes, HaveParsed;
	void DoParse(void);
	CHR *InCharP;
};

#endif
