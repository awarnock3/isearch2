// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// $Id: fgdcsite.cxx,v 1.4 1998/11/04 04:50:53 cnidr Exp $
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
File:		fgdcsite.cxx
Version:	$Revision
Description:	Class FGDCSITE - for FGDC Node descriptions
Author:		Kevin Gamiel, Kevin.Gamiel@cnidr.org
@@@*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "isearch.hxx"
#include "sgmltag.hxx"
#include "fgdcsite.hxx"

FGDCSITE::FGDCSITE(PIDBOBJ DbParent) 
  : SGMLTAG(DbParent) 
{
}


// Reads the file named by the "-o fieldtype=<filename>" doctype
// option (prompting interactively if missing) and loads one
// "FIELDNAME TYPE" entry per line into Db->FieldTypes.
void
FGDCSITE::LoadFieldTable() {
  STRLIST StrList;
  STRING  FieldTypeFilename;
  STRING  sBuf;
  CHR    *b, *pBuf;
  STRING  Field_and_Type;

  Db->GetDocTypeOptions(&StrList);
  StrList.GetValue("FIELDTYPE", &FieldTypeFilename);

  if (!IsFile(FieldTypeFilename)) {
    CHR buf[255];
    cout << "Specified fieldtype file was "
	 << "not found, but is required for this doctype." << endl;
    cout << "Please enter a new filename: ";
    cin.getline(buf,sizeof(buf));
    FieldTypeFilename = buf;
    if (!IsFile(FieldTypeFilename)) {
      cout << "Assuming all fields are text." << endl;
      cout << "Make sure you use the correct doctype option:" << endl;
      cout << endl;
      cout << "    -o fieldtype=<filename>" << endl;
      return;
    }
  }

  sBuf.ReadFile(FieldTypeFilename);
  b = sBuf.NewCString();

  pBuf = strtok(b,"\n");

  // BUGFIX #2 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): same bug as,
  // and fixed the same way as, doctype/cipc.cxx's BUGFIX #5 -- this
  // was a do-while, unconditionally running the body (and thus
  // `Field_and_Type = pBuf;`) once before ever checking pBuf. If the
  // FIELDTYPE file exists but is empty (IsFile() above only checks
  // existence, not content), strtok() returns nullptr on the very
  // first call, and STRING::operator=(const CHR*) calls strlen() on
  // it unconditionally -- a null-pointer-dereference crash. Checking
  // pBuf before the first iteration too, not just between iterations,
  // fixes it.
  while (pBuf) {
    Field_and_Type = pBuf;
    Field_and_Type.UpperCase();
    Db->FieldTypes.AddEntry(Field_and_Type);
    pBuf = strtok((CHR*)nullptr,"\n");
  }

  delete [] b;
}


FGDCSITE::~FGDCSITE() 
{
}


// True for the fixed set of site-locator field names (TITLE, HOSTNAME,
// bounding coordinates, contact info, etc.) that are worth indexing as
// search fields; false for anything else.
GDT_BOOLEAN
FGDCSITE::UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  if (FieldName.Search("TITLE"))
    return GDT_TRUE;
  else if (FieldName.Search("TITLE-ABBREVIATED"))
    return GDT_TRUE;
  else if (FieldName.Search("ABSTRACT"))
    return GDT_TRUE;
  else if (FieldName.Search("COST-INFORMATION"))
    return GDT_TRUE;
  else if (FieldName.Search("HOSTNAME"))
    return GDT_TRUE;
  else if (FieldName.Search("TCPPORT"))
    return GDT_TRUE;
  else if (FieldName.Search("DBNAME"))
    return GDT_TRUE;
  else if (FieldName.Search("IRL"))
    return GDT_TRUE;
  else if (FieldName.Search("SERVER-LAT"))
    return GDT_TRUE;
  else if (FieldName.Search("SERVER-LONG"))
    return GDT_TRUE;
  else if (FieldName.Search("WESTBC"))
    return GDT_TRUE;
  else if (FieldName.Search("EASTBC"))
    return GDT_TRUE;
  else if (FieldName.Search("NORTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("SOUTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-NAME"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-ORGANIZATION"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-STREET-ADDRESS"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-CITY"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-STATE-OR-PROVINCE"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-ZIP-OR-POSTAL-CODE"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-COUNTRY"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-NETWORK-ADDRESS"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-HOURS-OF-SERVICE"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-TELEPHONE"))
    return GDT_TRUE;
  else if (FieldName.Search("CONTACT-FAX"))
    return GDT_TRUE;
  else
    return GDT_FALSE;
}


