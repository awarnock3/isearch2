/* $Id: strlist.hxx,v 1.7 1999/04/17 03:23:28 cnidr Exp $ */
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
File:		strlist.hxx
Version:	1.1
$Revision: 1.7 $
Description:	Class STRLIST - String List
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef STRLIST_HXX
#define STRLIST_HXX

#include <stdio.h>
#include <string.h>

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"

// An ordered list of STRING entries, one per node: each STRLIST object
// is itself a VLIST node holding a single entry's String, and the
// list's identity is the head node's `this` (see VLIST for the
// circular-list mechanics AddEntry/GetEntry/etc. build on).
//
// NOTE: STRLIST has no explicit copy constructor, only operator=
// (below). VLIST's own copy constructor is compiler-generated and
// shallow-copies its Next/Prev pointers instead of splicing into the
// circle -- a known, tracked defect (see
// docs/AUTOPILOT_LOG.md#srcvlisthxx, blocked pending a header-signature
// decision on vlist.hxx). STRLIST inherits that same latent risk for
// copy-CONSTRUCTION (as opposed to assignment, which this file's
// operator= handles correctly); every call site in this tree already
// avoids it by default-constructing and then assigning, as Split()
// does below. Fixing it requires adding a declaration to vlist.hxx, so
// it's deferred there rather than duplicated as a second blocked row
// here.
class STRLIST
  : public VLIST
{

public:
  STRLIST();
  // Deep-copies OtherStrlist's entries via new nodes (never copies
  // Next/Prev pointers directly) -- safe even for self-assignment.
  STRLIST& operator=(const STRLIST& OtherStrlist);
  void AddEntry(const STRING& StringEntry);
  void AddEntry(const CHR* Entry);
  // 1-based. Beyond the current entry count, appends empty filler
  // entries up to Index. A no-op for Index 0.
  void SetEntry(const SIZE_T Index, const STRING& StringEntry);
  // 1-based. Leaves *StringEntry unchanged for an out-of-range Index.
  void GetEntry(const SIZE_T Index, STRING* StringEntry) const;
  // Splits TheString on every occurrence of Separator into entries,
  // replacing this list's current contents. A trailing empty segment
  // (Separator at the very end) is dropped, not added as an entry.
  void Split(const CHR* Separator, const STRING& TheString);
  void Split(const CHR Separator, const STRING& TheString);
  // Concatenates all entries, interleaving Separator between them.
  void Join(const CHR* Separator, STRING* StringBuffer);
  // Case-insensitive linear search; returns the 1-based index of the
  // first match, or 0 if none.
  SIZE_T SearchCase(const STRING& SearchTerm);
  // Treats each entry as a "Title = value" pair (whitespace around
  // Title is trimmed before the case-insensitive compare) and returns
  // the value for the first matching Title via *StringBuffer, or ""
  // if none match.
  void GetValue(const CHR* Title, STRING* StringBuffer);
  void GetValue(const STRING& Title, STRING* StringBuffer);
  // Writes each entry to fp, one per line.
  void Dump(const PFILE fp) ;
//	friend ostream & operator<<(ostream& os, const STRLIST& str);

private:
  STRING String;
};

typedef STRLIST* PSTRLIST;

#endif
