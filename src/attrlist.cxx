// $Id: attrlist.cxx,v 1.6 2000/02/04 23:12:46 cnidr Exp $
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
File:		attrlist.cxx
Version:	1.01
$Revision#
Description:	Class ATTRLIST - Attribute List
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "attrlist.hxx"


ATTRLIST::ATTRLIST() {
  Init();
}


// BUGFIX #1: see docs/BUG_CATALOG.md. Deep-copies Table instead of
// sharing the source's pointer.
ATTRLIST::ATTRLIST(const ATTRLIST& OtherAttrlist) {
  INT x;
  Table = new ATTR[OtherAttrlist.MaxEntries];
  for (x=0; x<OtherAttrlist.TotalEntries; x++) {
    Table[x] = OtherAttrlist.Table[x];
  }
  TotalEntries = OtherAttrlist.TotalEntries;
  MaxEntries = OtherAttrlist.MaxEntries;
}


// BUGFIX #3: Table was allocated as `new ATTR[7]` while MaxEntries was
// set to 8, so AddEntry's `TotalEntries == MaxEntries` bounds check
// let the 8th entry write to Table[7] -- one past the end of a 7-slot
// array, a real heap buffer overflow on every ATTRLIST that ever grows
// past 7 entries. Computing the allocation size from MaxEntries itself
// makes the two impossible to drift apart again. See docs/BUG_CATALOG.md.
void
ATTRLIST::Init() {
  MaxEntries   = 8;
  Table        = new ATTR[MaxEntries];
  TotalEntries = 0;
}


ATTRLIST&
ATTRLIST::operator=(const ATTRLIST& OtherAttrlist) {
  // BUGFIX #2: no self-assignment guard -- `delete [] Table; Init();`
  // ran before OtherAttrlist.GetTotalEntries() was read, so `x = x;`
  // saw an already-emptied list and copied nothing back, silently
  // wiping it. See docs/BUG_CATALOG.md.
  if (this == &OtherAttrlist) {
    return *this;
  }
  if (Table) {
    delete [] Table;
  }
  Init();
  INT y = OtherAttrlist.GetTotalEntries();
  INT x;
  ATTR attr;
  for (x=1; x<=y; x++) {
    OtherAttrlist.GetEntry(x, &attr);
    AddEntry(attr);
  }
  return *this;
}


void 
ATTRLIST::AddEntry(const ATTR& AttrRecord) {
  if (TotalEntries == MaxEntries)
    Expand();
  Table[TotalEntries] = AttrRecord;
  TotalEntries = TotalEntries + 1;
}


void 
ATTRLIST::GetEntry(const INT Index, PATTR AttrRecord) const {
  if ( (Index > 0) && (Index <= TotalEntries) ) {
    *AttrRecord = Table[Index-1];
  }
}


void 
ATTRLIST::SetEntry(const INT Index, const ATTR& AttrRecord) {
  if ( (Index > 0) && (Index <= TotalEntries) ) {
    Table[Index-1] = AttrRecord;
  }
}


void 
ATTRLIST::DeleteEntry(const INT Index) {
  if ( (Index > 0) && (Index <= TotalEntries) ) {
    INT x;
    for (x=(Index-1); x<(TotalEntries-1); x++) {
      Table[x] = Table[x+1];
    }
    TotalEntries--;
  }
}


void 
ATTRLIST::Expand() {
  Resize(TotalEntries+10);
}


void 
ATTRLIST::CleanUp() {
  Resize(TotalEntries);
}


void 
ATTRLIST::Resize(const INT Entries) {
  PATTR Temp = new ATTR[Entries];
  INT RecsToCopy;
  INT x;
  if (Entries >= TotalEntries) {
    RecsToCopy = TotalEntries;
  } else {
    RecsToCopy = Entries;
    TotalEntries = Entries;
  }
  for (x=0; x<RecsToCopy; x++) {
    Temp[x] = Table[x];
  }
  if (Table)
    delete [] Table;
  Table = Temp;
  MaxEntries = Entries;
}


INT
ATTRLIST::GetTotalEntries() const {
  return TotalEntries;
}


INT 
ATTRLIST::Lookup(const STRING& SetId, const INT AttrType) const {
  INT x;
  STRING S;
  for (x=0; x<TotalEntries; x++) {
    Table[x].GetSetId(&S);
    if ( (SetId.Equals(S)) && (AttrType == Table[x].GetAttrType()) ) {
      return (x+1);
    }
  }
  return 0;
}


INT 
ATTRLIST::Lookup(const STRING& SetId, const INT AttrType, 
		 const INT AttrValue) const {
  INT x;
  STRING S;
  for (x=0; x<TotalEntries; x++) {
    Table[x].GetSetId(&S);
    if (S.Equals(IsearchAttributeSet)) {
      // If the database attribute set is the default Isearch one, just
      // match it to whatever the user asked for and go on
      S = SetId;
    }
    if ((SetId.Equals(S)) && (AttrType == Table[x].GetAttrType()) &&
	(AttrValue == Table[x].GetAttrValue()) ) {
      return (x+1);
    }
  }
  return 0;
}


