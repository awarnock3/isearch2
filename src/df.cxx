// $Id: df.cxx,v 1.4 1998/05/12 16:48:59 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1994.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*@@@
File:		df.cxx
Version:	1.00
$Revision: 1.4 $
Description:	Class DF - Data Field
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

/*
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "fc.hxx"
#include "fct.hxx"
*/
#include "df.hxx"


DF::DF() {
}


DF& 
DF::operator=(const DF& OtherDf) {
  FieldName = OtherDf.FieldName;
  Fct = OtherDf.Fct;
  return *this;
}


void 
DF::SetFieldName(const STRING& NewFieldName) {
  FieldName = NewFieldName;
  FieldName.UpperCase();
}


void 
DF::GetFieldName(STRING *StringBuffer) const {
  *StringBuffer = FieldName;
}


void 
DF::SetFct(const FCT& NewFct) {
  Fct = NewFct;
}


void 
DF::GetFct(FCT *FctBuffer) const {
  *FctBuffer = Fct;
}


void 
DF::Write(FILE *fp) const {
  FieldName.Print(fp);
  fprintf(fp, "\n");
  Fct.Write(fp);
}


void 
DF::Read(FILE *fp) {
  FieldName.FGet(fp, 256);
  Fct.Read(fp);
}


DF::~DF() {
}
