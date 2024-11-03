// $Id: attrlist.hxx,v 1.5 1998/05/12 16:48:55 cnidr Exp $
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
File:		attrlist.hxx
Version:	1.00
$Revision: 1.5 $
Description:	Class ATTRLIST - Attribute List
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef ATTRLIST_HXX
#define ATTRLIST_HXX

#include "defs.hxx"
#include "attr.hxx"

class ATTRLIST {
public:
  ATTRLIST();
  void        Init();
  ATTRLIST&   operator=(const ATTRLIST& OtherAttrlist);
  void        AddEntry(const ATTR& AttrRecord);
  void        GetEntry(const INT Index, PATTR AttrRecord) const;
  void        SetEntry(const INT Index, const ATTR& AttrRecord);
  void        DeleteEntry(const INT Index);
  void        Expand();
  void        CleanUp();
  void        Resize(const INT Entries);
  INT         GetTotalEntries() const;
  INT         Lookup(const STRING& SetId, const INT AttrType) const;
  INT         Lookup(const STRING& SetId, const INT AttrType, 
		     const INT AttrValue) const;
  void        SetValue(const STRING& SetId, const INT AttrType, 
		       const STRING& AttrValue);
  void        SetValue(const STRING& SetId, const INT AttrType, 
		       const INT AttrValue);
  void        ClearAttr(const STRING& SetId, const INT AttrType, 
			const INT AttrValue);
  GDT_BOOLEAN GetValue(const STRING& SetId, const INT AttrType, 
		       PSTRING StringBuffer) const;
  GDT_BOOLEAN GetValue(const STRING& SetId, const INT AttrType, INT 
		       *IntBuffer) const;
  void        AttrSetFieldName(const STRING& FieldName);
  void        AttrSetFieldType(const STRING& FieldType);
  GDT_BOOLEAN AttrGetFieldName(PSTRING StringBuffer) const;
  GDT_BOOLEAN AttrGetFieldType(PSTRING StringBuffer) const;
  void        AttrSetRightTruncation(const GDT_BOOLEAN RightTruncation);
  GDT_BOOLEAN AttrGetRightTruncation() const;
  
  void        AttrSetRelation(const INT Relation);
  GDT_BOOLEAN AttrGetRelation(INT *IntBuffer) const;
  void        AttrSetStructure(const INT Structure);
  GDT_BOOLEAN AttrGetStructure(INT *IntBuffer) const;

  void        AttrSetTermWeight(const STRING& TermWeight);
  GDT_BOOLEAN AttrGetTermWeight(PSTRING StringBuffer) const;
  ~ATTRLIST();

private:
  PATTR Table;
  INT   TotalEntries;
  INT   MaxEntries;
};

typedef ATTRLIST* PATTRLIST;

#endif
