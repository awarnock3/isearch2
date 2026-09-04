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
File:		operator.hxx
Version:	1.00
Description:	Class OPERATOR - Query Operator
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-06
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef OPERATOR_HXX
#define OPERATOR_HXX

#include "defs.hxx"   // BUGFIX #1: was commented out; INT/TypeOperator below need it.
#include "string.hxx" // BUGFIX #1: was commented out; OPOBJ's interface needs it.
#include "opobj.hxx"  // BUGFIX #1: was commented out; base class.

// A query operator (AND/OR/ANDNOT) node for the RPN expression stack,
// distinguished from OPERAND (search terms/result sets) by GetOpType()
// returning TypeOperator. Fully concrete, unlike OPERAND/TERMOBJ: it
// implements every OPOBJ pure virtual itself.
class OPERATOR : public OPOBJ {
public:
	// BUGFIX #2: un-hides OPOBJ::operator=(const OPOBJ&) -- same
	// "-Woverloaded-virtual" pattern already fixed for TERMOBJ
	// (src/termobj.hxx). Even though OPERATOR declares its own
	// operator=(const OPOBJ&) override just below, that's still not
	// OPERATOR's *own* copy-assignment operator by the standard's
	// definition (the parameter type isn't OPERATOR), so the compiler
	// still generates an implicit operator=(const OPERATOR&) that hides
	// it from ordinary lookup.
	using OPOBJ::operator=;
	OPERATOR();
	INT GetOpType() const { return TypeOperator; };
	INT GetOperandType() const { return 0; };
	OPOBJ* Duplicate() const;
	virtual OPOBJ& operator=(const OPOBJ& OtherOp);
	void SetOperatorType(const INT OperatorType);
	INT GetOperatorType() const;
	~OPERATOR();
private:
	INT OperatorType;
};

typedef OPERATOR* POPERATOR;

#endif
