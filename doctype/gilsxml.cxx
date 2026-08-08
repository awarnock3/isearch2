// $Id: gilsxml.cxx,v 1.4 2000/02/04 22:47:49 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

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

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		gilsxml.cxx
Version:	$Revision: 1.4 $
Description:	Class GILSXML - for XML-tagged GILS records
Derived from:   Class SGMLTAG - SGML-like Document Type
Author:		Archie Warnock (awww@home.com), A/WWW Enterprises
Originally by:  Kevin Gamiel, Kevin.Gamiel@cnidr.org
@@@*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "isearch.hxx"
#include "sgmltag.hxx"
#include "gilsxml.hxx"

void CleanString(STRING& Buffer);

GILSXML::GILSXML(IDBOBJ *DbParent) 
  : SGMLTAG(DbParent) {
}

// Every field is searchable for GILSXML.
GDT_BOOLEAN
GILSXML::UsefulSearchField(const STRING& /* Field */)
{
  return GDT_TRUE;
}

void 
GILSXML::Present (const RESULT& ResultRecord, const STRING& ElementSet,
		  STRING *StringBuffer)
{
  STRING RecSyntax="SUTRS";
  Present(ResultRecord,ElementSet,RecSyntax,StringBuffer);
  return;
}


// Dispatches by ElementSet, then by RecordSyntax: "B" returns just the
// title (falling back to the filename); "G"/"S"/"F" each pick one of
// Present_<HTML|SGML|SUTRS>_<G|S|F>() below, where "G" is the brief
// primitive set, "S" is the full record minus its <CENTROID> section,
// and "F" is the full record unabridged. Anything else is treated as
// a literal field name to look up.
void
GILSXML::Present (const RESULT& ResultRecord, const STRING& ElementSet,
		  const STRING& RecordSyntax, STRING *StringBuffer) {
  // Removed here: ESN_G, FieldValue, FieldType, Hold, and Status were
  // all declared but never used anywhere in this function (only
  // FieldName is) -- each Present_*_*() helper below has its own local
  // of the same name. GCC's -Wunused-variable only flagged `Status`
  // (a plain GDT_BOOLEAN); the STRING locals went unflagged since
  // STRING has a non-trivial constructor.
  STRING FieldName;

  //
  // If we get asked for B, ignore RecordSyntax and send back the 
  // buffer as SUTRS.  Note that GRS gets handled by the server so
  // it can attach the tag path
  //
  if (ElementSet.CaseEquals("B")) {
    FieldName = "title"; // Brief headline is title
    DOCTYPE::Present (ResultRecord, FieldName, StringBuffer);
    // send back the file name if no title is available
    if (*StringBuffer == "") {
      ResultRecord.GetFileName(StringBuffer);
    }
  } else if (ElementSet.CaseEquals("G")) {
    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      Present_HTML_G(ResultRecord, StringBuffer);
    } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
      Present_SGML_G(ResultRecord, StringBuffer);
    } else {
      Present_SUTRS_G(ResultRecord, StringBuffer);
    }

  } else if (ElementSet.CaseEquals("S")) {
    // We invented the S element set to send full records without the
	 // centroid - S stands for "short" or "summary"
    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      Present_HTML_S(ResultRecord, StringBuffer);
    } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
      Present_SGML_S(ResultRecord, StringBuffer);
    } else {
      Present_SUTRS_S(ResultRecord, StringBuffer);
    }

  } else if (ElementSet.CaseEquals("F")) {
    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      Present_HTML_F(ResultRecord, StringBuffer);
    } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
      Present_SGML_F(ResultRecord, StringBuffer);
    } else {
#ifdef ASF
       Present_HTML_S(ResultRecord, StringBuffer);
#else
       Present_SUTRS_F(ResultRecord, StringBuffer);
#endif
    }

  } else {
    // ElementSet not recognized, must be a file name.  Since we cannot 
    // tell what it is, ignore RecordSyntax again and send back SUTRS
    FieldName = ElementSet;
    DOCTYPE::Present (ResultRecord, FieldName, StringBuffer);
  }
	
  return;
}


GILSXML::~GILSXML() {
}