// ElementSet "B" returns just the TITLE field; anything else returns
// the raw contents of the record's underlying file, trying several
// filename-extension variants in turn (RecordSyntax's own extension,
// then its short form, then uppercase long/short forms) before giving
// up and reporting the file as not found.
void
FGDCSITE::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING *StringBuffer)
{
  STRING FieldName;
  
  if (ElementSet.Equals("B")) {
    STRLIST Strlist;
    STRING Title;
    GDT_BOOLEAN Status;
    FieldName = "TITLE";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "(title not found)";
    *StringBuffer = Title;
  } else {
    STRING FullFilename, HoldFilename;
    STRING b;
    INT n;
    STRLIST StrList;
    STRING mpCommand;

    ResultRecord.GetFullFileName(&FullFilename);
    HoldFilename = FullFilename;

    n = FullFilename.SearchReverse('.');
    FullFilename.EraseAfter(n);

    if (RecordSyntax.Equals(HtmlRecordSyntax))
      FullFilename.Cat(FGDC_HTML_EXTENSION);  // extension=".html"
    // BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): this branch's
    // condition used to duplicate the HtmlRecordSyntax check above
    // verbatim, making it permanently unreachable (if the first branch
    // didn't match, neither could this identical one) -- an SGML
    // request always fell through to the final `else`, silently using
    // the unmodified filename instead of appending FGDC_SGML_EXTENSION.
    // That constant (and its short/uppercase siblings, all defined in
    // fgdcsite.hxx) was otherwise unused anywhere in this file --
    // strong evidence this SGML branch was intended but the condition
    // was copy-pasted wrong. Fixed by checking SgmlRecordSyntax instead
    // (declared in src/defs.hxx, alongside HtmlRecordSyntax/
    // SutrsRecordSyntax already used correctly here).
    else if (RecordSyntax.Equals(SgmlRecordSyntax))
      FullFilename.Cat(FGDC_SGML_EXTENSION);  // extension=".sgml"
    else if (RecordSyntax.Equals(SutrsRecordSyntax))
      FullFilename.Cat(FGDC_TEXT_EXTENSION);  // extension=".text"
    else
      FullFilename=HoldFilename; // Just use the filename in the result

    if (IsFile(FullFilename)) {
      // The file is there - read it in
      b.ReadFile(FullFilename);

    } else {
      // Hmmm...  We didn't find the requested file
      // Let's try sticking on the short extensions instead
      n = FullFilename.SearchReverse('.');
      FullFilename.EraseAfter(n);

      if (RecordSyntax.Equals(HtmlRecordSyntax))
	FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION);  // extension=".htm"
      // BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): same
      // duplicated-condition bug as above, in the short-extension
      // fallback.
      else if (RecordSyntax.Equals(SgmlRecordSyntax))
	FullFilename.Cat(SHORT_FGDC_SGML_EXTENSION);  // extension=".sgm"
      else if (RecordSyntax.Equals(SutrsRecordSyntax))
	FullFilename.Cat(SHORT_FGDC_TEXT_EXTENSION);  // extension=".txt"
      else
	FullFilename=HoldFilename; // Just use the filename in the result
      
      if (IsFile(FullFilename)) {
	// The file is there - read it in
	b.ReadFile(FullFilename);

      } else {
	// Hmmm...  We didn't find that one either
	// Let's try upper case extensions
	FullFilename.EraseAfter(n);

	if (RecordSyntax.Equals(HtmlRecordSyntax))
	  FullFilename.Cat(FGDC_HTML_EXTENSION_UC);  // extension=".HTML"
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): same
	// duplicated-condition bug as above, in the uppercase-extension
	// fallback.
	else if (RecordSyntax.Equals(SgmlRecordSyntax))
	  FullFilename.Cat(FGDC_SGML_EXTENSION_UC);  // extension=".SGML"
	else if (RecordSyntax.Equals(SutrsRecordSyntax))
	  FullFilename.Cat(FGDC_TEXT_EXTENSION_UC);  // extension=".TEXT"
	else
	  FullFilename=HoldFilename; // Just use the filename in the result
      
	if (IsFile(FullFilename)) {
	  // The file is there - read it in
	  b.ReadFile(FullFilename);

	} else {
	  // Nope - one more possibility
	  FullFilename.EraseAfter(n);

	  if (RecordSyntax.Equals(HtmlRecordSyntax))
	    FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION_UC); // extension=".HTM"
	  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): same
	  // duplicated-condition bug as above, in the short-uppercase-
	  // extension fallback.
	  else if (RecordSyntax.Equals(SgmlRecordSyntax))
	    FullFilename.Cat(SHORT_FGDC_SGML_EXTENSION_UC); // extension=".SGM"
	  else if (RecordSyntax.Equals(SutrsRecordSyntax))
	    FullFilename.Cat(SHORT_FGDC_TEXT_EXTENSION_UC); // extension=".TXT"
	  else
	    FullFilename=HoldFilename; // Just use the filename in the result
	  
	  if (IsFile(FullFilename)) {
	    // The file is there - read it in
	    b.ReadFile(FullFilename);

	  } else {
	      // Not defined, so bail out
	      *StringBuffer = "Requested file not found";
	      return;
	  }
	}
      }
    }

    if (b.GetLength() <= 0)
      *StringBuffer = "";
    else
      *StringBuffer = b;
  }
  return;
  
}

