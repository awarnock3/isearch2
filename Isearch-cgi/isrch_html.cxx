/* $Id: isrch_html.cxx,v 1.9 1998/05/18 19:22:06 cnidr Exp $ */
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
File:          	isrch_html.cxx
Version:        1.00
$Revision: 1.9 $
Description:    CGI app that searches against Iindex-ed databases of 
                HTML documents in the http server document tree
Authors:        Archie Warnock, warnock@clark.net
History:
                Derived from isrch_srch.cxx, originally written by
                Kevin Gamiel, kgamiel@cnidr.org
		Tim Gemma, stone@k12.cnidr.org
		Monty Walls, mwalls@castor.oktax.state.ok.us
		Archie Warnock, warnock@clark.net
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifdef UNIX
#include <sys/time.h>
#else
#include <time.h>
#endif

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
#include "tokengen.hxx"
#include "infix2rpn.hxx"
#include "config.hxx"
#include "cgi-util.hxx"

#define SEARCH_TYPE(n)		((n) & SEARCH_TYPE_MASK)
#define SEARCH_CLASS(n)		((n) & SEARCH_CLASS_MASK)

#define SEARCH_TYPE_MASK	0xf0
#define SEARCH_CLASS_MASK	0x0f

#define SIMPLE 			0x10
#define ADVANCED 		0x20
#define BOOLEAN 		0x30

#define BOOLEAN_AND		0x01

PCHR gettok(PCHR input);
INT get_term(INT t, STRING &PrintTerm, STRING &PrintField, STRING &PrintWeight);
PCHR get_field(const CHR *fmt, INT n);

INT Search(PCHR DBPath, PCHR DBName, STRING& query_str, STRING& ESName,
	INT Start, INT MaxHits, INT TYPE);
void  PutHTTPHeader(void);
void  PutHTMLHead(void);
void  PutHTMLBodyStart(void);
void  PutHTMLBodyEnd(void);

#define MAXHIT_DEFAULT 50
#define MAXSTR	1024

CGIAPP *cgidata;
STRING query;
// Randy.Wood@nau.edu
PCHR cgiDir;

