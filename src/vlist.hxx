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
File:		vlist.hxx
Version:	1.00
Description:	Class VLIST - Doubly Linked Circular List Base Class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef VLIST_HXX
#define VLIST_HXX

#include "gdt.h"
#include "defs.hxx"
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// Doubly linked *circular* list base class: every node's Next/Prev
// eventually loops back to itself. Carries no payload of its own --
// subclasses (e.g. FCT, STRLIST) add their own data per node.
class VLIST {
public:
	VLIST();
	// BUGFIX #1: no copy constructor and no working operator= existed
	// (the commented-out sketch below was pure-virtual and never
	// compiled). A copied node becomes the sole member of its own new
	// one-node circle -- VLIST carries no other state, so this is
	// identical to default-construction. The source's own Next/Prev
	// links are deliberately never read: splicing a node into someone
	// else's circle needs AddNode()'s bookkeeping, not a blind pointer
	// copy. Not virtual, unlike the old sketch -- nothing in this tree
	// assigns/copy-constructs through a VLIST& or VLIST* today; each
	// subclass that needs to duplicate a circle's *contents* already
	// does so at its own level (see e.g. FCT::operator=, which walks
	// the source circle and builds fresh nodes via AddNode()).
	VLIST(const VLIST& OtherVlist);
	VLIST& operator=(const VLIST& OtherVlist);
  //	virtual VLIST& operator=(const VLIST& OtherVlist) = 0;
	virtual void   Clear();
  //	virtual void   EraseAfter(const SIZE_T Index);
	virtual void   EraseAfter(const INT Index);
	virtual void   Reverse();
  //	virtual SIZE_T GetTotalEntries() const;
	virtual INT    GetTotalEntries() const;
	virtual ~VLIST();
protected:
	virtual void   AddNode(VLIST* NewEntryPtr);
  //	virtual VLIST* GetNodePtr(const SIZE_T Index) const;
	virtual VLIST* GetNodePtr(const INT Index) const;
	virtual VLIST* GetNextNodePtr() const;
private:
	VLIST* Next;
	VLIST* Prev;
};

#endif
