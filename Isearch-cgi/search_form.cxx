/* $Id: search_form.cxx,v 1.8 2000/02/04 22:52:25 cnidr Exp $ */
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
File:          	search_form.cxx
Version:        1.03
$Revision: 1.8 $
Description:    CGI app that builds a search html form for Iindex-ed databases
Author:         Kevin Gamiel, kgamiel@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include "gdt.h"
#include "isearch.hxx"
#include "common.hxx"

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

#define SIMPLE 1
#define BOOLEAN 2
#define ADVANCED 3

int main(int argc, char **argv)
{
  PCHR db, dbpath, www;
  GDT_BOOLEAN JustHTML=GDT_FALSE;
  CHR temp[64];
  INT type;

  if (!setlocale(LC_CTYPE,"")) {
    cout << "Warning: Failed to set the locale!" << endl;
  }

  if (argc < 3) {
    cout << "search_form " << IsearchVersion << endl;
    cout << "Copyright (c) 1995-2000 MCNC/CNIDR and A/WWW Enterprises" << endl;
    cout << "This file produces a custom HTML search page based upon a ";
    cout << "specified Isearch database.\n" << endl;
    cout << "Usage:  search_form [-page type] <dbpath> <dbname>\n";
    cout << "\nThe following page types are available:\n";
    cout << "  -simple\n  -boolean\n  -advanced\n  " << endl;
    cout << "  -html  // for HTML collections only" << endl;
    exit(0);
  }
  type=0;
  // BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgisearch_formcxx): this
  // used to strcpy(temp,argv[1]) into a fixed 64-byte buffer with no
  // bounds check -- argv[1] is a flag in the "-pagetype ..." form, but
  // it's the dbpath itself in the plain form, which can legitimately
  // be longer than 63 characters. Confirmed a real stack buffer
  // overflow (glibc's *** buffer overflow detected *** / SIGABRT) with
  // a >64-char dbpath. The copy was never actually necessary --
  // argv[1] itself is only ever read, never mutated, in this scope --
  // so just use it directly instead of copying it into a small fixed
  // buffer at all.
  if (argv[1][0]=='-') {
    if (!strcmp(argv[1],"-simple")) {
      type=SIMPLE;
    } else {
      if (!strcmp(argv[1],"-boolean")) {
        type=BOOLEAN;
      } else {
        if (!strcmp(argv[1],"-advanced")) {
          type=ADVANCED;
        } else {
	  if (!strcmp(argv[1],"-html")) {
	    type=BOOLEAN;
	    JustHTML=GDT_TRUE;
	  } else {
	    cout << "Form type " << argv[1] << " not recognized.\n\n";
	    exit(0);
	  }
	}
      }
    }
    // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgisearch_formcxx): the
    // argc < 3 check above only guarantees argv[1] and argv[2] --
    // nowhere near enough for this branch, which needs argv[1..3]
    // (flag, dbpath, dbname). Reading db=argv[3] when argc==3 read the
    // C++-standard-guaranteed nullptr sentinel at argv[argc] (harmless
    // here since db is never used afterward), but www=argv[4] read
    // *past* that guaranteed range entirely -- undefined behavior, not
    // just null -- and DBRootName=argv[3] a few lines below (a real
    // STRING assignment) crashed via strlen(nullptr). Confirmed with
    // the real binary: `search_form -simple <onepath>` (argc==3)
    // segfaulted before this fix. Fixed by requiring the 4 arguments
    // this branch actually needs before reading any of them.
    if (argc < 4) {
      cout << "Usage:  search_form " << argv[1] << " <dbpath> <dbname> [www]\n";
      exit(0);
    }
    dbpath = argv[2];
    db = argv[3];
    www = (argc > 4) ? argv[4] : nullptr;
  } else {
    dbpath = argv[1];
    db = argv[2];
    www = (argc > 3) ? argv[3] : nullptr;
  }


	
  STRING DBPathName;
  STRING DBRootName;
  IDB *pdb;
  INT FieldCount;
  DFDT dfdt;
  DFD dfd;
	
  // Open database
  if (type>0) {
    DBPathName = argv[2];
    DBRootName = argv[3];
  } else {
    DBPathName = argv[1];
    DBRootName = argv[2];
    type=SIMPLE;
  }

  pdb = new IDB(DBPathName, DBRootName);
	
  // Is the database valid?
  if (pdb->GetTotalRecords() <= 0) {
    cout << "Database " << DBRootName;
    cout << " does not exist or is corrupted\n";
    exit(1);
  }

  cout << "<html><head>" << endl;

  switch (type) {
    case SIMPLE: {
      cout << "<title>Simple Search Page</title>" << endl;
      cout << "</head>\n<body>" << endl;
      cout << "<h2>Simple Search</h2>" << endl;
      break;}
    case BOOLEAN: {
      cout << "<title>Boolean Search Page</title>" << endl;
      cout << "</head>\n<body>" << endl;;
      cout << "<h2>Boolean Search</h2>" << endl;
      break; }
    case ADVANCED: {
      cout << "<title>Advanced Search Page</title>" << endl;
      cout << "</head>\n<body>" << endl;
      cout << "<h2>Advanced Search</h2>" << endl;
      break; }
  }
  cout << "<!-- Isearch-cgi search page, produced by Isearch-cgi's search_form -->" << endl;
  cout << "<a href=\"http://www.cnidr.org/\"><i>CNIDR</a> Isearch-cgi " << endl;
  cout << IsearchVersion << "</i><p>" << endl;

  cout << "Database Name: " << DBRootName << "<br>" << endl;
  cout << "Total Records: " << pdb->GetTotalRecords() << "<br>" << endl;

  if (JustHTML)
    cout << "<form method=POST action=\"/cgi-bin/ihtml\">" << endl;
  else
    cout << "<form method=POST action=\"/cgi-bin/isearch\">" << endl;
  
  cout << "<input name=\"DATABASE\" type=hidden value=\"" << DBRootName;
  cout << "\">" << endl;
  cout << "<input name=\"SEARCH_TYPE\" type=hidden value=\"";
  switch (type) {
    case SIMPLE: cout << "SIMPLE"; break;
    case BOOLEAN: cout << "BOOLEAN"; break;
    case ADVANCED: cout << "ADVANCED"; break;
  }
  cout << "\">" << endl;
  if (www) {
    cout << "<input name=\"HTTP_PATH\" type=hidden value=\"" << www;
    cout << "\">" << endl;
  }

  pdb->GetDfdt(&dfdt);
  FieldCount = dfdt.GetTotalEntries();

  STRING Field;
  INT j;

  switch (type) {
    case SIMPLE: {
      for (INT i=1;i <= 3;i++) {
        snprintf(temp, sizeof(temp), "FIELD_%i", i);
        cout << "Field: <select name=\"" << temp << "\">" << endl;
        cout << "<option selected value=\"FULLTEXT\">FullText" << endl;
    
        for(j=1;j <= FieldCount;j++) {
          dfdt.GetEntry(j, &dfd);
          dfd.GetFieldName(&Field);
          cout << "<option>" << Field << endl;
        }
        cout << "</select>" << endl;
        snprintf(temp, sizeof(temp), "TERM_%i", i);
        cout << "Term: <input name=\"" << temp << "\">" << endl;
        snprintf(temp, sizeof(temp), "WEIGHT_%i", i);
        cout << "Weight: <select name=\"" << temp << "\">" << endl;
        cout << "<option selected>1" << endl;
        cout << "<option>2" << endl;
        cout << "<option>3" << endl;
        cout << "</select>" << endl;
    
        cout << "<br>" << endl;
      }
      break;
    }
    case BOOLEAN: {
      for (INT i=1;i <= 2;i++) {
        snprintf(temp, sizeof(temp), "FIELD_%i", i);
        cout << "Field: <select name=\"" << temp << "\">" << endl;
        cout << "<option selected value=\"FULLTEXT\">FullText" << endl;
    
	for(j=1;j <= FieldCount;j++) {
	  dfdt.GetEntry(j, &dfd);
	  dfd.GetFieldName(&Field);
	  cout << "<option>" << Field << endl;
	}
        cout << "</select>" << endl;
        snprintf(temp, sizeof(temp), "TERM_%i", i);
        cout << "Term: <input name=\"" << temp << "\">" << endl;
        snprintf(temp, sizeof(temp), "WEIGHT_%i", i);
        cout << "Weight: <select name=\"" << temp << "\">" << endl;
        cout << "<option selected>1" << endl;
        cout << "<option>2" << endl;
        cout << "<option>3" << endl;
        cout << "</select>" << endl;
  
        if (i==1) {
          cout << "<p>\n<ul>\n<select name=\"OPERATOR\">\n";
          cout << "<option selected>AND" << endl;
	  cout << "<option>OR" << endl;
	  cout << "<option>ANDNOT" << endl;
	  cout << "<option>NEAR" << endl;
          cout << "</select>" << endl;
	  cout << "</ul><p>" << endl;
        } else 
          cout << "<br>" << endl;	
      }
      break;
    }
    case ADVANCED: {
      cout << "<b>Query:</b>" << endl;
      cout << "<textarea rows=2 cols=75 maxcols=80 name=\"ISEARCH_TERM\">" << endl;
      cout << "</textarea><br>" << endl;
      break;
    }
  }


  cout << "<HR>" << endl;

  if (JustHTML) {
    cout << "<input name=\"ELEMENT_SET\" type=hidden value=\"TITLE\">";
  } else {
    cout << "Select Field to Display as Headline: ";
    cout << "<select name=\"ELEMENT_SET\">" << endl;
    if(FieldCount == 0) {
      cout << "<option selected value=\"B\">Brief" << endl;
    } 
    for(j=1;j <= FieldCount;j++) {
      dfdt.GetEntry(j, &dfd);
      dfd.GetFieldName(&Field);
      cout << "<option>" << Field << endl;
    }

  cout << "</select>" << endl;
  }
  cout << "<p>";
  cout << "Enter maximum number of hits to retrieve: " << endl;
  cout << "<input size=4 name=\"MAXHITS\" value=\"50\"><p>" << endl;
  cout << "Result format: <select name=\"OUTPUT\">" << endl;
  cout << "<option selected value=\"HTML\">HTML" << endl;
  cout << "<option value=\"JSON\">JSON (results only)" << endl;
  cout << "</select><p>" << endl;
  cout << "<input type=\"submit\" value=\" Submit Query \">" << endl;
  cout << "<input type=reset value=\" Clear Entries \">" << endl;

  cout << "</form>" << endl;

  if (type==ADVANCED) {
    cout << "<hr>\nExamples of advanced searches:<ul>" << endl;
    cout << "<li>roses and red" << endl;
    cout << "<li>(title/dogs and h1/cats) or h3/monkeys" << endl;
    cout << "<li>(img/stars and img/stripes) or (bald and eagle)\n</ul>" << endl;
  }
  cout << "</body></html>" << endl;

 return 0;
}