INT main(int argc, char **argv)
{
  CHR *db, *term, *field, *weight, *p, *path;
  STRING ESName;
  CHR temp[MAXSTR+1];
  INT Start, MaxHits, i, type, x, y, z, terms;
  STRING PrintQuery, PrintTerm, PrintField, PrintWeight;

  if (!setlocale(LC_CTYPE,"")) {
    cout << "Warning: Failed to set the locale!" << endl;
  }

  cgidata = new CGIAPP();

  // Write the preliminary stuff out - these can be customzied
  PutHTTPHeader();
  PutHTMLHead();
  PutHTMLBodyStart();

  // Good for debugging form values
  // cgidata->Display();
  // exit(0);

  if ((db = cgidata->GetValueByName("DATABASE")) == nullptr) {
    cout << "<B>You must specify a database name.</B>" << endl;
    PutHTMLBodyEnd();
    exit(0);
  }

  // Randy.Wood@nau.edu
  // if cgi-bin was specified by the web form, use it instead of cgi-bin
  if ((cgiDir = cgidata->GetValueByName("CGI_BIN")) == nullptr) {
    cgiDir = new CHR[10];
    strcpy(cgiDir,"cgi-bin");
  }

  // Which result set record number should be displayed first?

  Start = (p = cgidata->GetValueByName("START")) ? atoi(p): 1;

  // How many result set records should be displayed?
  MaxHits = (p = cgidata->GetValueByName("MAXHITS")) ? atoi(p): MAXHIT_DEFAULT;
  if (MaxHits == 0)
    MaxHits = 1;
  else if (MaxHits < 0)
    MaxHits = MAXHIT_DEFAULT;

  // What element set to return
  // "B" = brief.
  // "F" = fulltext.
  ESName = (p = cgidata->GetValueByName("ELEMENT_SET")) ? p: "B";
  query = "";

  // If they want URLs returned, there has to be a value for HTTP_PATH
  path = cgidata->GetValueByName("HTTP_PATH");

  // assume simple search type until told otherwise
  type = SIMPLE;
  if ((p = cgidata->GetValueByName("SEARCH_TYPE")))
    if (StrCaseCmp(p, "ADVANCED") == 0)
      type = ADVANCED;
    else if (StrCaseCmp(p, "BOOLEAN") == 0)
      type = BOOLEAN;

  if (SEARCH_TYPE(type) == ADVANCED) {
    // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiisrch_htmlcxx):
    // GetValueByName() returns nullptr when the named CGI field wasn't
    // submitted (e.g. an ADVANCED search request missing
    // ISEARCH_TERM); assigning that straight into a STRING calls
    // STRING::operator=(const CHR*), which dereferences it
    // unconditionally via strlen(). Confirmed with the real binary:
    // SEARCH_TYPE=ADVANCED with no ISEARCH_TERM segfaulted before this
    // fix. Fixed by treating a missing field the same as an empty one,
    // matching the ternary-guarded pattern already used for every
    // other field in this function (DATABASE, START, MAXHITS, etc.).
    p = cgidata->GetValueByName("ISEARCH_TERM");
    query = p ? p : "";
    PrintQuery = query;
  }
  else if (SEARCH_TYPE(type) == BOOLEAN) {
    STRING TempQuery;
    terms = 0;

    // BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgiisrch_htmlcxx): p was
    // left nullptr when the OPERATOR field wasn't submitted, then
    // reused unconditionally in every StrCaseCmp(p, ...)/Cat(p) call
    // below for the 2nd and later terms -- StrCaseCmp() calls
    // strcasecmp() directly, which (like STRING::operator=) doesn't
    // tolerate a null argument. Confirmed with the real binary: a
    // BOOLEAN search with 2 terms and no OPERATOR field segfaulted
    // before this fix. Defaulted to "OR" (matching this same file's
    // own documented "otherwise is an implied OR" convention for the
    // SIMPLE search branch below) so p is never null going into the
    // loop.
    if ((p = cgidata->GetValueByName("OPERATOR")))
      type = (StrCaseCmp(p, "AND") == 0) ? type | BOOLEAN_AND: type;
    else
      p = (PCHR)"OR";

    // Build up the infix query from the components
    // Might as well make a nice printable version, too
    for (i=1; get_term(i,PrintTerm,PrintField,PrintWeight) > 0; ++i) {
      if (i>1) {
	if (StrCaseCmp(p, "AND") == 0) {
	  PrintQuery.Cat(" and ");
	  TempQuery.Cat(" and ");
	} else if (StrCaseCmp(p, "ANDNOT") == 0) {
	  PrintQuery.Cat(" and not ");
	  TempQuery.Cat(" andnot ");
	} else if (StrCaseCmp(p, "OR") == 0) {
	  PrintQuery.Cat(" or ");
	  TempQuery.Cat(" or ");
	} else if (StrCaseCmp(p, "NEAR") == 0) {
	  PrintQuery.Cat(" is near ");
	  TempQuery.Cat(" near ");
	} else {
	  PrintQuery.Cat(" ");
	  PrintQuery.Cat(p);
	  PrintQuery.Cat(" ");
	  
	  TempQuery.Cat(" ");
	  TempQuery.Cat(p);
	  TempQuery.Cat(" ");
	}
      }

      PrintQuery.Cat(PrintTerm);
      if (PrintField.GetLength()>0) {
	PrintQuery.Cat(" in ");
	PrintQuery.Cat(PrintField);
	TempQuery.Cat(PrintField);
	TempQuery.Cat("/");
      }
      TempQuery.Cat(PrintTerm);
      if (PrintWeight.GetLength()>0) {
	TempQuery.Cat(":");
	TempQuery.Cat(PrintWeight);
      }
      
    }
    query = TempQuery;
    type = ADVANCED;
  }
  else {
     // only SIMPLE searches get here 
    terms = 0;

    // only 1 OPERATOR is possible = AND otherwise is an implied OR
    if ((p = cgidata->GetValueByName("OPERATOR")))
      type = (StrCaseCmp(p, "AND") == 0) ? type | BOOLEAN_AND: type;

    for (i=1; get_term(i,PrintTerm,PrintField,PrintWeight) > 0; ++i) {
      // since we know that logic is either "AND" or "OR" just insert
      if (i>1) {
	if (SEARCH_CLASS(type) == BOOLEAN_AND)
	  PrintQuery.Cat(" and ");
	else
	  PrintQuery.Cat(" or ");     
      }
      PrintQuery.Cat(PrintTerm);
      if (PrintField.GetLength()>0) {
        PrintQuery.Cat(" in ");
        PrintQuery.Cat(PrintField);
      }
    }
  }

  if (query.Equals("")) {
    /*
     * cout << "<B>You must enter a query term</B>" << endl;
     * PutHTMLBodyEnd();
     * exit(0);
     */
    // From Monty Walls
    if (Start >1) {
      // BUGFIX #1, continued (docs/BUG_CATALOG.md#isearch-cgiisrch_htmlcxx):
      // same missing-field-crash shape as the ADVANCED branch above --
      // a "next page" request (Start>1) with no ISEARCH_TERM field
      // would crash here identically.
      p = cgidata->GetValueByName("ISEARCH_TERM");
      query = p ? p : "";
      // only 1 OPERATOR is possible, or get implied OR
      if ((p=cgidata->GetValueByName("OPERATOR")))
	type = (StrCaseCmp(p,"AND") == 0) ? type|BOOLEAN_AND:type;
    } else {
      cout << "<B>You must enter a query term</B>" << endl;
      PutHTMLBodyEnd();
      exit(0);
    }
  }

  cout << "<H2>Operation Summary</H2>" << endl;
  cout << "<B>Query:</B> <CODE>" << endl;
  cout << PrintQuery << "</CODE><P>" << endl;

  INT nhits;
  nhits = Search(argv[1], db, query, ESName, Start, MaxHits, type);
  if (nhits > 0) {
    cout << "<HR>" << endl;
  }
  
  PutHTMLBodyEnd();
  delete cgidata;
  exit(0);
}

