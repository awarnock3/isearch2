/* $Id: Iget.cxx,v 1.3 2001/03/06 04:14:16 cnidr Exp $ */
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
File:		Iget.cxx
Version:	1.00
$Revision: 1.3 $
Description:	Command-line retrieval utility
Author:		A. Warnock (warnock@awcubed.com), adapted from Isearch.cxx
                by Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "isearch.hxx"

#include "common.hxx"
//#include "infix2rpn.hxx"
#include "dtreg.hxx"
//#include "rcache.hxx"
#include "index.hxx"
//#include "fprec.hxx"
//#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "vidb.hxx"
//#include "thesaurus.hxx"

int 
main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr,"Iget v%s\n", IsearchVersion);
    fprintf(stderr,"Copyright (c) 2000-2001 MCNC/CNIDR and A/WWW Enterprises\n");
    fprintf(stderr,"-d (X)   # Search database with root name (X).\n");
    fprintf(stderr,"-id (X)  # Request document(s) with docid (X).\n");
    fprintf(stderr,"-p (X)   # Present element set (X) with results.\n");
    fprintf(stderr,"-f (X)   # Present results in format (X).\n");
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
    fflush(stdout); fflush(stderr); exit (0);
  }

  STRLIST DocTypeOptions;
  STRING Flag;
  STRING DBName, RecordKey, ESet, RecordSyntax;
  INT x = 0;
  INT LastUsed = 0;
  STRING DBPathName, DBFileName;
  STRING PathName, FileName;
  RESULT RsRecord;
  STRING Record;
  GDT_BOOLEAN Error=GDT_FALSE;
  STRING error_message="";
  STRLIST RecordKeyList;
  INT n_keys;

  ESet = "F";
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("ERROR: No database name specified after -d.\n");
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-id")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("ERROR: No document ID specified after -id.\n");
	}
	RecordKey = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-p")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("ERROR: No element set specified after -p.\n");
	}
	ESet = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-f")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("ERROR: No format specified after -f.\n");
	}
	RecordSyntax = argv[x];
	LastUsed = x;
      }
    }
    x++;
  }
	
  if (DBName.Equals("")) {
    DBName = IsearchDefaultDbName;
  }

  RecordKeyList.Split(',',RecordKey);
  n_keys = RecordKeyList.GetTotalEntries();
  if (n_keys <= 0) {
    Error=GDT_TRUE;
    error_message.Cat("No record ids given.\n");
  }
  
  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }

  x = LastUsed + 1;
  if (x > argc) {
    Error=GDT_TRUE;
    error_message.Cat("Unrecognized arguments\n");
  }
  
  DBPathName = DBName;
  DBFileName = DBName;
  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);

  // See if we have a legitimate file
  if (!DBExists(DBName)) {
    // The file does not exist
    Error=GDT_TRUE;
    error_message.Cat("The specified database was not found: ");
    error_message.Cat(DBName);
    error_message.Cat("\n");
  }

  if (Error) {
    error_message.Print(stderr);
    EXIT_ERROR;
  }

  VIDB *pdb;
  pdb = new VIDB(DBPathName, DBFileName, DocTypeOptions);
  /*
  if (DebugFlag) {
    pdb->DebugModeOn();
  }
  */
  if (!pdb->IsDbCompatible()) {
    Error=GDT_TRUE;
    error_message.Cat("ERROR: The specified database is not compatible with this version of Iget.\n");
    delete pdb;
    error_message.Print(stderr);
    EXIT_ERROR;
  }

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

  RecordKey = "";
  for (INT i=1;i<=n_keys;i++) {

    RecordKeyList.GetEntry(i,&RecordKey);
    pdb->KeyLookup(RecordKey, &RsRecord);
    pdb->Present(RsRecord, ESet, RecordSyntax, &Record);
    if (Record.GetLength() > 0) {
      Record.Print();

    } else {
      error_message.Cat("ERROR: Record ");
      error_message.Cat(RecordKey);
      error_message.Cat(" not found.\n");
      error_message.Print(stderr);
    }
  }

  delete pdb;
  fflush(stdout); fflush(stderr); exit (0);
}
