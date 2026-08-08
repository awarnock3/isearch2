/* $Id: iknowdoc.hxx,v 1.2 1998/05/12 16:48:33 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Center for Networked Information Discovery and
Retrieval, 1996. 

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

3. The names of MCNC and Center for Networked Information Discovery and
Retrieval may not be used in any advertising or publicity relating to the
software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:           iknowdoc.hxx
Version:        1.0
$Revision: 1.2 $
Description:    Class IKNOWDOC - Colon Tagged Document Type for use with Iknow
Author:         Tim Gemma, stone@cnidr.org
@@@-*/

#ifndef IKNOWDOC_HXX
#define IKNOWDOC_HXX

#ifndef ISEARCH_HXX
#include "isearch.hxx"
#endif

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

#include "colondoc.hxx"

// An "IKNOW"-flavored colon-tagged DOCTYPE: like COLONDOC, but every
// record must open with a "Template:" field (naming the record's
// template type) followed immediately by a "Handle:" field, and every
// tagged value is additionally duplicated into a catch-all
// "Value-only" field (so a search can match any field's value without
// naming the field). The set of distinct template types seen across
// every record processed by this object is accumulated in
// TemplateTypes and written out to a "<db>.tpt" sidecar file when the
// object is destroyed.
class IKNOWDOC :  public COLONDOC {
public:
        IKNOWDOC(PIDBOBJ DbParent);
        void ParseFields(PRECORD NewRecord);
        STRLIST TemplateTypes;
        ~IKNOWDOC();
};
typedef IKNOWDOC* PIKNOWDOC;

#endif