/*
 * slight variation in the traditional strtok
 * forces the delimiters to balance - good for
 * quotes
 *
 * based on Henry Spencers implementation of strtok.c
 */

PCHR
gettok(PCHR input)
{
  static PCHR last;
  CHR *pos, *tok;
  CHR lc;

  if (input == nullptr && last == nullptr)
    return (nullptr);

  pos = (input == nullptr)? last: input;
  
  for (lc = ' '; *pos != '\0'; ++pos) {
    if (*pos == ' ')
      continue;
    
    // either non-delimiter or '"'
    if (*pos != '"')
      lc = ' ';
    else {
      lc = '"';
      ++pos;
    }

    // found non-delimiter
    for (tok = pos; *pos; ++pos)
      if (*pos != lc)
	continue;
      else {
	*pos = '\0';
	last = ++pos;
	return (tok);
      }
    // loop ends on a null
    last = pos;
    return (tok);
  }
  return (nullptr);
}

PCHR
get_field(const CHR *f, INT n)
{
  PCHR bp;
  PCHR field;
  bp = new CHR[MAXSTR+1];

  snprintf(bp, MAXSTR+1, f, n);
  if ((field = cgidata->GetValueByName(bp))) {
    delete [] bp;
    return (field);
  }

  delete [] bp;
  return (nullptr);
}

