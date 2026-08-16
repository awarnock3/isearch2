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

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "gstack.hxx"
#include "gdt.h"

/* 
   Stack Routines
*/
GSTACK::GSTACK() : CurrentIndex(nullptr)
{
  // BUGFIX #1: CurrentIndex used to be left uninitialized here. Never
  // read before Push()/Top()/Pop() all unconditionally overwrite it
  // first, so this was never reachable as a live bug, but it's the
  // same class of fix as every other "constructor leaves a raw pointer
  // uninitialized" turn this batch (src/index.cxx, src/infix2rpn.cxx).
  // See docs/BUG_CATALOG.md#srcgstackhxx.
}

INT GSTACK::GetSize(void)
{
  return(Stack.GetLength());
}

void GSTACK::Push(GATOM* a)
{
  CurrentIndex = Stack.First();
  Stack.InsertBefore(CurrentIndex,a);
  CurrentIndex = Stack.First();
}

GATOM* GSTACK::Top(void)
{
  // BUGFIX #2: this used to call Stack.Retrieve(nullptr) on an empty
  // stack -- GLIST::Retrieve() dereferences its argument unconditionally
  // -- a real null-pointer-dereference crash, confirmed with a
  // standalone repro (`GSTACK().Top()`) before fixing:
  // AddressSanitizer: SEGV ... in GLIST::Retrieve. Every real caller
  // found in the tree (doctype/cipc.cxx, cipp.cxx, anzmeta.cxx,
  // anzlic.cxx, fgdc.cxx) happens to only reach Top()/Pop() after
  // confirming GetSize() != 0 first via its own surrounding logic, so
  // this wasn't observed to crash in practice, but GSTACK itself
  // shouldn't rely on every future caller getting that right. See
  // docs/BUG_CATALOG.md#srcgstackhxx.
  CurrentIndex = Stack.First();
  if (CurrentIndex == nullptr)
    return nullptr;
  return(Stack.Retrieve(CurrentIndex));
}

GATOM* GSTACK::Pop(void)
{
  // BUGFIX #2 (same as Top(), see above): guard the empty-stack case
  // here too, rather than dereferencing a null CurrentIndex via
  // Retrieve()/Delete().
  GATOM *p;
  CurrentIndex = Stack.First();
  if (CurrentIndex == nullptr)
    return nullptr;
  p = Stack.Retrieve(CurrentIndex);
  Stack.Delete(CurrentIndex);
  CurrentIndex = Stack.First();
  return p;
}
