// $Id: dfdt.hxx,v 1.4 1998/05/12 16:49:01 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1995. 

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
File:		dfdt.hxx
Version:	1.00
$Revision: 1.4 $
Description:	Class DFDT - Data Field Definitions Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef DFDT_HXX
#define DFDT_HXX

#include <stdlib.h> //for exit()
#include <string.h>

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "vlist.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dfd.hxx"
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// Data Field Definitions Table: an ordered, resizable array of DFD
// entries, one per indexed field, tracking each field's assigned file
// number and attributes.
class DFDT {
public:
  DFDT();
  // BUGFIX #1: DFDT owned a heap-allocated Table array but declared no
  // copy constructor, so the compiler-generated one did a shallow
  // pointer copy -- confirmed to double-free Table under ASan.
  // Deep-copies Table (sized to the source's MaxEntries), TotalEntries,
  // MaxEntries, and Changed, mirroring operator='s own logic. See
  // docs/BUG_CATALOG.md.
  DFDT(const DFDT& OtherDfdt);
  DFDT& operator=(const DFDT& OtherDfdt);
  void  Initialize();
  void  LoadTable(const STRING& FileName);
  void  SaveTable(const STRING& FileName);
  void  AddEntry(const DFD& DfdRecord);
  void  FastAddEntry(const DFD& DfdRecord);
  void  GetEntry(const INT Index, PDFD DfdRecord) const;
  // Leaves *DfdRecord untouched if FieldName isn't found -- same
  // convention as GetEntry() above (see BUGFIX #3 in source: this used
  // to try to signal "not found" by nulling the DfdRecord parameter
  // itself, which is a by-value pointer and has no effect the caller
  // can observe).
  void  GetDfdRecord(const STRING& FieldName, PDFD DfdRecord) const;
  INT   GetNewFileNumber() const;
  void  Expand();
  void  CleanUp();
  void  Resize(const INT Entries);
  INT   GetTotalEntries() const;
  INT   GetChanged() const;
  ~DFDT();

private:
  PDFD Table;
  INT  TotalEntries;
  INT  MaxEntries;
  INT  Changed;
};

typedef DFDT* PDFDT;

#endif
