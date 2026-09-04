// $Id: attr.hxx,v 1.4 1998/05/12 16:48:54 cnidr Exp $
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
File:		attr.hxx
Version:	1.00
$Revision: 1.4 $
Description:	Class ATTR - Attribute
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef ATTR_HXX
#define ATTR_HXX

#include "defs.hxx"
#include "string.hxx"

// A single Z39.50/GILS-style search attribute: an attribute-set id, an
// attribute type (Use/Relation/Position/Structure/Truncation/
// Completeness, per the protocol), and its value -- stored as STRING
// regardless of which SetAttrValue overload set it, so GetAttrValue()
// (INT) parses it back out via STRING::GetInt().
class ATTR {
public:
  ATTR();
  ATTR& operator=(const ATTR& OtherAttr);
  void  SetSetId(const STRING& NewSetId);
  void  GetSetId(PSTRING StringBuffer) const;
  void  SetAttrType(const INT NewAttrType);
  INT   GetAttrType() const;
  void  SetAttrValue(const STRING& NewAttrValue);
  void  GetAttrValue(PSTRING StringBuffer) const;
  void  SetAttrValue(const INT NewAttrValue);
  // Parses the stored value as an integer (0 if it isn't one).
  INT   GetAttrValue() const;
  ~ATTR();
private:
  STRING SetId;
  INT    AttrType;
  STRING AttrValue;
};

typedef ATTR* PATTR;

#endif
