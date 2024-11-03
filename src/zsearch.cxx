/* $Id: zsearch.cxx,v 1.15 2001/03/06 04:14:16 cnidr Exp $ */
/************************************************************************
Copyright (c) A/WWW Enterprises, 2000-2001
************************************************************************/

/************************************************************************
Original Copyright Notice

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
File:		zsearch.cxx
Version:	1.00
$Revision: 1.15 $
Description:	Command-line search utility
Author:		Archie Warnock (warnock@awcubed.com), based on 
                Isearch by Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>
#ifdef UNIX
#include <unistd.h>
#endif

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

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr,"zsearch v%s\n", IsearchVersion);
    fprintf(stderr,"Copyright (c) 200-2001 MCNC/CNIDR and A/WWW Enterprises\n");
    fprintf(stderr,"-d (X)        # Search database with root name (X).\n");
    fprintf(stderr,"-V            # Print the version number.\n");
    fprintf(stderr,"-and          # Perform boolean \"and\" on results.\n");
    fprintf(stderr,"-rpn          # Interpret as an RPN query.\n");
    fprintf(stderr,"-infix        # Interpret as a boolean algebra query.\n");
    fprintf(stderr,"-syn          # Do synonym expansion.\n");
    fprintf(stderr,"-o (X)        # Document type specific option.\n");
    fprintf(stderr,"-prefix (X)   # Add prefix (X) to matched terms in document.\n");
    fprintf(stderr,"-suffix (X)   # Add suffix (X) to matched terms in document.\n");
    fprintf(stderr,"-byterange    # Print the byte range of each document within\n");
    fprintf(stderr,"              # the file that contains it.\n");
    fprintf(stderr,"              # in the list.\n");
    fprintf(stderr,"-RECT{North South West East}  # Find targets that overlap\n");
    fprintf(stderr,"                              # this geographic rectangle.\n");
    fprintf(stderr,"(X) (Y) (...) # Search for words (X), (Y), etc.\n");
    fprintf(stderr,"              # [fieldname/]searchterm[*][:n]\n");
    fprintf(stderr,"              # Prefix with fieldname/ for fielded searching.\n");
    fprintf(stderr,"              # Append * for right truncation.\n");
    //    cout << "                        // Append ~ for soundex search." << endl;
    fprintf(stderr,"              # Append :n for term weighting (default=1).\n");
    fprintf(stderr,"              # (Use negative values to lower rank.)\n");
    fprintf(stderr,"Examples: zsearch -d POETRY truth \"beaut*\" urn:2\n");
    fprintf(stderr,"          zsearch -d WEBPAGES title/library\n");
    fprintf(stderr,"          zsearch -d STORIES -rpn title/cat title/dog or title/mouse and\n");
    fprintf(stderr,"          zsearch -d STORIES -infix '(title/cat or title/dog) and title/mouse'\n");
    fprintf(stderr,"          zsearch -d PRUFROCK -infix '(ether and table) or mermaids'\n");
    fprintf(stderr,"          zsearch -d BIBLE -infix '(Saul||Goliath)&&David'\n");

    fprintf(stderr,"\n");
    fprintf(stderr,"zsearch is currently experimental and should be used\n");
    fprintf(stderr,"cautiously.  Suggestions and improvements are welcomed\n");
    fprintf(stderr,"\n");
    fflush(stdout); fflush(stderr); exit (0);
  }

  printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  printf("<!-- Generated by zsearch v%s, part of the Isite package from -->\n", IsearchVersion);
  printf("<!-- A/WWW Enterprises, http://www.awcubed.com/Isite/ -->\n");
  printf("<!DOCTYPE zsearch SYSTEM \"zsearch.dtd\">\n");
  printf("<zsearch xmlns:isearch=\"http://www.awcubed.com/dtd\">\n");

  STRLIST DocTypeOptions;
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
  GDT_BOOLEAN Error=GDT_FALSE;
  STRING error_message="";

  ElementSet = "B";
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-o")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No option specified after -o.</isearch:error_text>\n");
	}
	STRING S;
	S = argv[x];
	DocTypeOptions.AddEntry(S);
	LastUsed = x;
      }
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No database name specified after -d.</isearch:error_text>\n");
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-p")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No element set specified after -p.</isearch:error_text\n");
	}
	ElementSet = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-f")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No format specified after -f.</isearch:error_text>\n");
	}
	RecordSyntax = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-prefix")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No prefix specified after -prefix.</isearch:error_text>\n");
	}
	TermPrefix = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-suffix")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No suffix specified after -suffix.</isearch:error_text>\n");
	}
	TermSuffix = argv[x];
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
	fflush(stdout); fflush(stderr); exit (0);
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
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>The -rpn and -and options can not be used together.</isearch:error_text>\n");
  }

  if ( (InfixQuery) && (BooleanAnd) ) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>The -infix and -and options can not be used together.</isearch:error_text>\n");
  }

  if ( (RpnQuery) && (InfixQuery) ) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>The -rpn and -infix options can not be used together.</isearch:error_text>\n");
  }
	
  /*
  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }
  */

  x = LastUsed + 1;
  if (x >= argc) {
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>Unrecognized arguments</isearch:error_text>\n");
  }
  
  INT NumWords = argc - x;
  INT z = x;

  STRING *WordList = new STRING[NumWords];
  for (z=0; z<NumWords; z++) {
    WordList[z] = argv[z+x];
  }
  
  STRING DBPathName, DBFileName;
  STRING DBCheckName;
  STRING XmlBuffer;
  STRING PathName, FileName;
  SQUERY squery;
  PRSET  prset=(PRSET)NULL;
  PIRSET pirset=(PIRSET)NULL;
  RESULT result;
  INT t, n;
	
  DBPathName = DBName;
  DBFileName = DBName;
 
  // See if we have a legitimate file
  if (!DBExists(DBName)) {
    // The file does not exist
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>The specified database was not found: ");
    XmlBuffer = DBName;
    XmlBuffer.XmlCleanup();
    error_message.Cat(XmlBuffer);
    error_message.Cat("</isearch:error_text>\n");
  }

  if (Error) {
    XmlBuffer = DBName;
    XmlBuffer.XmlCleanup();
    cout << "\t<isearch:search status=\"Error\" dbname=\"" << XmlBuffer
	 << "\">" << endl;

    XmlBuffer = error_message;
    //    XmlBuffer.XmlCleanup();
    cout << "\t\t<isearch:error_block>" << endl;
    cout << XmlBuffer;
    cout << "\t\t</isearch:error_block>" << endl;
    cout << "\t</search>" << endl;
    cout << "</zsearch>" << endl;

    fflush(stdout); 
    fflush(stderr); 
    exit (0);
  //    EXIT_ERROR;
  }

  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);

  VIDB *pdb;
  pdb = new VIDB(DBPathName, DBFileName, DocTypeOptions);

  if (DebugFlag) {
    pdb->DebugModeOn();
  }
  
  if (!pdb->IsDbCompatible()) {
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>The specified database is not compatible with this version of zsearch.</isearch:error_text>\n");
    delete [] WordList;
    delete pdb;
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

  // cerr << "Query String ->" << QueryString << "<-" << endl;

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
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>");
	  error_message.Cat("INFIX QUERY ERROR: ");
	  TempString.XmlCleanup();
	  error_message.Cat(TempString);
	  error_message.Cat("</isearch:error_text>\n");
	}
	else {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>");
	  error_message.Cat("INFIX QUERY ERROR: Unable to parse -");
	  XmlBuffer = QueryString;
	  XmlBuffer.XmlCleanup();
	  error_message.Cat(XmlBuffer);
	  error_message.Cat("</isearch:error_text>\n");
	}
      }
    }
    squery.SetRpnTerm(QueryString);
  } else {
    squery.SetTerm(QueryString);
  }

  if (Error) {
    XmlBuffer = DBName;
    XmlBuffer.XmlCleanup();
    cout << "\t<isearch:search status=\"Error\" dbname=\"" << XmlBuffer
	 << "\">" << endl;
    cout << "\t\t<isearch:error_block>" << endl;
    XmlBuffer = error_message;
    //    XmlBuffer.XmlCleanup();
    cout << XmlBuffer;
    cout << "\t\t</isearch:error_block>" << endl;
    cout << "\t</isearch:search>" << endl;
    cout << "</zsearch>" << endl;

    fflush(stdout); 
    fflush(stderr); 
    exit (0);

    //    EXIT_ERROR;
  }

  QueryString.XmlCleanup();
  cout << "\t<isearch:query>" << QueryString << "</isearch:query>" << endl;

  if (Synonyms) {
    STRING S;
    squery.ExpandQuery();
    squery.GetTerm(&S);
    XmlBuffer = S;
    XmlBuffer.XmlCleanup();
    cout << "\t<isearch:query type=\"expanded\">" 
	 << XmlBuffer << "</isearch:query>" << endl;
  }

  if (BooleanAnd) {
    pirset = pdb->AndSearch(squery);
  } else {
    pirset = pdb->Search(squery);
  }

  if (pirset) {
    printf("\t<isearch:search status=\"OK\" dbname=\"");
    XmlBuffer = DBName;
    XmlBuffer.XmlCleanup();
    XmlBuffer.Print();
    printf("\">\n");

    n = pirset->GetTotalEntries();
    printf("\t\t<isearch:results count=\"%i\">\n",n);

    pirset->SortByScore();
    RecordSyntax = SutrsRecordSyntax;
    pdb->BeginRsetPresent(RecordSyntax);
	
    // display all of them
    prset=pirset->GetRset(0,n);
    pirset->Fill(0,n,prset);
    prset->SetScoreRange(pirset->GetMaxScore(),
			 pirset->GetMinScore());
  
    n = prset->GetTotalEntries();
  
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

    for (t=1; t<=n; t++) {
      prset->GetEntry(t, &result);
      ++MajorCount;
      
      result.GetPathName(&PathName);
      result.GetFileName(&FileName);
      
      result.GetVKey(&ResultKey);
      printf("\t\t\t<isearch:result rank=\"%i\"",t);
      cout << " docid=\"" << ResultKey << "\" ";
      printf(" score=\"%i\"/>\n",
	     prset->GetScaledScore(result.GetScore(), 100));

      /*
      if (ByteRangeFlag) {
	printf("              [ %i - %i ]\n",
	       result.GetRecordStart(),
	       result.GetRecordEnd());
	
	if (TerseFlag)
	  printf(" | ");
      }
      */
    }
    printf("\t\t</isearch:results>\n");
    pdb->EndRsetPresent(RecordSyntax);
    delete pirset;
    
    //  } else {
  }

  printf("\t</isearch:search>\n");

  if (WordList)
    delete [] WordList;
  if (prset)
    delete prset;
  if (pdb)
    delete pdb;

  printf("</zsearch>\n");
  fflush(stdout); 
  fflush(stderr); 
  exit (0);
}
