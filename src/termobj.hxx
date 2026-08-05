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
File:		termobj.hxx
Version:	1.00
Description:	Class TERMOBJ - Search Term Base Class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef TERMOBJ_HXX
#define TERMOBJ_HXX

#include "defs.hxx"   // BUGFIX #1: was commented out; INT/TypeTerm below need it.
#include "string.hxx" // BUGFIX #1: was commented out; OPERAND's interface needs it.
#include "operand.hxx" // BUGFIX #1: was commented out; base class.

// Abstract intermediate base for concrete search-term operand classes
// (e.g. STERM), distinguished from IRSET (the other OPERAND subtype) by
// GetOperandType() returning TypeTerm instead of TypeRset. Adds no state
// of its own beyond what OPERAND already provides.
class TERMOBJ : public OPERAND {
public:
	// BUGFIX #2: un-hides OPERAND::operator=(const OPOBJ&). Without
	// this, the compiler-generated TERMOBJ::operator=(const TERMOBJ&)
	// hides the inherited virtual overload from ordinary (non-virtual)
	// lookup -- confirmed by the standing "-Woverloaded-virtual" warning
	// this class has been causing in every build since src/operand.hxx's
	// turn introduced that operator=. Assigning through a polymorphic
	// OPOBJ&/OPERAND& reference already dispatched correctly regardless
	// (virtual dispatch doesn't care about hiding); this only affects
	// direct same-type assignment.
	using OPERAND::operator=;
	TERMOBJ();
	INT GetOperandType() const { return TypeTerm; };
	virtual ~TERMOBJ();
};

typedef TERMOBJ* PTERMOBJ;

#endif