void
CleanString(STRING& Buffer)
{
  Buffer.Replace("\n"," ");
  Buffer.Replace("\r"," ");
}


/* This method creates the G element set as an HTML document */
void 
GILSXML::Present_HTML_G(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING ESN_G, FieldName, FieldValue, Hold;
  STRING FieldType="TEXT";
  STRING Title;
  GDT_BOOLEAN Status;

  // The primitive element set name 'G' contains at least 
       // Title, Control Identifier, Originator, Local Control Number 
       // and Cross Reference
		
       FieldName = "TITLE";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  // Set up the start of the HTML document
  Hold = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n";
  Hold.Cat("<HTML>\n<HEAD>\n");

  // Fill the title tag
  Hold.Cat("<TITLE>");
  if (Status) {
    CleanString(FieldValue);
    Title = FieldValue;
  } else {
    ResultRecord.GetFileName(&Title);
  }
  Hold.Cat(Title);
  Hold.Cat("</TITLE>\n");

  // Some META tags could go here if desired
  Hold.Cat("</HEAD>\n");
  Hold.Cat("<BODY BGCOLOR=\"#ffffff\" TEXT=\"#000000\">\n");

  // Now set up the start of the document

  Hold.Cat("<H1>");
  Hold.Cat(Title);
  Hold.Cat("</H1>\n");

  ESN_G.Cat(Hold);
  
  FieldName = "CONTROL-IDENTIFIER";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<p><i>CONTROL-IDENTIFIER:</i>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</p>\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "ORIGINATOR";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<p><i>ORIGINATOR:</i>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</p>\n");
  ESN_G.Cat(Hold);
  
		
  FieldName = "LOCAL-CONTROL-NUMBER";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<p><i>LOCAL-CONTROL-NUMBER:</i>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</p>\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "CROSS-REFERENCE";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<p><i>CROSS-REFERENCE:</i>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</p>\n");
  ESN_G.Cat(Hold);
  ESN_G.Cat("</BODY>\n</HTML>\n");

  *StringBuffer = ESN_G;
}


/* This method creates the G element set as an XML document - note we
 * use the official SGML OID to denote XML
 */
void 
GILSXML::Present_SGML_G(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING ESN_G, FieldName, FieldValue, Hold;
  STRING FieldType="TEXT";
  STRING Title;
  GDT_BOOLEAN Status;

  // The primitive element set name 'G' contains at least 
       // Title, Control Identifier, Originator, Local Control Number 
       // and Cross Reference
		
       FieldName = "TITLE";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  // Set up the start of the HTML document

  Hold = "<?XML VERSION=\"1.0\" ENCODING=\"UTF-8\" ?>\n";
  Hold.Cat("<!DOCTYPE Locator SYSTEM \"xml.dtd\" >\n");
  Hold.Cat("<Locator>\n\n");

  // Fill the title tag
  Hold.Cat("<TITLE>");
  if (Status) {
    CleanString(FieldValue);
    Title = FieldValue;
  } else {
    ResultRecord.GetFileName(&Title);
  }
  Hold.Cat(Title);
  Hold.Cat("</TITLE>\n");

  ESN_G.Cat(Hold);
  
  FieldName = "CONTROL-IDENTIFIER";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<CONTROL-IDENTIFIER>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</CONTROL-IDENTIFIER>\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "ORIGINATOR";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<ORIGINATOR>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</ORIGINATOR>\n");
  ESN_G.Cat(Hold);
  
		
  FieldName = "LOCAL-CONTROL-NUMBER";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<LOCAL-CONTROL-NUMBER>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</LOCAL-CONTROL-NUMBER>\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "CROSS-REFERENCE";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "<CROSS-REFERENCE>";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("</CROSS-REFERENCE>\n");
  ESN_G.Cat(Hold);
  ESN_G.Cat("</LOCATOR>");

  *StringBuffer = ESN_G;
}


