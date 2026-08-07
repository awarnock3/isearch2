// $Id: df.hxx,v 1.4 1998/05/12 16:48:59 cnidr Exp $
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
File:		df.hxx
Version:	1.00
$Revision: 1.4 $
Description:	Class DF - Data Field
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef DF_HXX
#define DF_HXX

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "fc.hxx"
#include "fct.hxx"

// A Data Field definition: a field name (stored uppercased -- see
// SetFieldName) paired with the FCT (Field Coordinate Table) of
// [start,end) byte-offset occurrences within a document that belong to
// that field.
class DF {
public:
  DF();
  // Deep-copies OtherDf's FieldName and Fct; safe under self-assignment.
  DF& operator=(const DF& OtherDf);
  // Stores NewFieldName uppercased.
  void SetFieldName(const STRING& NewFieldName);
  void GetFieldName(STRING *StringBuffer) const;
  void SetFct(const FCT& NewFct);
  void GetFct(FCT *FctBuffer) const;
  // Serializes as text: field name, a newline, then FCT::Write's own
  // format.
  void Write(FILE *fp) const;
  // Reads the pair back from text previously written by Write().
  void Read(FILE *fp);
  ~DF();

private:
  STRING FieldName;
  FCT    Fct;
};

typedef DF* PDF;

#endif
