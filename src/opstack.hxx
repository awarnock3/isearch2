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
File:		opstack.hxx
Version:	1.01
Description:	Class OPSTACK - Operand/operator Stack
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef OPSTACK_HXX
#define OPSTACK_HXX

#include "defs.hxx"  // BUGFIX #1: was commented out; below needs it transitively.
#include "string.hxx" // BUGFIX #1: was commented out; below needs it transitively.
#include "opobj.hxx" // BUGFIX #1: was commented out; OPOBJ/POPOBJ below need it.
#include "irset.hxx" // BUGFIX #1: was commented out; PIRSET below needs it.

// A singly linked stack of owned, heap-allocated OPOBJ* entries (query
// operators/operands), used while evaluating a query's RPN expression.
// operator<<(OPOBJ&) pushes a Duplicate() of its argument; operator<<
// (OPOBJ*) takes ownership of the pointer directly. Either way, popped
// entries (via operator>>) become the caller's responsibility to delete.
class OPSTACK {
public:
	OPSTACK();
	// BUGFIX #2: added -- previously absent, so copy-constructing an
	// OPSTACK (e.g. `OPSTACK b = a;`) used the compiler-generated shallow
	// copy of Head. This was harmless only because ~OPSTACK() didn't free
	// anything (see BUGFIX #3); now that it does, an unguarded shallow
	// copy would double-free the shared chain of nodes the same way
	// DFT/RSET/IRSET's missing copy constructors did. Deep-copies
	// entries the same way operator= does, below.
	OPSTACK(const OPSTACK& OtherOpstack);
	OPSTACK& operator=(const OPSTACK& OtherOpstack);
	OPSTACK& operator<<(OPOBJ& Op);
	OPSTACK& operator<<(OPOBJ* Op);
	POPOBJ operator>>(POPOBJ& OpPtr);
 	PIRSET operator>>(PIRSET& OpPtr);
	void Reverse();
	~OPSTACK();
private:
	void Push(OPOBJ& Op);
	void Push(OPOBJ *Op);
	POPOBJ Pop();
	POPOBJ Head;
};

typedef OPSTACK* POPSTACK;

#endif
