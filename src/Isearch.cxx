/* $Id: Isearch.cxx,v 1.42 2001/03/06 04:14:16 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994-2001. 

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
File:		Isearch.cxx
Version:	1.02
$Revision: 1.42 $
Description:	Command-line search utility
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09, reprocessed 2026-08-31
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
// NOTE: this is a `main()`-only CLI entry point (like src/Iget.cxx and
// src/Iindex.cxx), so it cannot be linked into the shared Catch2 test
// binary (its `main()` would collide with catch_amalgamated.cpp's
// own). At 754 lines the deep audit prioritized the search/result-set
// pipeline (VIDB::Search()/AndSearch() and their return-value handling)
// and the argument-parsing loop over the JSON-output/interactive-
// browsing tail of main(). Verified via compile-clean confirmation and
// manual trace-through, including reading VIDB::Search()/AndSearch()'s
// own implementation in src/vidb.cxx to confirm a real (not
// hypothetical) nullptr-return path; see docs/BUG_CATALOG.md for the
// full note, including a documented-but-unimplemented `-RECT{...}`
// flag left as-is rather than guessed-at. Reopened 2026-08-31 for a
// user-reported bug in the very argument-parsing loop this turn's own
// audit covered but missed -- see `BUGFIX #2`.

#include <stdio.h>
#include <string.h>
#include <locale.h>

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
#include "vidb.hxx"
#include "thesaurus.hxx"

static void PrintJsonEscaped(const STRING& Value) {
  CHR* text = Value.NewCString();
  putchar('"');
  for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
    unsigned char c = *p;
    switch (c) {
      case '\"': fputs("\\\"", stdout); break;
      case '\\': fputs("\\\\", stdout); break;
      case '\b': fputs("\\b", stdout); break;
      case '\f': fputs("\\f", stdout); break;
      case '\n': fputs("\\n", stdout); break;
      case '\r': fputs("\\r", stdout); break;
      case '\t': fputs("\\t", stdout); break;
      default:
        if (c < 0x20) {
          printf("\\u%04x", (unsigned int)c);
        } else {
          putchar((int)c);
        }
        break;
    }
  }
  putchar('"');
  delete [] text;
}

/// Parses search flags (-d/-p/-f/-json/-and/-rpn/-infix/-syn/...) and a
/// trailing word list, runs the query against the named database, then
/// either prints JSON (-json) or an interactive/terse result listing.
int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr,"Isearch v%s\n", IsearchVersion);
    fprintf(stderr,"Copyright (c) 1995-2001 MCNC/CNIDR and A/WWW Enterprises\n");
    fprintf(stderr,"-d (X)        # Search database with root name (X).\n");
    fprintf(stderr,"-V            # Print the version number.\n");
    fprintf(stderr,"-p (X)        # Present element set (X) with results.\n");
    fprintf(stderr,"              # Values: B (brief filename), F (full record),\n");
    fprintf(stderr,"              # field name, comma-separated list, or doctype-\n");
    fprintf(stderr,"              # specific element sets (for example A/S/C/R).\n");
    fprintf(stderr,"-f (X)        # Present results in format (X).\n");
    fprintf(stderr,"              # Values: TEXT, SUTRS, USMARC, HTML, SGML,\n");
    fprintf(stderr,"              # XML, GRS-1, or syntax OIDs:\n");
    fprintf(stderr,"              # 1.2.840.10003.5.101 (SUTRS)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.10 (USMARC)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.109.3 (HTML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.108 (old HTML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.1000.34.1 (CNIDR HTML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.109.9 (SGML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.1000.34.2 (CNIDR SGML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.109.10 (XML)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.105 (GRS-1)\n");
    fprintf(stderr,"              # 1.2.840.10003.5.109 (MIME->SUTRS)\n");
    fprintf(stderr,"-json         # Return search results as JSON.\n");
    fprintf(stderr,"-q            # Print results and quit immediately.\n");
    fprintf(stderr,"-t            # Print terse results and quit immediately.\n");
    fprintf(stderr,"-and          # Perform boolean \"and\" on results.\n");
    fprintf(stderr,"-rpn          # Interpret as an RPN query.\n");
    fprintf(stderr,"-infix        # Interpret as a boolean algebra query.\n");
    fprintf(stderr,"-syn          # Do synonym expansion.\n");
    fprintf(stderr,"-o (X)        # Document type specific option.\n");
    fprintf(stderr,"-prefix (X)   # Add prefix (X) to matched terms in document.\n");
    fprintf(stderr,"-suffix (X)   # Add suffix (X) to matched terms in document.\n");
    fprintf(stderr,"-byterange    # Print the byte range of each document within\n");
    fprintf(stderr,"              # the file that contains it.\n");
    fprintf(stderr,"-startdoc (X) # Display result set starting with the (X)th\n");
    fprintf(stderr,"              # document in the list.\n");
    fprintf(stderr,"-enddoc (X)   # Display result set ending with the (X)th document\n");
    fprintf(stderr,"              # in the list.\n");
    fprintf(stderr,"-RECT{North South West East}  # Find targets that overlap\n");
    fprintf(stderr,"                              # this geographic rectangle.\n");
    fprintf(stderr,"(X) (Y) (...)  # Search for words (X), (Y), etc.\n");
    fprintf(stderr,"               # [fieldname/]searchterm[*][:n]\n");
    fprintf(stderr,"               # Prefix with fieldname/ for fielded searching.\n");
    fprintf(stderr,"               # Append * for right truncation.\n");
    //    cout << "                        // Append ~ for soundex search." << endl;
    fprintf(stderr,"               # Append :n for term weighting (default=1).\n");
    fprintf(stderr,"               # (Use negative values to lower rank.)\n");
    fprintf(stderr,"Examples: Isearch -d POETRY truth \"beaut*\" urn:2\n");
    fprintf(stderr,"          Isearch -d WEBPAGES title/library\n");
    fprintf(stderr,"          Isearch -d STORIES -rpn title/cat title/dog or title/mouse and\n");
    fprintf(stderr,"          Isearch -d STORIES -infix '(title/cat or title/dog) and title/mouse'\n");
    fprintf(stderr,"          Isearch -d PRUFROCK -infix '(ether and table) or mermaids'\n");
    fprintf(stderr,"          Isearch -d BIBLE -infix '(Saul||Goliath)&&David'\n");
    fprintf(stderr,"Document Types Supported:");
    DTREG dtreg(0);
    STRLIST DocTypeList;
    dtreg.GetDocTypeList(&DocTypeList);
    STRING s;
    INT x;
    INT y = DocTypeList.GetTotalEntries();
    for (x=1; x<=y; x++) {
      DocTypeList.GetEntry(x, &s);
      fprintf(stderr,"\t ");
      s.Print(stderr);
    }
    fprintf(stderr,"\n");
    //    fflush(stdout); fflush(stderr); exit (0);
    RETURN_ERROR;
  }

  STRLIST DocTypeOptions;
//  GDT_BOOLEAN Merge=GDT_TRUE;
  STRING Flag;
  STRING DBName;
  STRING ElementSet;
  STRING RecordSyntax;
  STRING TermPrefix, TermSuffix;
  STRING StartDoc="", EndDoc="";
  INT DebugFlag = 0;
  INT QuitFlag = 0;
  INT ByteRangeFlag = 0;
  INT BooleanAnd = 0;
  INT RpnQuery = 0;
  INT InfixQuery = 0;
  INT SpatialRectFlag=0;
  INT x = 0;
  INT LastUsed = 0;
  GDT_BOOLEAN TerseFlag=GDT_FALSE;
  GDT_BOOLEAN Synonyms=GDT_FALSE;
  GDT_BOOLEAN JsonFlag=GDT_FALSE;
	
  ElementSet = "B";
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-o")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No option specified after -o.\n");
	  //	  fflush(stdout); fflush(stderr); exit (0);
	  RETURN_ERROR;
	}
	STRING S;
	S = argv[x];
	DocTypeOptions.AddEntry(S);
	LastUsed = x;
      }
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No database name specified after -d.\n");
	  RETURN_ERROR;
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-p")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No element set specified after -p.\n");
	  RETURN_ERROR;
	}
	ElementSet = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-f")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No format specified after -f.\n");
	  RETURN_ERROR;
	}
	RecordSyntax = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-prefix")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No prefix specified after -prefix.\n\n");
	  RETURN_ERROR;
	}
	TermPrefix = argv[x];
	LastUsed = x;
      }
      //      if (Flag.Equals("-nomerge")) {
	//	Merge=GDT_FALSE;
	//	LastUsed=x;
	//      }
      if (Flag.Equals("-suffix")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No suffix specified after -suffix.\n");
	  RETURN_ERROR;
	}
	TermSuffix = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-startdoc")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No value specified after -startdoc.\n");
	  RETURN_ERROR;
	}
	StartDoc = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-enddoc")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No value specified after -enddoc.\n");
	  RETURN_ERROR;
	}
	EndDoc = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-q")) {
	QuitFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-syn")) {
	Synonyms = GDT_TRUE;
	LastUsed = x;
      }
      if (Flag.Equals("-t")) {
	TerseFlag = GDT_TRUE;
	QuitFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-byterange")) {
	ByteRangeFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-json")) {
	JsonFlag = GDT_TRUE;
	TerseFlag = GDT_TRUE;
	QuitFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-and")) {
	BooleanAnd = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-rpn")) {
	RpnQuery = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-infix")) {
	InfixQuery = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-V")) {
	// BUGFIX #2 (docs/BUG_CATALOG.md#srcisearchcxx): this printed
	// nothing and exited 1 (a real-error code) instead of printing
	// the version and exiting 0, as the file's own usage text (`-V
	// # Print the version number.`) promises. Unlike src/Iindex.cxx
	// -- whose main() prints its version banner unconditionally as
	// the very first statement, before argument parsing even starts,
	// so its own near-identical `RETURN_ZERO`-only `-V` case works
	// only as a side effect -- this file's version print happens
	// later, gated behind `!TerseFlag`, well after this early-return
	// case. Confirmed live: `bin/Isearch -V` produced no output and
	// exited 1 before this fix. Fixed by printing directly here
	// instead of relying on a later code path this case never
	// reaches, and returning 0 to match the actual outcome (this is
	// a successful informational request, not an error).
	fprintf(stderr,"Isearch v%s\n", IsearchVersion);
	RETURN_ZERO;
      }
      if (Flag.Equals("-debug")) {
	DebugFlag = 1;
	LastUsed = x;
      }
    }
    x++;
  }
	
  if (DBName.Equals("")) {
    DBName = IsearchDefaultDbName;
  }
	
  if ( (RpnQuery) && (BooleanAnd) ) {
    fprintf(stderr,"ERROR: The -rpn and -and options can not be used together.\n");
    RETURN_ERROR;
  }

  if ( (InfixQuery) && (BooleanAnd) ) {
    fprintf(stderr,"ERROR: The -infix and -and options can not be used together.\n");
    RETURN_ERROR;
  }

  if ( (RpnQuery) && (InfixQuery) ) {
    fprintf(stderr,"ERROR: The -rpn and -infix options can not be used together.\n");
    RETURN_ERROR;
  }
	
  if(!TerseFlag) {
    fprintf(stderr,"Isearch v%s\n", IsearchVersion);
  }

  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }

  INT NumWords = 0;
  for (INT argi = 1; argi < argc; ++argi) {
    STRING Arg = argv[argi];
    if (Arg.Equals("-o") || Arg.Equals("-d") || Arg.Equals("-p") ||
	Arg.Equals("-f") || Arg.Equals("-prefix") || Arg.Equals("-suffix") ||
	Arg.Equals("-startdoc") || Arg.Equals("-enddoc")) {
      ++argi; // skip value for options that take one argument
      continue;
    }
    if (Arg.Equals("-q") || Arg.Equals("-syn") || Arg.Equals("-t") ||
	Arg.Equals("-byterange") || Arg.Equals("-json") || Arg.Equals("-and") ||
	Arg.Equals("-rpn") || Arg.Equals("-infix") || Arg.Equals("-V") ||
	Arg.Equals("-debug")) {
      continue;
    }
    ++NumWords;
  }
  if (NumWords <= 0) {
    RETURN_ERROR;
  }

  STRING *WordList = new STRING[NumWords];
  INT z = 0;
  for (INT argi = 1; argi < argc; ++argi) {
    STRING Arg = argv[argi];
    if (Arg.Equals("-o") || Arg.Equals("-d") || Arg.Equals("-p") ||
	Arg.Equals("-f") || Arg.Equals("-prefix") || Arg.Equals("-suffix") ||
	Arg.Equals("-startdoc") || Arg.Equals("-enddoc")) {
      ++argi;
      continue;
    }
    if (Arg.Equals("-q") || Arg.Equals("-syn") || Arg.Equals("-t") ||
	Arg.Equals("-byterange") || Arg.Equals("-json") || Arg.Equals("-and") ||
	Arg.Equals("-rpn") || Arg.Equals("-infix") || Arg.Equals("-V") ||
	Arg.Equals("-debug")) {
      continue;
    }
    WordList[z++] = argv[argi];
  }
  
  STRING DBPathName, DBFileName;
  STRING PathName, FileName;
  SQUERY squery;
  RSET  *prset=(RSET*)NULL;
  IRSET *pirset=(IRSET*)NULL;
  RESULT result;
  INT t, n;
	
  if (!DBExists(DBName)) {
    fprintf(stderr,"Database ");
    DBName.Print(stderr);
    fprintf(stderr," does not exist.\n");
    RETURN_ZERO;
  }

  DBPathName = DBName;
  DBFileName = DBName;
  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);

  VIDB *pdb;
  pdb = new VIDB(DBPathName, DBFileName, DocTypeOptions);
  //    IDB *pdb=(PIDB)NULL;
  //    pdb = new IDB(DBPathName, DBFileName, DocTypeOptions);

  if (DebugFlag) {
    pdb->DebugModeOn();
  }
  
  if (!pdb->IsDbCompatible()) {
    fprintf(stderr,"The specified database is not compatible with this version of Isearch.\n");
    fprintf(stderr,"Please use matching versions of Iindex, Isearch, and Iutil.\n");
    delete [] WordList;
    delete pdb;
    RETURN_ERROR;
  }
  
  if(!TerseFlag) {
    printf("Searching database ");
    DBName.Print();
    printf(":\n");
  }

  if (Synonyms) {
    squery.OpenThesaurus(DBPathName, DBFileName);
  }

  STRING QueryString;
  for (z=0; z<NumWords; z++) {
    if (z != 0) {
      QueryString.Cat(' ');
    }
    QueryString.Cat(WordList[z]);
  }

  if (RpnQuery || InfixQuery) {
    if (InfixQuery) {
      STRING TempString;
      INFIX2RPN *Parser;
      Parser = new INFIX2RPN(QueryString, &TempString);
      if (Parser->InputParsedOK()) {
	QueryString = TempString;
	delete Parser;
      }
      else {
	if (Parser->GetErrorMessage(&TempString)) {
	  fprintf(stderr,"INFIX QUERY ERROR : ");
	  TempString.Print(stderr);
	  fprintf(stderr,"\n");
	  RETURN_ERROR;
	}
	else {
	  fprintf(stderr,"INFIX QUERY ERROR: Unable to parse\n");
	  RETURN_ERROR;
	}
      }
    }
    squery.SetRpnTerm(QueryString);
  } else {
    squery.SetTerm(QueryString);
  }

  if(!TerseFlag) {
    printf("Query String = ");
    QueryString.Print();
    printf("\n");
  }

  if (Synonyms) {
    STRING S;
    squery.ExpandQuery();
    squery.GetTerm(&S);
    if(!TerseFlag) {
      printf("Expanded Query String = ");
      S.Print();
      printf("\n");
    }
  }

  if (BooleanAnd) {
    pirset = pdb->AndSearch(squery);
  } else {
    pirset = pdb->Search(squery);
  }

  // BUGFIX #1 (docs/BUG_CATALOG.md#srcisearchcxx): VIDB::Search()/
  // AndSearch() (src/vidb.cxx) both return nullptr when the virtual
  // database has no usable sub-databases ("Bail out if no databases",
  // c_dbcount <= 0) -- a real, reachable state for a misconfigured or
  // empty virtual-database registry, not just a defensive guess. The
  // very next line used to dereference pirset unconditionally.
  if (!pirset) {
    fprintf(stderr,"ERROR: Search failed (no usable databases).\n");
    delete [] WordList;
    delete pdb;
    RETURN_ERROR;
  }

  n = pirset->GetTotalEntries();
  pirset->SortByScore();
  
  // Set the record syntax to SUTRS if it is not specified, and
  // convert OIDs to text, if necessary
  if (RecordSyntax.GetLength() == 0)
    RecordSyntax = SutrsRecordSyntax;
  else if (RecordSyntax.CaseEquals("TEXT"))
    RecordSyntax = SutrsRecordSyntax;
  else if (RecordSyntax.CaseEquals(SutrsRecordSyntaxOID))
    RecordSyntax = SutrsRecordSyntax;
  else if (RecordSyntax.CaseEquals(MimeRecordSyntaxOID))
    RecordSyntax = SutrsRecordSyntax;
  else if (RecordSyntax.CaseEquals(UsmarcRecordSyntaxOID))
    RecordSyntax = UsmarcRecordSyntax;
  else if (RecordSyntax.CaseEquals(HtmlRecordSyntaxOID))
    RecordSyntax = HtmlRecordSyntax;
  else if (RecordSyntax.CaseEquals(SgmlRecordSyntaxOID))
    RecordSyntax = SgmlRecordSyntax;
  else if (RecordSyntax.CaseEquals(XmlRecordSyntaxOID))
    RecordSyntax = XmlRecordSyntax;
  else if (RecordSyntax.CaseEquals(GRS1RecordSyntaxOID))
    RecordSyntax = GRS1RecordSyntax;
  else if (RecordSyntax.CaseEquals(OldHtmlRecordSyntaxOID))
    RecordSyntax = HtmlRecordSyntax;
  else if (RecordSyntax.CaseEquals(CNIDRHtmlRecordSyntaxOID))
    RecordSyntax = HtmlRecordSyntax;
  else if (RecordSyntax.CaseEquals(CNIDRSgmlRecordSyntaxOID))
    RecordSyntax = SgmlRecordSyntax;

  pdb->BeginRsetPresent(RecordSyntax);
	
  if(!TerseFlag) {
    printf("\n%i document(s) matched your query, ", n);
  }
	
  // Display only documents in -startdoc/-enddoc range
       INT x1, x2;
  x1 = StartDoc.GetInt();
  if(x1 <= 1)
    x1 = 1;
  x2 = EndDoc.GetInt();
  
  if ( (x1 != 1) || (x2 != 0) ) {
    if (x2 == 0)
      x2 = n;
    
    PRSET NewPrset;
    NewPrset=pirset->GetRset(x1-1,x2); 
    pirset->Fill(x1-1,x2,NewPrset);
    NewPrset->SetScoreRange(pirset->GetMaxScore(),
			    pirset->GetMinScore());
    delete prset;
    prset = NewPrset;
  } else {			
    // display all of them
    prset=pirset->GetRset(0,n);
    pirset->Fill(0,n,prset);
    prset->SetScoreRange(pirset->GetMaxScore(),
			 pirset->GetMinScore());
  }
  
#ifdef DEBUG  
  if (n>0) {
    printf(" unscaled scores from %i to %i\n",
	   pirset->GetMinScore(), pirset->GetMaxScore());
  } else
    printf("\n");
#endif
  
  n = prset->GetTotalEntries();
  if(!TerseFlag) {
    printf("%i document(s) displayed.\n\n", n);
  }

  INT TotalMatches = pirset->GetTotalEntries();

  if (JsonFlag) {
    RESULT JsonResult;
    STRING JsonPath, JsonFile, JsonKey, JsonElement, JsonBrief, JsonTotalBrief, JsonTempElementSet;
    INT score;

    printf("{\"database\":");
    PrintJsonEscaped(DBName);
    printf(",\"query\":");
    PrintJsonEscaped(QueryString);
    printf(",\"total_matches\":%i", TotalMatches);
    printf(",\"displayed\":%i", n);
    printf(",\"start_doc\":%i", x1);
    printf(",\"end_doc\":%i", x2 == 0 ? n : x2);
    printf(",\"results\":[");

    for (t=1; t<=n; t++) {
      prset->GetEntry(t, &JsonResult);
      score = prset->GetScaledScore(JsonResult.GetScore(), 100);
      JsonResult.GetPathName(&JsonPath);
      JsonResult.GetFileName(&JsonFile);
      JsonResult.GetKey(&JsonKey);

      JsonTotalBrief = "";
      JsonTempElementSet = ElementSet;
      while (!JsonTempElementSet.Equals("")) {
	JsonElement = JsonTempElementSet;
	if ( (x=JsonTempElementSet.Search(',')) ) {
	  JsonElement.EraseAfter(x-1);
	  JsonTempElementSet.EraseBefore(x+1);
	} else {
	  JsonTempElementSet = "";
	}
	pdb->Present(JsonResult, JsonElement, RecordSyntax, &JsonBrief);
	if (JsonTotalBrief.GetLength() > 0) {
	  JsonTotalBrief += " | ";
	}
	JsonTotalBrief += JsonBrief;
      }

      if (t > 1) {
	printf(",");
      }
      printf("{\"rank\":%i,\"score\":%i,\"path\":", t, score);
      PrintJsonEscaped(JsonPath);
      printf(",\"file\":");
      PrintJsonEscaped(JsonFile);
      printf(",\"key\":");
      PrintJsonEscaped(JsonKey);
      printf(",\"brief\":");
      PrintJsonEscaped(JsonTotalBrief);
      printf(",\"record_start\":%ld,\"record_end\":%ld}",
	     (long)JsonResult.GetRecordStart(),
	     (long)JsonResult.GetRecordEnd());
    }
    printf("]}\n");
    pdb->EndRsetPresent(RecordSyntax);
    delete [] WordList;
    delete pirset;
    delete prset;
    delete pdb;
    RETURN_ZERO;
  }
  
  CHR Selection[80];
  CHR s[256];
  INT FileNum;
  STRING BriefString;
  STRING Element, TempElementSet;
  GDT_BOOLEAN FirstRun = GDT_TRUE;
  STRLIST BriefList;
  STRING TotalBrief;
  STRING ResultKey;
  STRING Delim;

  INT MajorCount=0;
  //  INT LoadPos=1;

  do {
    if ((n != 0) && (!TerseFlag)) {
      printf("      Score   File\n");
    }
    
    for (t=1; t<=n; t++) {
      //      if(MajorCount%20==0) {
      //	LoadPos=1;
      //      } else {
      //	LoadPos++;
      //      }
      
      //      prset->GetEntry(LoadPos, &result);
      prset->GetEntry(t, &result);
      ++MajorCount;
      
      if(!TerseFlag) {
	printf("%4i.", t);
	printf("%6i   ", prset->GetScaledScore(result.GetScore(), 100));
      } else {
	printf("%i | ", prset->GetScaledScore(result.GetScore(), 100));
      }
      
      result.GetPathName(&PathName);
      result.GetFileName(&FileName);
      PathName.Print();
      FileName.Print();
      
      result.GetKey(&ResultKey);
      if(TerseFlag) {
	printf(" | ");
	ResultKey.Print();
	printf(" | ");
      } else {
	printf("\n");
      }
      
      if (ByteRangeFlag) {
	printf("              [ %i - %i ]\n",
	       result.GetRecordStart(),
	       result.GetRecordEnd());
				
	if (TerseFlag)
	  printf(" | ");
      }
			
      if (FirstRun) {
	TotalBrief = "";
	TempElementSet = ElementSet;
	while (!TempElementSet.Equals("")) {
	  Element = TempElementSet;
	  if ( (x=TempElementSet.Search(',')) ) {
	    Element.EraseAfter(x-1);
	    TempElementSet.EraseBefore(x+1);
	  } else {
	    TempElementSet = "";
	  }
	  pdb->Present(result, Element, RecordSyntax, &BriefString);
	  Delim = " | ";
	  TotalBrief += BriefString;
 
	  if(TerseFlag) {   
	    TotalBrief += Delim;
	  }
	}
	BriefList.AddEntry(TotalBrief);
      } else {
	BriefList.GetEntry(t, &TotalBrief);
      }
      /*	TotalBrief.Replace("\n","");           // for P. Schweitzer*/

      if (TotalBrief.GetLength() > 0) {
	TotalBrief.Print();
      }
      printf("\n");
    }
    pdb->EndRsetPresent(RecordSyntax);
    if ( (QuitFlag) || (n == 0) ) {
      FileNum = 0;
    } else {
      printf("\nSelect file #: ");
      if (!fgets(Selection,79,stdin)) {
	FileNum = 0;
      } else {
	FileNum = atoi(Selection);
      }
    }
    if ( (FileNum > n) || (FileNum < 0) ) {
      printf("\nSelect a number between 1 and %i.\n", n);
    }
    if ( (FileNum != 0) && (FileNum <= n) && (FileNum >= 1) ) {
      prset->GetEntry(FileNum, &result);
      
      STRING Buf;
      STRING Full;
      Full = "F";
      if ( (TermPrefix.Equals("")) && (TermSuffix.Equals("")) ) {
	pdb->Present(result, Full, RecordSyntax, &Buf);
      } else {
	result.GetHighlightedRecord(TermPrefix, TermSuffix, &Buf);
      }
      Buf.Print();
      // printf("\n");
			
      printf("Press <Return> to select another file: ");
      if (!fgets(s,255,stdin)) {
	s[0] = '\0';
      }
      printf("\n");
      //      LoadPos=0;
      MajorCount=0;
			
    }
    if (FirstRun == GDT_TRUE) {
      FirstRun = GDT_FALSE;
    }
  } while (FileNum != 0);
  
  delete [] WordList;
  delete pirset;
  delete prset;
  delete pdb;
  //  fflush(stdout); fflush(stderr); exit (0);
  RETURN_ZERO;

}