INT
get_term(INT i, STRING &PrintTerm, STRING &PrintField, STRING &PrintWeight)
{
  PCHR buffer;
  PCHR s;
  PCHR argument;
  PCHR field;
  PCHR weight;
  PCHR entry;
  INT w, terms;
  PCHR phrase_term = nullptr;

  buffer = new CHR[MAXSTR+1];

  // See if the form included a button to request phrase searching
  PCHR phrase;
  GDT_BOOLEAN do_phrase=GDT_FALSE;
  if ((phrase = get_field("PHRASE_%i",i)) != nullptr) {
    if (StrCaseCmp(phrase,"YES") == 0) {
      do_phrase = GDT_TRUE;
    } else {
      do_phrase = GDT_FALSE;
    }
  }

  *buffer = '\0';
  terms = 0;

  if ((argument = get_field("TERM_%i", i)) == nullptr) {
    delete [] buffer;
    return (0);
  }

  // Even if there's no button, do phrase searching if the phrase is
  // in quotes
  if (argument[0] == '"')
    do_phrase=GDT_TRUE;

  // One other case to consider - no quotes, but the user has asked for
  // phrase searching via separate input.  Nest the query in quotes so
  // that gettok handles it correctly, and so it gets passed to the
  // TOKENGEN routines.

  if ((do_phrase) && (argument[0] != '"')) {
    phrase_term = new CHR[strlen(argument)+3];
    strcpy(phrase_term,"\"");
    strcat(phrase_term,argument);
    strcat(phrase_term,"\"");
    argument = phrase_term;
  }

  if ((field = get_field("FIELD_%i", i)) != nullptr) {
    if (StrCaseCmp(field, "FULLTEXT") == 0) {
      strcpy(field,"");
    }
  }

  // BUGFIX #4 (docs/BUG_CATALOG.md#isearch-cgiisrch_htmlcxx): same
  // missing-field-crash shape as BUGFIX #1/#2 -- field stays nullptr
  // when FIELD_%i wasn't submitted (a completely ordinary request: no
  // field restriction means search all fields), and this assigned it
  // straight into a STRING unconditionally. Confirmed with the real
  // binary: a BOOLEAN search with terms but no FIELD_1/FIELD_2
  // segfaulted here before this fix.
  PrintField = field ? field : "";

  if ((weight = get_field("WEIGHT_%i", i)) != nullptr) {
    w = atoi(weight);
    PrintWeight = weight;
  }
  else {
    w = 0;
    PrintWeight = "";
  }

  PrintTerm = argument;

  while ((s = gettok(argument)) != nullptr) {
    argument = nullptr;

    entry = (PCHR)&buffer[0];

    if (do_phrase) {
      if ((field != nullptr) && (strlen(field) > 0)) {
        if (w > 0)
          snprintf(entry, MAXSTR+1, "%.128s/\"%.256s\":%d", field, s, w);
        else
          snprintf(entry, MAXSTR+1, "%.128s/\"%.256s\"", field, s);
      }
      else {
        if (w > 0)
          snprintf(entry, MAXSTR+1, "\"%.256s\":%d", s, w);
        else
          snprintf(entry, MAXSTR+1, "\"%.256s\"", s);
      }

    } else {
      if ((field != nullptr) && (strlen(field) > 0)) {
        if (w > 0)
          snprintf(entry, MAXSTR+1, "%.128s/%.256s:%d", field, s, w);
        else
          snprintf(entry, MAXSTR+1, "%.128s/%.256s", field, s);
      }
      else {
        if (w > 0)
          snprintf(entry, MAXSTR+1, "%.256s:%d", s, w);
        else
          snprintf(entry, MAXSTR+1, "%.256s", s);
      }
    }

    if (!query.Equals(""))
      query.Cat(" ");

    query.Cat(entry);
    ++terms;
  }
  delete [] buffer;
  if (phrase_term != nullptr) {
    delete [] phrase_term;
  }
  return (terms);
}

/*
  Path    = full path to directory where database files reside, 
            e.g. "/usr/dbs".
  Name    = root database name, e.g. "MYDB".
  Field   = field in which to search.  NULL means full text search.
  Term    = term(s) for which to search.
  ESName  = field to display as headline.  NULL will choose whatever is 
            available.
  MaxHits = maximum number of hits to display

  Returns number of hits on success, -1 on failure
*/

