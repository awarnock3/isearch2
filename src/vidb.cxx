/* $Id: vidb.cxx,v 1.11 2001/02/28 11:44:39 cnidr Exp $ */
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
File:           vidb.cxx
Version:        $Revision: 1.11 $
Description:    Class VIDB, Virtual IDB
Author:         Kevin Gamiel, kgamiel@cnidr.org
		Nassib Nassar, nrn@cnidr.org
                Archie Warnock, warnock@awcubed.com
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <sys/stat.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include "vidb.hxx"
#include "common.hxx"
#include "dtreg.hxx"

//
//  Calls Initialize()
//
VIDB::VIDB(const STRING& NewPathName, const STRING& NewFileName, 
	const STRLIST& NewDocTypeOptions)
  //  : IDB(NewPathName, NewFileName, NewDocTypeOptions)
{
	Initialize(NewPathName, NewFileName, NewDocTypeOptions);
}


//
//  Calls Initialize()
//
VIDB::VIDB(const STRING& NewPathName, const STRING& NewFileName)
  //  : IDB(NewPathName, NewFileName)
{
	STRLIST EmptyList;

	//	Initialize(NewPathName, NewFileName, EmptyList, GDT_TRUE);
	Initialize(NewPathName, NewFileName, EmptyList);
}


//
// Attempts to locate a database file with extension ".vdb".  If that
// file does not exist, it attempts to open the database normally.  If
// it does exist, it opens that file and assumes there to be a list of
// database names separated by newline characters.  It loads each
// database listed in the ".vdb" file and subsequent search and
// present operations are performed on the entire list of databases.
//
void
VIDB::Initialize(const STRING& NewPathName, const STRING& NewFileName,
		 const STRLIST& NewDocTypeOptions)
{
  //
  // Build various filenames
  //
  STRING DbPathName, VirtualDbFn, DbFileName;
  STRING      RawFilenameList;
  STRING      DbName, DocType, TempDocType;
  struct stat status;
  INT4        DatabaseCount;
  STRLIST     FilenameList;
  INT4        i;
  STRINGINDEX j;
  STRING      ThisDBPathName, ThisDBFileName, PathName;
  INT         k=0;

  DbPathName = NewPathName;
  AddTrailingSlash(&DbPathName);
  DatabasePath = DbPathName;
  ExpandFileSpec(&DbPathName);
  DbFileName = NewFileName;
  RemovePath(&DbFileName);
  DatabaseName = DbFileName;
  VirtualDbFn = DbPathName;
  VirtualDbFn.Cat(DbFileName);
  VirtualDbFn.Cat(".vdb");

  //
  // Try to read the .vdb file if it exists
  //
  if (stat(VirtualDbFn,&status) == 0) {
    RawFilenameList.ReadFile(VirtualDbFn);
    DbisVirtual = GDT_TRUE;
  } else {
    RawFilenameList = DbFileName;
    DbisVirtual = GDT_FALSE;
  }

  if ((RawFilenameList.GetLength() > 0) && (!RawFilenameList.Equals(""))) {
    //
    // This is a Virtual database!
    //
    FilenameList.Split("\n", RawFilenameList);
    DatabaseCount = FilenameList.GetTotalEntries();

  } else {
    // Something is wrong because nothing was in the .vdb file
    perror(DatabaseName);
    EXIT_ERROR;
  }

  // Ultimately, we should look through the vdb file to count
       // comment lines so we can allocate the right number of elements 
       // in the list correctly.
  c_dblist = new IDB*[DatabaseCount];
  c_inconsistent_doctypes = GDT_FALSE;
  c_dbcount = 0;

  // Load each database by looping through the list of databases
  for(i=0;i < DatabaseCount; i++) {
    FilenameList.GetEntry(i+1, &DbName);

    // Handle comments correctly
    j = DbName.Search('#');
    if (j>0)
      DbName.EraseAfter(j-1);
    DbName.Trim();

    // If the line is not just a comment
    if(DbName.GetLength() != 0) {
      ThisDBPathName = DbName;
      ThisDBFileName = DbName;
      RemovePath(&ThisDBFileName);
      RemoveFileName(&ThisDBPathName);
      if (ThisDBPathName.GetLength() != 0)
	PathName = ThisDBPathName;
      else
	PathName = NewPathName;

      // Now create the database object
	   // use k to keep track of the actual number of databases
	   // and do not increment it if the line was just a comment
	   c_dblist[k] = new IDB(PathName, ThisDBFileName, NewDocTypeOptions);
      c_dbcount++;
      k++;
    }
  }
}


//
// Delete all databases
//
VIDB::~VIDB()
{
  INT4 i;
  for(i=0;i < c_dbcount; i++) {
    if(c_dblist[i])
      delete c_dblist[i];
  }
  delete [] c_dblist;
}


