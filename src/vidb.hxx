/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994-1998.

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
File:		vidb.hxx
Version:	$Revision: 1.7 $
Description:	Class VIDB
Author:		Kevin Gamiel, kgamiel@cnidr.org
		Nassib Nassar, nrn@cnidr.org, 
		Glenn MacStravic, gem@cnidr.org
                Archie Warnock, warnock@awcubed.com
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef VIDB_HXX
#define VIDB_HXX

#include <time.h>
#include "isearch.hxx"
#include "idb.hxx"
#include "gdt.h"
#include "opobj.hxx"
#include "operator.hxx"
#include "dtreg.hxx"

// A "virtual database": a read-mostly view over a list of real IDB
// databases (c_dblist), loaded from a "<dbname>.vdb" file listing one
// sub-database path per line (blank/'#'-comment lines skipped) --
// falls back to treating NewFileName as a single ordinary IDB if no
// .vdb file exists. Search()/AndSearch() fan out to every
// sub-database and concatenate results, tagging each with its source
// database's index (RESULT::SetDbNum()) so Present()/KeyLookup() can
// route back to the right one later. Not related to IDB by
// inheritance (this is composition, not polymorphism) despite the
// mutual `friend` declarations.
class VIDB
{
  friend class IDB;

public:
  VIDB(const STRING& NewPathName, const STRING& NewFileName,
       const STRLIST& NewDocTypeOptions);
  VIDB(const STRING& NewPathName, const STRING& NewFileName);
  void Initialize(const STRING& NewPathName, const STRING& NewFileName, 
		  const STRLIST& NewDocTypeOptions);

  ~VIDB();
  void        DebugModeOn();
  GDT_BOOLEAN IsDbCompatible();
  GDT_BOOLEAN IsDbVirtual(void);
  IRSET*      Search(const SQUERY& SearchQuery);
  IRSET*      AndSearch(const SQUERY& SearchQuery);
  void        BeginRsetPresent(const STRING& RecordSyntax);
  void        Present(const RESULT& ResultRecord, const STRING& ElementSet, 
		      const STRING& RecordSyntax, STRING* StringBuffer);
  void        EndRsetPresent(const STRING& RecordSyntax);

  // Needed by SAPI
  INT         GetTotalRecords() const;
  void        GetGlobalDocType(STRING *StringBuffer) const;
  void        GetDfdt(DFDT *DfdtBuffer) const;
  void        GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer) const;
  void        KeyLookup(const STRING& Key, RESULT *ResultBuffer) const;
  void        GetDbNameByNumber(INT DbNumber, STRING *DbName);
private:
  GDT_BOOLEAN DbisVirtual;

  STRING DatabaseName;
  STRING DatabasePath;
  IDB		**c_dblist;
  INT4		c_dbcount;
  GDT_BOOLEAN	c_inconsistent_doctypes;

};

typedef VIDB* PVIDB;

#endif
