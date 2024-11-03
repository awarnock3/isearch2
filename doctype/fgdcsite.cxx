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

#include <iostream.h>
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

  do {
    Field_and_Type = pBuf;
    Field_and_Type.UpperCase();
    Db->FieldTypes.AddEntry(Field_and_Type);
  } while ( (pBuf = strtok((CHR*)NULL,"\n")) );

  delete [] b;
}


FGDCSITE::~FGDCSITE() 
{
}


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
    else if (RecordSyntax.Equals(HtmlRecordSyntax))
      FullFilename.Cat(FGDC_HTML_EXTENSION);  // extension=".html"
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
      else if (RecordSyntax.Equals(HtmlRecordSyntax))
	FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION);  // extension=".htm"
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
	else if (RecordSyntax.Equals(HtmlRecordSyntax))
	  FullFilename.Cat(FGDC_HTML_EXTENSION_UC);  // extension=".HTML"
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
	  else if (RecordSyntax.Equals(HtmlRecordSyntax))
	    FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION_UC); // extension=".HTM"
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

