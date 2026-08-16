/* $Id: glist.cxx,v 1.4 1998/05/12 16:49:04 cnidr Exp $ */
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

#include "glist.hxx"
#include "gdt.h"

/*
   PRE:	 None.
   POST: Pointer to allocated GLIST
*/
GLIST::GLIST()
{
  Length=0;
  Head=nullptr;
  Tail=nullptr;
}

/*
   PRE:	 l has been created.
   POST: Nonzero if empty
	 Zero if not empty
*/
GDT_BOOLEAN GLIST::IsEmpty(void)
{
  if(Length==0)
    return GDT_TRUE;
  return GDT_FALSE;
}

/*
   PRE:	 l has been created.
	 l contains at least one cell.
   POST: Pointer to first cell in list on success.
	 NULL pointer on failure.
*/
GPOSITION* GLIST::First()
{
  return Head;
}

/*
   PRE:	 l has been created.
	 l contains at least one cell.
   POST: Pointer to last cell in list on success.
	 NULL pointer on failure.
*/
GPOSITION* GLIST::Last()
{
  if(Tail == nullptr)
    // Attempt to access first position of empty list
    return nullptr;

  return Tail;
}

/*
   PRE:	 l has been created.
	 c is a pointer to a cell in l.
   POST: Pointer to next cell in list on success.
	 NULL pointer if no more cells in list.
*/
GPOSITION* GLIST::Next(GPOSITION *c)
{
  if(Tail == c)
    // Attempt to access one position past end of list
    return nullptr;

  return c->Next;
}

/*
   PRE:	 l has been created.
	 c is a pointer to a cell in l.
   POST: Pointer to previous cell in list on success.
	 NULL pointer if no more cells in list before c.
*/
GPOSITION* GLIST::Prev(GPOSITION *c)
{
  if(Head == c)
    // Attempt to access one position before start of list
    return nullptr;

  return c->Prev;
}

GDT_BOOLEAN GLIST::InsertBefore(GPOSITION *c, GATOM *a, int Type)
{
  if(a==nullptr)
    return GDT_FALSE;

  if(IsEmpty())
    InsertAfter(c,a,Type);
  else {
    // "Insert before c" is implemented by inserting the new cell AFTER
    // c, then swapping c and the new cell's contents -- c keeps its
    // identity/position for existing GPOSITION* holders, but now holds
    // the new data.
    InsertAfter(c,a,Type);
    GPOSITION *nc = Next(c);
    // BUGFIX #2: this used to swap only Atom (via Update(), which sets
    // Atom and nothing else) and leave Type untouched on both cells,
    // so after the swap `a`'s Type tag stayed on the OLD cell (still
    // holding the old Atom) while the OLD Atom ended up tagged with
    // the NEW Type -- confirmed with a standalone repro: InsertBefore
    // an atom with Type 99 next to one with Type 42 and the two ended
    // up with their Atom/Type pairings crossed. Swap Type along with
    // Atom so they stay paired correctly. See
    // docs/BUG_CATALOG.md#srcglisthxx.
    GATOM *ta = c->Atom;
    int tt = c->Type;
    c->Atom = nc->Atom;
    c->Type = nc->Type;
    nc->Atom = ta;
    nc->Type = tt;
  }
  return GDT_TRUE;

}

GDT_BOOLEAN GLIST::InsertBefore(GPOSITION *c, GATOM *a)
{
  return(InsertBefore(c,a,LIST_PTR));
}

GDT_BOOLEAN GLIST::Update(GPOSITION *c, GATOM *a)
{
  if(a==nullptr)
    return GDT_FALSE;

  c->Atom = a;

  return GDT_TRUE;
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
	 a points to an allocated structure.
   POST: 0 if out of memory condition.
	 1 on success.  a is inserted in list after c.
*/
GDT_BOOLEAN GLIST::InsertAfter(GPOSITION *c, GATOM *a, int Type)
{
  GPOSITION *nc, *tc;

  if(Head == nullptr) {
    // Inserting into empty list
    if((Head = new GPOSITION()) == nullptr)
      return GDT_FALSE;
    Head->Atom = a;
    Head->Type = Type;
    Head->Next = nullptr;
    Head->Prev = nullptr;
    Tail = Head;
  } else {
    // Inserting into non-empty list
    if((nc =  new GPOSITION()) == nullptr)
      return GDT_FALSE;
    nc->Atom = a;
    nc->Type = Type;
    tc = c->Next;
    c->Next = nc;
    nc->Prev = c;
    nc->Next = tc;
    if(c == Tail)
      // Inserted after tail, move the pointer
      Tail = nc;
  }
  Length=Length+1;

  return GDT_TRUE;
}

GDT_BOOLEAN GLIST::InsertAfter(GPOSITION *c, GATOM *a)
{
  return(InsertAfter(c,a,LIST_PTR));
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
   POST: Pointer to atom in c.
*/
GATOM* GLIST::Retrieve(GPOSITION *c)
{
  return c->Atom;
}

INT GLIST::GetLength()
{
  return Length;
}

/*
   PRE:	 l has been created.
	 c points to a cell in l.
	 Caller has freed memory allocated within the atom in c.
   POST: c is removed from l.
*/
void GLIST::Delete(GPOSITION *c)
{
  if(IsEmpty())
    return;

  if(Head == c) {
    if(Tail == c) {
      // singleton list
      Head = nullptr;
      Tail = nullptr;
    } else {
      // deleting first cell from non-singleton
      Head = c->Next;
      c->Next->Prev = nullptr;
    }
  } else {
    if(Tail == c) {
      // deleting last cell from non-singleton
      Tail = c->Prev;
      c->Prev->Next = nullptr;
    } else {
      // BUGFIX #1: this set `c->Next = c->Prev` -- overwriting a
      // pointer field on the cell about to be deleted, achieving
      // nothing -- instead of `c->Next->Prev = c->Prev`, which is what
      // actually needs updating to skip over `c`. The cell after `c`
      // was left with Prev still pointing at the just-freed `c`,
      // confirmed a real heap-use-after-free with a standalone repro
      // (delete an interior cell, then Prev()/Retrieve() the following
      // one). See docs/BUG_CATALOG.md#srcglisthxx.
      // deleting cell from interior
      c->Prev->Next = c->Next;
      c->Next->Prev = c->Prev;
    }
  }
  delete c;
  Length=Length - 1;

  return;
}

/*
   PRE:	 c is a valid cell.
   POST: Data type integer as defined in list.h
*/
INT GLIST::DataType(GPOSITION *c)
{
  return(c->Type);
}
