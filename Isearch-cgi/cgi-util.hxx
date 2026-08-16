// $Id: cgi-util.hxx,v 1.3 1998/05/12 16:48:06 cnidr Exp $
/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
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
File:          	cgi-util.hxx
Version:        1.04
$Revision: 1.3 $
Description:    CGI utilities
Authors:        Kevin Gamiel, kgamiel@cnidr.org
		Tim Gemma, stone@k12.cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef _CGIUTIL_HXX
#define _CGIUTIL_HXX

#include "gdt.h"
#include "string.hxx"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define CGI_MAXENTRIES 100
#define POST 0
#define GET 1

// Parses CGI form input (GET's QUERY_STRING or POST's stdin body,
// picked via REQUEST_METHOD) into up to CGI_MAXENTRIES name/value
// pairs at construction time, decoding %XX escapes and '+'-as-space
// along the way. See docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx for
// three confirmed, externally-reachable crashes fixed in GetInput()
// this turn (a client fully controls REQUEST_METHOD, CONTENT_LENGTH,
// QUERY_STRING, and the POST body).
class CGIAPP {
  PCHR name[CGI_MAXENTRIES];
  PCHR value[CGI_MAXENTRIES];
  INT entry_count;
  INT Method;
  void GetInput();

public:
  CGIAPP();
  void Display();
  // No bounds checking against the actual entry count -- there's no
  // public accessor for it either, so these are only really safe to
  // call from a loop this class itself controls (see Display()).
  // GetValueByName() below is the safe, bounds-checked way to look up
  // a specific field.
  PCHR GetName(INT4 i);
  PCHR GetValue(INT4 i);
  PCHR GetValueByName(const CHR *name);
  ~CGIAPP();

};

void plustospace(PCHR p);
void unescape_url(PCHR p);
// BUGFIX note (docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx): out has no
// size parameter, so its caller must independently know this can write
// up to 3x strlen(url) bytes (every non-alnum, non-space input byte
// becomes a 3-byte %XX escape) -- easy to get wrong, and fixing it
// needs a signature change this turn left alone since there are
// currently no callers anywhere in the tree to get it wrong yet.
void escape_url(PCHR url, PCHR out);
void spacetoplus(PCHR str);
CHR x2c(PCHR p);

// Path parameter extraction
STRING ExtractPathParam(const CHR *pathInfo, INT paramIndex);

#endif