//
// Turn on debugging mode for all databases
//
void 
VIDB::DebugModeOn()
{
  INT4 i;
  for(i=0; i<c_dbcount; i++)
    c_dblist[i]->DebugModeOn();
}


//
// Check each database for compatibility problems
//
GDT_BOOLEAN 
VIDB::IsDbCompatible()
{
  
  //
  // See the Initialize() method for info on this
  //
  if(c_inconsistent_doctypes)
    return GDT_FALSE;

  INT4 i;
  for(i=0;i<c_dbcount;i++) {
    if(c_dblist[i]->IsDbCompatible() == GDT_FALSE)
      return GDT_FALSE;
  }
  return GDT_TRUE;
}


//
// Deprecated
//
GDT_BOOLEAN 
VIDB::IsDbVirtual(void) {
  return DbisVirtual;
}


IRSET*
VIDB::Search(const SQUERY& SearchQuery) 
{

  STRING GlobalDoctype;
  IRSET *RsetPtr, **RsetPtrs;
  DOCTYPE **DoctypePtrs;
  SQUERY Query;
  INT n_hits;
  MDT *pMDT;

  RsetPtr = nullptr;

  // Bail out if no databases
  if (c_dbcount <= 0)
    return RsetPtr;

  //  GetGlobalDocType(&GlobalDoctype);
  Query = SearchQuery;
  n_hits = 0;

  // Make a list of doctype pointers
  DoctypePtrs = new PDOCTYPE[c_dbcount];

  // Make a list of result set pointers
  RsetPtrs = new PIRSET[c_dbcount];

  // Loop over the databases
  for (INT i = 0; i < c_dbcount; i++) {
    c_dblist[i]->GetGlobalDocType(&GlobalDoctype);
    pMDT = c_dblist[i]->GetMainMdt();
    DoctypePtrs[i] = c_dblist[i]->DocTypeReg->GetDocTypePtr(GlobalDoctype);
    DoctypePtrs[i]->BeforeSearching(&Query);

    // Do the search
    RsetPtrs[i] = c_dblist[i]->MainIndex->Search(Query);

    // Stash the database number in each record
    RsetPtrs[i]->StoreDbNum(i);
    RsetPtrs[i]->SetMdt(*pMDT);

    // How many hits did we get?
    n_hits += RsetPtrs[i]->GetTotalEntries();

    RsetPtrs[i] = DoctypePtrs[i]->AfterSearching(RsetPtrs[i]);

    // Combine results
    if (i==0)
      RsetPtr = RsetPtrs[i];
    else
      RsetPtr->Concat(*(RsetPtrs[i]));
  }

  if (DoctypePtrs)
    delete [] DoctypePtrs;
  if (RsetPtrs)
    delete [] RsetPtrs;
  //  printf("Got %d hits\n",n_hits);
  return RsetPtr;
}


IRSET*
VIDB::AndSearch(const SQUERY& SearchQuery) 
{

  STRING GlobalDoctype;
  IRSET *RsetPtr, **RsetPtrs;
  DOCTYPE **DoctypePtrs;
  SQUERY Query;
  INT n_hits;

  RsetPtr = nullptr;

  // Bail out if no databases
  if (c_dbcount <= 0)
    return RsetPtr;

  //  GetGlobalDocType(&GlobalDoctype);
  Query = SearchQuery;
  n_hits = 0;

  // Make a list of doctype pointers
  DoctypePtrs = new PDOCTYPE[c_dbcount];

  // Make a list of result set pointers
  RsetPtrs = new PIRSET[c_dbcount];

  // Loop over the databases
  for (INT i = 0; i < c_dbcount; i++) {
    c_dblist[i]->GetGlobalDocType(&GlobalDoctype);
    DoctypePtrs[i] = c_dblist[i]->DocTypeReg->GetDocTypePtr(GlobalDoctype);
    DoctypePtrs[i]->BeforeSearching(&Query);

    // Do the search
    RsetPtrs[i] = c_dblist[i]->MainIndex->AndSearch(Query);

    // And stash the database number in each record
    RsetPtrs[i]->StoreDbNum(i);

    // How many hits did we get?
    n_hits += RsetPtrs[i]->GetTotalEntries();

    RsetPtrs[i] = DoctypePtrs[i]->AfterSearching(RsetPtrs[i]);

    // Combine results
    if (i==0)
      RsetPtr = RsetPtrs[i];
    else
      RsetPtr->Concat(*(RsetPtrs[i]));
  }

  if (DoctypePtrs)
    delete [] DoctypePtrs;
  if (RsetPtrs)
    delete [] RsetPtrs;
  //  printf("Got %d hits\n",n_hits);
  return RsetPtr;
}


void 
VIDB::BeginRsetPresent(const STRING& RecordSyntax)
{
  for(INT i=0;i<c_dbcount;i++) {
    c_dblist[i]->BeginRsetPresent(RecordSyntax);
  }
}