/* This method creates the G element set as a SUTRS record */
void 
GILSXML::Present_SUTRS_G(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING ESN_G, FieldName, FieldValue, Hold;
  STRING FieldType="TEXT";
  GDT_BOOLEAN Status;
  // The primitive element set name 'G' contains at least 
       // Title, Control Identifier, Originator, Local Control Number 
       // and Cross Reference
		
       FieldName = "TITLE";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "TITLE:";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("\n");
  ESN_G.Cat(Hold);
  
		
  FieldName = "CONTROL-IDENTIFIER";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "CONTROL-IDENTIFIER:";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "ORIGINATOR";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "ORIGINATOR:";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("\n");
  ESN_G.Cat(Hold);
  
		
  FieldName = "LOCAL-CONTROL-NUMBER";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "LOCAL-CONTROL-NUMBER:";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("\n");
  ESN_G.Cat(Hold);
  
  
  FieldName = "CROSS-REFERENCE";
  //    Db->FieldTypes.GetValue(FieldName,&FieldType);
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  Hold = "CROSS-REFERENCE:";
  if (Status) {
    CleanString(FieldValue);
    Hold.Cat(FieldValue);
  } else {
    Hold.Cat(" (not available)");
  }
  Hold.Cat("\n");
  ESN_G.Cat(Hold);
  
  *StringBuffer = ESN_G;
}


/* These produce the formatted results for the S (short) element set */
void 
GILSXML::Present_HTML_S(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING ESN_F, FieldName, FieldValue, Hold;
  STRING FieldType="TEXT";
  STRING Title;
  GDT_BOOLEAN Status;
  STRING FileName;

  // The primitive element set name 'G' contains at least 
       // Title, Control Identifier, Originator, Local Control Number 
       // and Cross Reference
		
  FieldName = "TITLE";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  // Set up the start of the HTML document
  Hold = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n";
  Hold.Cat("<HTML>\n<HEAD>\n");

  // Fill the title tag
  Hold.Cat("<TITLE>");
  if (Status) {
    CleanString(FieldValue);
    Title = FieldValue;
  } else {
    ResultRecord.GetFileName(&Title);
  }
  Hold.Cat(Title);
  Hold.Cat("</TITLE>\n");

  // Some META tags could go here if desired
  Hold.Cat("</HEAD>\n");
  Hold.Cat("<BODY BGCOLOR=\"#ffffff\" TEXT=\"#000000\">\n");

  // Now set up the start of the document

  Hold.Cat("<H1>");
  Hold.Cat(Title);
  Hold.Cat("</H1>\n");

  ESN_F.Cat(Hold);

  ESN_F.Cat("<pre>\n");

  ResultRecord.GetFullFileName(&FileName);

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypegilsxmlcxx): Hold still held
  // the header text just transferred into ESN_F above (it was never
  // reset), so accumulating the file's non-centroid lines into it via
  // Cat() appended them onto that stale header instead of starting
  // fresh -- the header text then got duplicated into the output a
  // second time (this time HTML-escaped) by the Replace()+Cat() below.
  // doctype/gilsxml.cxx's own Present_HTML_F() avoids this by using
  // Hold.ReadFile() (which replaces Hold's content) instead of Cat()
  // in a loop; Present_SGML_S() avoids it by using a Hold that was
  // never assigned anything beforehand. Fixed by resetting Hold here,
  // right before it's reused as the body accumulator.
  Hold = "";

  // Grotesque hack to skip <centroid> listing
  CHR *cHold,*ptr;
  STRING FileBuffer;
  GDT_BOOLEAN InCentroid=GDT_FALSE;

  FileBuffer.ReadFile(FileName);
  cHold = FileBuffer.NewCString();

  ptr = strtok(cHold,"\n");
  while(ptr) {
    if (!StrCaseCmp(ptr,"<CENTROID>")) {
      InCentroid=GDT_TRUE;
    } else if (!StrCaseCmp(ptr,"</CENTROID>")) {
      InCentroid=GDT_FALSE;
    }
    if (!InCentroid) {
      Hold.Cat(ptr);
      Hold.Cat("\n");
    }
    ptr = strtok(nullptr,"\n");
  }

  delete [] cHold;

  Hold.Replace("<","&lt;");
  Hold.Replace(">","&gt;");

  ESN_F.Cat(Hold);
  ESN_F.Cat("</pre>\n");

  ESN_F.Cat("</BODY>\n</HTML>\n");

  *StringBuffer = ESN_F;
}