void 
ATTRLIST::SetValue(const STRING& SetId, const INT AttrType, 
		   const STRING& AttrValue) {
  ATTR Attr;
  INT y = Lookup(SetId, AttrType);
  if (y) {
    GetEntry(y, &Attr);
    Attr.SetAttrValue(AttrValue);
    SetEntry(y, Attr);
  } else {
    Attr.SetSetId(SetId);
    Attr.SetAttrType(AttrType);
    Attr.SetAttrValue(AttrValue);
    AddEntry(Attr);
  }
}


void 
ATTRLIST::SetValue(const STRING& SetId, const INT AttrType, 
		   const INT AttrValue) {
  STRING S;
  S = AttrValue;
  SetValue(SetId, AttrType, S);
}


void 
ATTRLIST::ClearAttr(const STRING& SetId, const INT AttrType, 
		    const INT AttrValue) {
  INT y = Lookup(SetId, AttrType, AttrValue);
  DeleteEntry(y);
}


GDT_BOOLEAN 
ATTRLIST::GetValue(const STRING& SetId, const INT AttrType, 
		   STRING *StringBuffer) const {
  ATTR Attr;
  INT y = Lookup(SetId, AttrType);
  
  if (y) {
    
    GetEntry(y, &Attr);
    Attr.GetAttrValue(StringBuffer);
    
    return GDT_TRUE;
  } else {
    *StringBuffer = "";
    return GDT_FALSE;
  }
}


GDT_BOOLEAN 
ATTRLIST::GetValue(const STRING& SetId, const INT AttrType, 
		   INT *IntBuffer) const {
  GDT_BOOLEAN b;
  STRING S;
  b = GetValue(SetId, AttrType, &S);
  *IntBuffer = S.GetInt();
  return b;
}


void 
ATTRLIST::AttrSetFieldName(const STRING& FieldName) {
  STRING Field = FieldName;
  Field.UpperCase();
  SetValue(IsearchAttributeSet, IsearchFieldAttr, Field);
}


GDT_BOOLEAN 
ATTRLIST::AttrGetFieldName(STRING *StringBuffer) const {
  STRING t;
  t=IsearchAttributeSet;
  return GetValue(t, IsearchFieldAttr, StringBuffer);
}


void 
ATTRLIST::AttrSetFieldType(const STRING& FieldType) {
  STRING Type = FieldType;
  Type.UpperCase();
  SetValue(IsearchAttributeSet, IsearchTypeAttr, Type);
}


GDT_BOOLEAN 
ATTRLIST::AttrGetFieldType(STRING *StringBuffer) const {
  STRING t;
  t=IsearchAttributeSet;
  return GetValue(t, IsearchTypeAttr, StringBuffer);
}


void 
ATTRLIST::AttrSetRightTruncation(const GDT_BOOLEAN RightTruncation) {
  if (RightTruncation) {
    SetValue(Bib1AttributeSet, ZdistTruncationAttr, 1);
  } else {
    ClearAttr(Bib1AttributeSet, ZdistTruncationAttr, 1);
  }
}


GDT_BOOLEAN 
ATTRLIST::AttrGetRightTruncation() const {
  INT x = Lookup(Bib1AttributeSet, ZdistTruncationAttr, 1);
  if (x) {
    return GDT_TRUE;
  } else {
    return GDT_FALSE;
  }
}

void 
ATTRLIST::AttrSetRelation(const INT Relation) {
  STRING Value;
  Value = Relation;
  SetValue(IsearchAttributeSet, ZdistRelationAttr, Value);
}


GDT_BOOLEAN 
ATTRLIST::AttrGetRelation(INT *IntBuffer) const {
  GDT_BOOLEAN b;
  STRING S;
  b = GetValue(IsearchAttributeSet, ZdistRelationAttr, &S);
  *IntBuffer = S.GetInt();
  return b;
}


void 
ATTRLIST::AttrSetStructure(const INT Structure) {
  STRING Value;
  Value = Structure;
  SetValue(IsearchAttributeSet, ZdistStructureAttr, Value);
}


GDT_BOOLEAN 
ATTRLIST::AttrGetStructure(INT *IntBuffer) const {
  GDT_BOOLEAN b;
  STRING S;
  b = GetValue(IsearchAttributeSet, ZdistStructureAttr, &S);
  *IntBuffer = S.GetInt();
  return b;
}


void 
ATTRLIST::AttrSetTermWeight(const STRING& TermWeight) {
  SetValue(IsearchAttributeSet, IsearchWeightAttr, TermWeight);
}


GDT_BOOLEAN 
ATTRLIST::AttrGetTermWeight(STRING *StringBuffer) const {
  return GetValue(IsearchAttributeSet, IsearchWeightAttr, StringBuffer);
}


ATTRLIST::~ATTRLIST() {
  if (Table)
    delete [] Table;
}