void
VIDB::Present(const RESULT& ResultRecord, const STRING& ElementSet,
	      const STRING& RecordSyntax, STRING *StringBuffer)
{
  // BUGFIX #2 (continued, see GetDbNameByNumber() above): same
  // missing-bounds-check shape -- ResultRecord.GetDbNum() defaults to
  // 0 (safe whenever c_dbcount > 0) and is otherwise only ever set
  // internally by VIDB's own Search()/AndSearch()/KeyLookup(), so this
  // wasn't confirmed externally reachable either, but it's the same
  // free fix. Leaves *StringBuffer untouched when out of range.
  INT i;
  i = ResultRecord.GetDbNum();
  if (i < 0 || i >= c_dbcount)
    return;
  c_dblist[i]->Present(ResultRecord, ElementSet, RecordSyntax,
			   StringBuffer);
}


void 
VIDB::EndRsetPresent(const STRING& RecordSyntax)
{
  for(INT i=0;i<c_dbcount;i++) {
    c_dblist[i]->EndRsetPresent(RecordSyntax);
  }
}


INT
VIDB::GetTotalRecords() const {
  INT Entries=0;
  for(INT i=0;i<c_dbcount;i++) {
    Entries += c_dblist[i]->MainMdt->GetTotalEntries();
  }
  return Entries;
}


void 
VIDB::GetGlobalDocType(STRING *StringBuffer) const {
  *StringBuffer = "VIRTUAL";
}


void
VIDB::KeyLookup(const STRING& Key, RESULT *ResultBuffer) const {
  //  c_dblist[0]->KeyLookup(Key,ResultBuffer);
  //  ResultBuffer->SetDbNum(0);
  STRINGINDEX n;
  STRING ThisDB,ThisKey;
  INT i;

  n = Key.Search(':');
  if (n > 0) {
    // The key contains a DB number of the form DBnum:Key
    ThisDB = Key;
    ThisDB.EraseAfter(n-1);
    i = ThisDB.GetInt();

    ThisKey = Key;
    ThisKey.EraseBefore(n+1);

    // BUGFIX #1: this indexed c_dblist[i] with i parsed directly out
    // of Key -- caller-supplied, external input at every real call
    // site found in the tree (src/Iget.cxx, src/zpresent.cxx both pass
    // a raw record key straight from their command-line/protocol
    // arguments through to VIDB::KeyLookup()) -- with no bounds check.
    // A key like "999:somekey" against a VIDB with fewer than 1000
    // sub-databases read c_dblist[999] out of bounds and called
    // KeyLookup() through whatever garbage pointer was there. See
    // docs/BUG_CATALOG.md#srcvidbhxx.
    if (i < 0 || i >= c_dbcount)
      return;

    c_dblist[i]->KeyLookup(ThisKey,ResultBuffer);
    ResultBuffer->SetDbNum(i);

  } else {
    // BUGFIX #1 (continued): c_dblist[0] is only ever actually
    // populated if at least one non-comment line was found in the
    // .vdb file (see Initialize()) -- a .vdb file that's non-empty but
    // entirely comments leaves c_dbcount at 0 with c_dblist[0] never
    // written to, so this used to dereference an uninitialized pointer
    // for every plain (no "DBnum:" prefix) key in that case.
    if (c_dbcount <= 0)
      return;
    c_dblist[0]->KeyLookup(Key,ResultBuffer);
    ResultBuffer->SetDbNum(0);
  }
}


void
VIDB::GetDbNameByNumber(INT DbNumber, STRING *DbName) {
  // From Rami Heinisuo <rami@eduix.com>
  // possible because IDB and VIDB are friends
  // BUGFIX #2: same missing-bounds-check shape as KeyLookup() above --
  // DbNumber is caller-supplied with nothing stopping an out-of-range
  // value from indexing c_dblist out of bounds. Current callers
  // (Isearch-cgi/isrch_srch.cxx) only ever pass back a DbNum a search
  // result was already stamped with internally, so this wasn't
  // confirmed externally reachable the way KeyLookup()'s was, but the
  // method is public API and the fix is free. Leaves *DbName untouched
  // (its caller-supplied default) when out of range. See
  // docs/BUG_CATALOG.md#srcvidbhxx.
  if (DbNumber < 0 || DbNumber >= c_dbcount)
    return;
  *DbName = c_dblist[DbNumber]->DbFileName;
}


void
VIDB::GetDfdt(DFDT *DfdtBuffer) const
{
  // BUGFIX #1 (continued, see KeyLookup() above): c_dblist[0] is only
  // populated when c_dbcount > 0.
  if (c_dbcount <= 0)
    return;
  c_dblist[0]->GetDfdt(DfdtBuffer);
}

void
VIDB::GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer) const
{
  // BUGFIX #1 (continued): same as GetDfdt() just above.
  if (c_dbcount <= 0)
    return;
  c_dblist[0]->GetRecordDfdt(Key,DfdtBuffer);
}