void 
GILSXML::Present_SGML_S(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING Hold, FileName;
  ResultRecord.GetFullFileName(&FileName);

  // Grotesque hack to skip <centroid> listing
  CHR *cHold,*ptr;
  STRING FileBuffer;
  GDT_BOOLEAN InCentroid=GDT_FALSE;

  FileBuffer.ReadFile(FileName);
  cHold = FileBuffer.NewCString();

  ptr = strtok(cHold,"\n");
  while(ptr) {
    if (!StrCaseCmp(ptr,"<CENTROID>")) {
      InCentroid=GDT_TRUE;
    } else if (!StrCaseCmp(ptr,"</CENTROID>")) {
      InCentroid=GDT_FALSE;
    }
    if (!InCentroid) {
      Hold.Cat(ptr);
      Hold.Cat("\n");
    }
    ptr = strtok(nullptr,"\n");
  }

  delete [] cHold;

  *StringBuffer = Hold;
}


void
GILSXML::Present_SUTRS_S(const RESULT& ResultRecord, STRING *StringBuffer)
{
  // BUGFIX #2 (docs/BUG_CATALOG.md#doctypegilsxmlcxx): this delegated
  // to Present_SGML_F() (the *full* record, centroid included) instead
  // of Present_SGML_S() (the *short* record, centroid stripped) --
  // "S" stands for "short"/"summary" per Present()'s own comment
  // ("We invented the S element set to send full records without the
  // centroid"), so the SUTRS "S" element set was silently returning
  // the same content as its "F" sibling instead of omitting the
  // centroid like Present_HTML_S()/Present_SGML_S() both correctly do.
  // A plain copy-paste from Present_SUTRS_F() just below, whose
  // identical delegation is correct there.
  Present_SGML_S(ResultRecord, StringBuffer);
}


/* These produce the formatted results for the F (full record) element set */
void 
GILSXML::Present_HTML_F(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING ESN_F, FieldName, FieldValue, Hold;
  STRING FieldType="TEXT";
  STRING Title;
  GDT_BOOLEAN Status;
  STRING FileName;

  // The primitive element set name 'G' contains at least 
       // Title, Control Identifier, Originator, Local Control Number 
       // and Cross Reference
		
       FieldName = "TITLE";
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, 
			    &FieldValue);
  // Set up the start of the HTML document
  Hold = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n";
  Hold.Cat("<HTML>\n<HEAD>\n");

  // Fill the title tag
  Hold.Cat("<TITLE>");
  if (Status) {
    CleanString(FieldValue);
    Title = FieldValue;
  } else {
    ResultRecord.GetFileName(&Title);
  }
  Hold.Cat(Title);
  Hold.Cat("</TITLE>\n");

  // Some META tags could go here if desired
  Hold.Cat("</HEAD>\n");
  Hold.Cat("<BODY BGCOLOR=\"#ffffff\" TEXT=\"#000000\">\n");

  // Now set up the start of the document

  Hold.Cat("<H1>");
  Hold.Cat(Title);
  Hold.Cat("</H1>\n");

  ESN_F.Cat(Hold);

  ESN_F.Cat("<pre>\n");

  ResultRecord.GetFullFileName(&FileName);
  Hold.ReadFile(FileName);

  Hold.Replace("<","&lt;");
  Hold.Replace(">","&gt;");

  ESN_F.Cat(Hold);
  ESN_F.Cat("</pre>\n");

  ESN_F.Cat("</BODY>\n</HTML>\n");

  *StringBuffer = ESN_F;
}


void 
GILSXML::Present_SGML_F(const RESULT& ResultRecord, STRING *StringBuffer)
{
  STRING Hold, FileName;
  ResultRecord.GetFullFileName(&FileName);

  Hold.ReadFile(FileName);
  *StringBuffer = Hold;
}


void 
GILSXML::Present_SUTRS_F(const RESULT& ResultRecord, STRING *StringBuffer)
{
  // Do the obvious
  Present_SGML_F(ResultRecord, StringBuffer);
}

