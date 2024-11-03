// $Id: infix2rpn.hxx,v 1.7 1998/05/12 16:49:08 cnidr Exp $
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


#ifndef INFIX2RPN_HXX
#define INFIX2RPN_HXX

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "strstack.hxx"
#include "tokengen.hxx"

#define MAX_OP_LEN 8

enum operators { NOP, LeftParen, BoolOR, BoolAND, BoolNOT, ProxNEAR
#ifdef UNARYNOT
, UnNOT
#endif
, DEFAULT};

class INFIX2RPN {

public:
  INFIX2RPN();
  INFIX2RPN(const STRING &StrInput, STRING *StrOutput);
  INFIX2RPN(const STRING &StrInput, STRING *StrOutput, const CHR *Op);
  void        Parse(const STRING &StrInput, STRING *StrOutput);
  GDT_BOOLEAN InputParsedOK(void);
  GDT_BOOLEAN GetErrorMessage(STRING *Error) const;
  void        SetDefaultOp(const CHR *Op);
  void        GetDefaultOp(CHR *Op);

private:
  void        ProcessOp(const operators op, STRSTACK *TheStack,  
			STRING *result);
  void        RegisterError(const STRING &Error);
  CHR        *op2string(const operators op);
  CHR        *StandardizeOpName(const STRING op);
  INT         TermsWithNoOps;
  STRING      ErrorMessage;
  CHR         DefaultOp[MAX_OP_LEN];
};


#endif //INFIX2RPN_HXX