INT Search(PCHR DBPath, PCHR DBName, STRING& query_str, STRING& ESName,
	INT Start, INT MaxHits, INT type)
{
  PRSET prset;
  PIRSET pirset;
  STRING DBPathName;
  STRING DBRootName;
  SQUERY query;
  IDB *pdb;
  INT HitCount, FieldCount, NextStart, NextCount, i;
  DFDT dfdt, rec_dfdt;
  DFD dfd;
  time_t StartTime, EndTime;


  if (!ESName.Equals("F"))
    ESName = "B";

  switch (SEARCH_TYPE(type)) {
  case ADVANCED: {
    // break the query string into tokens.
    STRLIST PhraseList;
    STRING StrTerm;
    TOKENGEN TokenGen(query_str);

    // this loop processes each term.
    query_str = "";
    INT IsBool=0;
    INT TotalPhrases = TokenGen.GetTotalEntries();
    for (i=1;i <= TotalPhrases;i++) {
      TokenGen.GetEntry(i, &StrTerm);
      if ( (StrTerm ^= "AND") 
	   || (StrTerm ^= "OR") 
	   || (StrTerm ^= "ANDNOT") 
	   || (StrTerm == "||") 
	   || (StrTerm == "&&") ) {
	IsBool = 1;
      }
      //rebuild the query
      query_str += StrTerm;
      if (i < TotalPhrases)
	query_str += ' ';
    }
    if (IsBool) {
      STRING ProcessedQuery;
      INFIX2RPN Parser;

      Parser.SetDefaultOp("OR");
      Parser.Parse(query_str, &ProcessedQuery);
      
      if (!Parser.InputParsedOK()) {
	cout << "The Query <i>" << query_str << "</i>\n";
	cout << "was unparseable. If you think this is an error in this\n";
	cout << "gateway, send mail to <a href=\"mailto:isite@cnidr.org\">";
	cout << "CNIDR's Isite Technical Support</a>. Please include all relevant\n";
	cout << "information, including the URL of the page you're searching\n";
	cout << "in and the query that you entered.\n";
	exit(0);
      } else {
	query.SetRpnTerm(ProcessedQuery);
      }
    } else
      query.SetTerm(query_str);
    break;
  }
  case SIMPLE: {
    query.SetTerm(query_str); 
    break;
  }

  }

  // Open database
  DBPathName=DBPath;
  DBRootName=DBName;

  if ((pdb = new IDB(DBPathName, DBRootName)) == nullptr) {
    printf("Failed to open database [%s]\n", DBName);
    return -1;
  }

  // Is the database valid?
  if (pdb->GetTotalRecords() <= 0) {
    cout << "Database " << DBRootName;
    cout << " does not exist or is corrupted\n";
    return -1;
  }

  // Execute the search
  time(&StartTime);
  if (SEARCH_CLASS(type) == BOOLEAN_AND)
    pirset=pdb->AndSearch(query);
  else
    pirset=pdb->Search(query);
  time(&EndTime);

  // BUGFIX #3 (docs/BUG_CATALOG.md#isearch-cgiisrch_htmlcxx): no null
  // check on pirset before dereferencing it -- the same root cause
  // already confirmed and fixed in src/Isearch.cxx's own BUGFIX #1
  // (VIDB::Search()/AndSearch() return nullptr when there's nothing to
  // search), traced here one level down: INDEX::Search()'s final
  // `TempStack >> NewIrset;` (src/index.cxx) explicitly sets NewIrset
  // to nullptr via OPSTACK::Pop()'s empty-stack branch when the SQUERY
  // has zero operands, and that nullptr propagates unchanged through
  // IDB::Search()/AndSearch(). A query that tokenizes to zero real
  // search terms (SQUERY::SetTerm() on a degenerate string) reaches
  // this exact path.
  if (!pirset) {
    cout << "<B>Unable to process query.</B>" << endl;
    delete pdb;
    return -1;
  }

  pirset->SortByScore();

  // How many hits?
  HitCount = pirset->GetTotalEntries();
//  pdb->BeginRsetPresent(RecordSyntax);

  PRSET NewPrset;

  NewPrset=pirset->GetRset(0,HitCount);
  pirset->Fill(0,HitCount,NewPrset);
  NewPrset->SetScoreRange(pirset->GetMaxScore(),
			  pirset->GetMinScore());
  prset = NewPrset;
  HitCount = prset->GetTotalEntries();

  INT FetchCount 
    = HitCount > (Start + MaxHits - 1) ? MaxHits : (HitCount - Start + 1);

  cout << "<i>Matching Record Count:</i> " << HitCount << "<br>\n";
  cout << "<i>Total Retrieved:</i> " << FetchCount << "<br>\n";
  cout << "<i>Interpreted Query:</i> " << query_str << "<br>\n";
  cout << "<i>Total Database Records:</i> " << pdb->GetTotalRecords();
  cout << "<br>\n";
  cout << "<i>Query Time:</i> " << (EndTime - StartTime);
  cout << " seconds<br>" << endl;

  cout << "<H2>Results</H2>" << endl;

  if (HitCount == 0) {
    cout << "<p>\n<b>No matches found.</b>\n<p>" << endl;
    return 0;
  } else cout << "<HR>";

  pdb->GetDfdt(&dfdt);
  FieldCount = dfdt.GetTotalEntries();

  // Build the HTML output.  You'll probably want to customize here
  RESULT RsRecord;
  STRING File, RecordKey, Headline, Field, Fullname;
  DOUBLE Score;
  INT j, x, y;
  CHR *url, *name, *HttpPath;

  HttpPath=(char *)getenv("DOCUMENT_ROOT");

  /*
    CHR *PathInfo,*PathTranslated;
    PathInfo=(char *)getenv("PATH_INFO");
    PathTranslated=(char *)getenv("PATH_TRANSLATED");
    cout << "PATH_INFO=" << PathInfo << "<BR>" << endl;
    cout << "PATH_TRANSLATED=" << PathTranslated << "<BR>" << endl;
  */

  for (i=Start;i <= (Start + FetchCount - 1);i++) {
    // Fetch the first hit
    prset->GetEntry(i, &RsRecord);
    // Construct a headline for this hit in HTML
    //    pdb->Present(RsRecord, ESName, &Headline);
    pdb->Present(RsRecord, ESName, HtmlRecordSyntax, &Headline);

    // Get the name of the file
    RsRecord.GetFullFileName(&Fullname);
    RsRecord.GetFileName(&File);
    name=Fullname.NewCString();

    // Find the url
    if (HttpPath) {
      url=strstr(name,HttpPath);
      if (url)
        url=url+strlen(HttpPath);
    } else
      url=nullptr;

    // Get the unique database key for this record to be use
    // in subsequent retrieval when URL is clicked.
    RsRecord.GetKey(&RecordKey);

    Score = prset->GetScaledScore(RsRecord.GetScore(),100);

    cout << "<b>Match Number:</b> " << i << " of " << HitCount 
         << "<br>" << endl;
    cout << "<b>Score:</b> " << Score << " - " << endl;

    /* Files not within the given WWW path must be accessed with ifetch
       for their full text */
    if (url==nullptr) {
#if defined(_WIN32) || defined (MSDOS)
      cout << "<a href=\"ifetch.cmd?";
#else
      cout << "<a href=\"ifetch?";
#endif
      cout << DBRootName;
      cout << "+";
      cout << RecordKey;
      cout << "+F\">";
      if (Headline.GetLength() > 0) {
	cout << Headline;
	cout << " [<i>";
	cout << File;
	cout << "</i>]</a>  " << endl;
      } else {
	cout << File;
	cout << "</a>" << endl;
      }
    } else {		// Just print the URL
      cout << "<a href=\""; 
      cout << url;
      cout << "\">";
      if (Headline.GetLength() > 0) {
	cout << Headline;
	cout << " [<i>";
	cout << File;
	cout << "</i>]</a>" << endl;
      } else {
	cout << File;
	cout << "</a>" << endl;
      }
    }

      //    }
    if ((i + 1) <= HitCount)
      cout << "<HR>";
  }

  INT Remaining = (HitCount - (Start + MaxHits - 1));
  NextStart = Start + MaxHits;
  NextCount = Remaining < MaxHits ? Remaining : MaxHits;

  // If we're not on the first page, let's give them a way to move back
  if (Start > 1) {

    // Display a mini-form with CGI variables
    cout << endl;
    // Randy.Wood@nau.edu
    // Break up the cgi-bin button to use the variable instead of hard-code
    cout << "<FORM ACTION=\"/";
    cout << cgiDir;
#if defined(_WIN32) || defined (MSDOS)
    cout << "\ihtml.cmd\" METHOD=\"POST\">";
#else
    cout << "/ihtml\" METHOD=\"POST\">";
#endif
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"CGI_BIN\" VALUE=\"";
    cout << cgiDir;
    cout << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"START\" VALUE=\"";
    cout << Start - FetchCount << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"MAXHITS\" VALUE=\"";
    cout << MaxHits << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"ISEARCH_TERM\" VALUE=\"";
    cout << query_str << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"SEARCH_TYPE\" VALUE=\"";

    switch (type) {
      case SIMPLE: cout << "SIMPLE"; break;
      case ADVANCED: cout << "ADVANCED"; break;
    }

    cout << "\">";
    cout << endl;
    if (HttpPath) {
      cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"HTTP_PATH\" VALUE=\"";
      cout << HttpPath << "\">";
    }
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"ELEMENT_SET\" VALUE=\"";
    cout << ESName << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"DATABASE\" VALUE=\"";
    cout << DBName << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Previous ";
    cout << FetchCount << "\">";
    cout << endl;
    cout << "</FORM>" << endl;
  }

  // If we were unable to display all hits, lets give them a hyperlink
  // to display the next MaxHits of them.

  if (Remaining > 0) {

    // Display a mini-form with CGI variables
    cout << endl;
    // Randy.Wood@nau.edu
    // Break up the cgi-bin button to use the variable instead of hard-code
    cout << "<FORM ACTION=\"/";
    cout << cgiDir;
#if defined(_WIN32) || defined (MSDOS)
    cout << "/ihtml.cmd\" METHOD=\"POST\">";
#else
    cout << "/ihtml\" METHOD=\"POST\">";
#endif
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"CGI_BIN\" VALUE=\"";
    cout << cgiDir;
    cout << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"START\" VALUE=\"";
    cout << NextStart << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"MAXHITS\" VALUE=\"";
    cout << MaxHits << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"ISEARCH_TERM\" VALUE=\"";
    cout << query_str << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"SEARCH_TYPE\" VALUE=\"";

    switch (type) {
      case SIMPLE: cout << "SIMPLE"; break;
      case ADVANCED: cout << "ADVANCED"; break;
    }

    cout << "\">";
    cout << endl;
    if (HttpPath) {
      cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"HTTP_PATH\" VALUE=\"";
      cout << HttpPath << "\">";
    }
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"ELEMENT_SET\" VALUE=\"";
    cout << ESName << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"DATABASE\" VALUE=\"";
    cout << DBName << "\">";
    cout << endl;
    cout << "<INPUT TYPE=\"SUBMIT\" VALUE=\"Retrieve Next ";
    cout << NextCount << "\">";
    cout << endl;
    cout << "</FORM>" << endl;
  }

  return HitCount;
}

void  PutHTTPHeader()
{
  cout << "Content-type: text/html\n\n";
}

void  PutHTMLHead()
{
  cout << "<html>\n<head>" << endl;
  cout << "<title>Isearch Results</title>" << endl;
  cout << "</head>" << endl;
}

void  PutHTMLBodyStart()
{
  cout << "<body>" << endl;
  cout << "<a href=\"http://www.cnidr.org/\"><i>CNIDR</a> Isearch-cgi ";
  cout << IsearchVersion << "</i><p>" << endl;
}

void  PutHTMLBodyEnd()
{
  cout << "</body></html>" << endl;
}
