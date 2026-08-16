// $Id: attr.cxx,v 1.5 1998/05/12 16:48:54 cnidr Exp $
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
File:		attr.cxx
Version:	1.01
$Revision: 1.5 $
Description:	Class ATTR - Attribute
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "attr.hxx"

ATTR::ATTR() {
  SetId = "";
  AttrType = 0;
  AttrValue = "";
}


ATTR& 
ATTR::operator=(const ATTR& OtherAttr) {
  SetId = OtherAttr.SetId;
  AttrType = OtherAttr.AttrType;
  AttrValue = OtherAttr.AttrValue;
  return *this;
}


void 
ATTR::SetSetId(const STRING& NewSetId) {
  SetId = NewSetId;
}


void 
ATTR::GetSetId(PSTRING StringBuffer) const {
  *StringBuffer = SetId;
}


void 
ATTR::SetAttrType(const INT NewAttrType) {
  AttrType = NewAttrType;
}


INT 
ATTR::GetAttrType() const {
  return AttrType;
}


void 
ATTR::SetAttrValue(const STRING& NewAttrValue) {
  AttrValue = NewAttrValue;
}


void 
ATTR::GetAttrValue(PSTRING StringBuffer) const {
  *StringBuffer = AttrValue;
}


void 
ATTR::SetAttrValue(const INT NewAttrValue) {
  AttrValue = NewAttrValue;
}


INT 
ATTR::GetAttrValue() const {
  return AttrValue.GetInt();
}


ATTR::~ATTR() {
}
