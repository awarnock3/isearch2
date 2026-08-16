/* $Id: isrch_fetch.cxx,v 1.8 2000/02/04 22:52:25 cnidr Exp $ */
/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1996, 1997, 1998.

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
File:          	isrch_fetch.cxx
Version:        1.02
$Revision: 1.8 $
Description:    CGI app that searches against Iindex-ed databases 
Author:         Kevin Gamiel, kgamiel@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <locale.h>

#include "gdt.h"
#include "isearch.hxx"

#include "common.hxx"
#include "infix2rpn.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
//#include "tokengen.hxx"
//#include "infix2rpn.hxx"
#include "config.hxx"

int main(int argc, char **argv)
{
  STRING DBPathName, DBRootName, Record;
  IDB *pdb;
  RESULT RsRecord;
  STRING RecordKey, ESet;
  STRING pathDb;

  if (!setlocale(LC_CTYPE,"")) {
    cout << "Warning: Failed to set the locale!" << endl;
  }

  cout << "Content-type: text/html\n\n";
  
  cout << "<a href=\"http://www.cnidr.org/\"><i>CNIDR</a> Isearch-cgi ";
  cout << IsearchVersion << "</i> ";

  // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiisrch_fetchcxx): this
  // checked argc < 4, guaranteeing only argv[0..3] (dbpath, dbname,
  // key), but the code below unconditionally reads argv[4] (es) too --
  // when argc == 4 exactly (the usage message's 4 parameters minus the
  // omitted <es>), argv[4] is the C++-standard-guaranteed nullptr
  // sentinel, dereferenced via STRING::operator=(const CHR*)'s
  // strlen() call. Confirmed with the real binary: `isrch_fetch dbpath
  // dbname key` (3 real args) segfaulted before this fix. Fixed by
  // requiring argc >= 5, matching the 4 real parameters the usage
  // message documents.
  if(argc < 5) {
    cout << "isrch_fetch " << IsearchVersion << endl;
    cout << "Copyright (c) 1995-2000 MCNC/CNIDR and A/WWW Enterprises" << endl;
    cout << "<p>Usage:  isrch_fetch &lt;dbpath&gt; &lt;dbname&gt; &lt;key&gt; &lt;es&gt;\n";
    exit(0);
  }

  STRING File;
  DBPathName = argv[1];
  DBRootName = argv[2];
  RecordKey = argv[3];
  ESet = argv[4];

  // Open database
  pdb = new IDB(DBPathName, DBRootName);
  
  // Is the database valid?
  if(pdb->GetTotalRecords() <= 0) {
    cout << "<p>Database " << DBRootName;
    cout << " does not exist or is corrupted\n";
    return -1;
  }

  pdb->KeyLookup(RecordKey, &RsRecord);
  RsRecord.GetFileName(&File);

  cout << "<i>(File: " << File << ")</i><p>" << endl;

  PCHR name;
  name=File.NewCString();
  // BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgiisrch_fetchcxx): both
  // copies of this check computed name+strlen(name)-5 (and -4)
  // unconditionally -- for a filename shorter than 5 (or 4) characters
  // (e.g. File ending up empty when RecordKey doesn't match any real
  // record, since KeyLookup()'s result is never checked), this
  // underflows the pointer before the start of the name buffer, technically
  // undefined behavior even though it didn't reproduce as an observable
  // crash under ASan/UBSan (including -fsanitize=pointer-overflow) in
  // testing -- pointer arithmetic that's never dereferenced doesn't
  // trip those checks. Computed once and length-guarded instead of
  // duplicating the fragile pointer arithmetic at both call sites.
  size_t namelen = strlen(name);
  GDT_BOOLEAN IsHtmlFile =
    (namelen>=5 && strstr(name,".html")==name+namelen-5) ||
    (namelen>=4 && strstr(name,".htm")==name+namelen-4);
  if (!IsHtmlFile)
    cout << "\n<pre>" << endl;

  // Get the record in HTML explicitly
  //  pdb->Present(RsRecord, ESet, &Record);
  pdb->Present(RsRecord, ESet, HtmlRecordSyntax, &Record);
  cout << Record;

  if (!IsHtmlFile)
    cout << "\n</pre>" << endl;
  
  return 0;
}

