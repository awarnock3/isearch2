// $Id: sgmltag.hxx,v 1.5 2000/02/04 22:50:50 cnidr Exp $
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
File:		sgmltag.hxx
Version:	1.03
$Revision: 1.5 $
Description:	Class SGMLTAG - SGML-like Document Type
Author:		Kevin Gamiel, Kevin.Gamiel@cnidr.org
Changes:	See sgmltag.cxx
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef SGMLTAG_HXX
#define SGMLTAG_HXX

//#include <sys/time.h>
#include <time.h>
#include "defs.hxx"
#include "doctype.hxx"

// A stricter SGML-like DOCTYPE than SGMLNORM: only tag pairs whose open
// and close tags are exactly the same text (e.g. <title>...</title>,
// excluding the closing slash) become fields -- see the "What"/"Pre"/
// "Post" comments above ParseFields()/sgml_parse_tags() in sgmltag.cxx
// for the precise contract. Independent implementation of DOCTYPE, not
// a subclass of SGMLNORM.
class SGMLTAG
  : public DOCTYPE {

public:
  SGMLTAG(PIDBOBJ DbParent);
  ~SGMLTAG();
  void ParseFields(PRECORD NewRecord);
  // Scans t (nullptr-terminated) starting at *t for a "/"+tag closing
  // tag matching tag's full text, case-insensitively. Returns a
  // pointer to the matching entry, or nullptr if none/t is empty.
  virtual char *find_end_tag(char **t, char *tag);
  // Splits b (len bytes) in place into a nullptr-terminated array of
  // pointers to each tag's contents (each tag's '>' is overwritten
  // with '\0' to terminate it); *numtags counts how many of those tags
  // DOCTYPE::UsefulSearchField() considers worth indexing (informational
  // only -- ParseFields() doesn't currently act on it). Returns nullptr
  // on allocation failure. Caller owns the returned array (delete[] it;
  // the char* elements point into b, not separately allocated).
  virtual char **sgml_parse_tags(char *b, int len, int *numtags);
};

typedef SGMLTAG* PSGMLTAG;

#endif
