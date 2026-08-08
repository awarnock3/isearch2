// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/* $Id: anzlic.cxx,v 1.6 2000/02/04 22:49:32 cnidr Exp $ */
/************************************************************************
Copyright (c) 1994,1995 Basis Systeme netzwerk, Munich
              Brecherspitzstr. 8
              D-81541 Munich, Germany

              ISRCH-LIC-1B EXPORT: Tue Aug 15 14:20:42 MET DST 1995

              Public Software License Agreement:
              ----------------------------------

Basis Systeme netzwerk(*) (herein after referred to as BSn) hereby
provides COMPANY (herein after referred to as "Licensee") with a
non-exclusive, royalty-free, worldwide license to use, reproduce, modify
and redistribute this software and its documentation (hereafter referred
to as "Materials"), in whole or in part, with Licensee's products under
the following conditions:

1. All copyrights and restrictions in the source files of the Software
Materials will be honored.

2. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

3. The origin of these Materials will be explicitly stated in Licensee's
accompanying documentation as developed by Basis Systeme netzwerk (BSn)
and its collaborators.

4. The name of the author(s) or BSn may not be used to endorse or
promote products derived from these Materials without specific prior
written permission.

5. Versions of the Software Materials or documentation that are altered
or changed will be marked as such.

6. Licensee shall make reasonable efforts to provide BSn with all
enhancements and modifications made by Licensee to the Software
Materials, for a period of three years from the date of execution of
this License. BSn shall have the right to use and/or redistribute the
modifications and enhancements without accounting to Licensee.

Enhancements and Modifications shall be defined as follows:
    i) Changes to the source code, support files or documentation.
   ii) Documentation directly related to Licensee's distribution of the 
       software.
  iii) Licensee software modules that actively solicit services from
       the software and accompanying user documentation.

7. Users of this software agree to make their best efforts to inform BSn
of noteworthy uses of this software.

8. You agree that neither you, nor your customers, intend to, or will,
export these Materials to any country which such export or transmission
is restricted by applicable law without prior written consent of the
appropriate government agency with jurisdiction over such export or
transmission.

8. BSn makes no representation on the suitability of the Software
Materials for any purpose.  The SOFTWARE IS PROVIDED "AS IS" AND WITHOUT
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.

9. Licensee agrees to indemnify, defend and hold harmless BSn from any
loss, claim, damage or liability of any kind, including attorney's fees
and court costs, arising out of or in connection with any use of the
Materials under this License.

10. In no event shall BSn be liable to Licensee or third parties
licensed by licensee for any indirect, special, incidental, or
consequential damages (including lost profits).

11. BSn has no knowledge of any conditions that would impair its right
to license the Materials.  Notwithstanding the foregoing, BSn does not
make any warranties or representations that the Materials are free of
claims by third parties of patent, copyright infringement or the like,
nor does BSn assume any liability in respect of any such infringement of
rights of third parties due to Licensee operation under this license.

12. IN NO EVENT SHALL BSN OR THE AUTHORS BE LIABLE FOR ANY SPECIAL,
INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

13. The place of execution of this agreement is Munich and the
applicable laws are those of the Federal Republic of Germany. The
agreement also remains in force even in states/jurisdictions that
exclude one or more clauses. For these cases the applicable clauses are
to be replaced by other agreements that come as close as possible to the
original intent.

"Diese Vereinbarung unterliegt dem Recht der Bundesrepublik Deutschland.
Sie enthaelt saemtliche Vereinbarungen, welche die Parteien hinsichtlich
des Vereinbarungsgegenstandes getroffen haben, und ersetzt alle
vorhergehenden muendlichen oder schriftlichen Abreden. Diese
Vereinbarung bleibt in Zweifel auch bei rechtlicher Unwirksamkeit
enzelner Bestimmungen in seinen uebrigen Teilen verbindlich. Unwirksame
Bestimmungen sind durch Regulungen zu ersetzen, die dem angestrebten
Erfolg moeglichst nahe kommen."

______________________________________________________________________________
(*)Basis Systeme netzwerk, Brecherspitzstr. 8, 81541 Muenchen, Germany 

************************************************************************/
/*-@@@
File:		anzlic_string.cxx
Version:	$Revision: 1.6 $
Description:	Class ANZLIC - ANZLIC Document Type
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and USGS/ANZLIC
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "isearch.hxx"
#include "sgmlnorm.hxx"
#include "anzlic.hxx"

/* ------- ANZLIC Support --------------------------------------------- */

void ANZLIC::LoadFieldTable() {
  STRLIST StrList;
  STRING FieldTypeFilename;
  Db->GetDocTypeOptions(&StrList);
  StrList.GetValue("FIELDTYPE", &FieldTypeFilename);

  if (FieldTypeFilename.GetLength() == 0) {
    cout << "No fieldtype file specified.  Assuming all fields are text.";
    cout << endl;
    cout << "Make sure you use the correct doctype option:" << endl;
    cout << endl;
    cout << "    -o fieldtype=<filename>" << endl;
    return;
  }

  STRING Field_and_Type;
  CHR *b, *pBuf;
  INT4 RecStart, RecEnd, len, ActualLength;
  PFILE fp = fopen(FieldTypeFilename, "r");

  // Let's bring the entire file into memory
  if (!fp) {
    cout << "Specified fieldtype file not found.  Assuming all fields are text.";
    cout << endl;
    cout << "Make sure you use the correct doctype option:" << endl;
    cout << endl;
    cout << "    -o fieldtype=<filename>" << endl;
    return;
  }
  fseek(fp, 0L, SEEK_END);
  RecStart = 0;
  RecEnd = ftell(fp);

  fseek(fp, (long)RecStart, SEEK_SET);
  len = RecEnd - RecStart;
  b = new CHR[len + 1];
  ActualLength = fread(b, 1, len, fp);
  b[ActualLength] = '\0';
  fclose(fp);
  pBuf = strtok(b,"\n");

  do {
    Field_and_Type = pBuf;
    Field_and_Type.UpperCase();
    Db->FieldTypes.AddEntry(Field_and_Type);
  } while ( (pBuf = strtok((CHR*)nullptr,"\n")) );

  delete [] b;
}

GDT_BOOLEAN ANZLIC:: UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  /* This list changed to match GEO2.1a
     - Ben Hatton August 1997 */

  if (FieldName.Search("TITLE"))
    return GDT_TRUE;
  else if (FieldName.Search("ORIGIN"))
    return GDT_TRUE;
  else if (FieldName.Search("ABSTRACT"))
    return GDT_TRUE;
  else if (FieldName.Search("CUSTOD"))
    return GDT_TRUE;
  else if (FieldName.Search("THEMEKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("AVLFORM"))
    return GDT_TRUE;
  else if (FieldName.Search("BOUNDING"))
    return GDT_TRUE;
  else if (FieldName.Search("WESTBC"))
    return GDT_TRUE;
  else if (FieldName.Search("EASTBC"))
    return GDT_TRUE;
  else if (FieldName.Search("NORTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("SOUTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("BEGDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("ENDDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("PLACEKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("PROGRESS"))
    return GDT_TRUE;
  else if (FieldName.Search("METD"))
    return GDT_TRUE;
  else
    return GDT_FALSE;
}

ANZLIC::ANZLIC (PIDBOBJ DbParent) : SGMLNORM (DbParent)
{
}

void ANZLIC::ParseRecords (const RECORD& FileRecord)
{
  SGMLNORM::ParseRecords (FileRecord);
}

DOUBLE ANZLIC::ParseDateSingle(const PCHR Buffer) {
  DOUBLE fVal;
  STRING Hold;

  Hold = Buffer;

  if (Hold.CaseEquals("present"))
    fVal = 99999999;
  else if (Hold.CaseEquals("unknown"))
    fVal = -1.0;
  else if (Hold.IsNumber())
    fVal = Hold.GetFloat();
  else {
    cout << "Bad date, value=" << Buffer << endl;
    fVal = -1.0;
  }
  return fVal;
}

void ANZLIC::ParseDateRange(const PCHR Buffer, DOUBLE* fStart, 
			  DOUBLE* fEnd){
  PCHR found;
  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;

  Hold = Buffer;
  Hold.UpperCase();
  Start = Hold.Search("<BEGDATE>");
  Start += 9;
  Hold.EraseBefore(Start);
  End = Hold.Search("</BEGDATE");
  Hold.EraseAfter(End-1);
  if (Hold.CaseEquals("present")
      || Hold.CaseEquals("9999")
      || Hold.CaseEquals("999999")
      || Hold.CaseEquals("99999999")) {
    *fStart = -1;
  } else if (Hold.CaseEquals("unknown")) {
    *fStart = -1.0;
  } else if (Hold.IsNumber()) {
    *fStart = Hold.GetFloat();
  } else {
    cout << "Bad Start date, value=" << Buffer << endl;
    *fStart = -1.0;
  }

  Hold = Buffer;
  Hold.UpperCase();
  Start = Hold.Search("<ENDDATE>");
  Start += 9;
  Hold.EraseBefore(Start);
  End = Hold.Search("</ENDDATE");
  Hold.EraseAfter(End-1);
  if (Hold.CaseEquals("present")) {
    *fEnd = 99999999;
  } else if (Hold.CaseEquals("unknown")) {
    *fEnd = -1.0;
  } else if (Hold.IsNumber()) {
    *fEnd = Hold.GetFloat();
  } else {
    cout << "Bad End Date, value=" << Buffer << endl;
    *fEnd = 99999999;
  }

  return;
}

void ANZLIC::ParseFields (PRECORD NewRecord)
{
  PFILE fp;
  STRING fn;

  if (NewRecord == (PRECORD)nullptr) return; // Error

  // Open the file
  NewRecord->GetFullFileName (&fn);
  if (!(fp = fopen (fn, "rb")))
    return;			// ERROR

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0) {
    fseek (fp, 0L, SEEK_END);
    RecStart = 0;
    RecEnd = ftell (fp);
  }
  fseek (fp, (long)RecStart, SEEK_SET);

  // Read the whole record in a buffer
  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = new CHR[RecLength + 1];
  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  fclose (fp);

  STRING FieldName;
  STRING FullFieldname;

  FC fc, fc_full;
  DF df, df_full;
  DFD dfd, dfd_full;
  STRING doctype;
  NUMERICFLD nc, nc_full;

  FullFieldname = "";
  NewRecord->GetDocumentType(&doctype);

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == nullptr) {
    cout << "Unable to parse `" << doctype << "' tags in file " << fn << "\n";
    // Clean up
    delete [] RecBuffer;
    return;
  }

  GSTACK Nested;
  size_t LastEnd;
  PAMD_Element pCurrentTag;
  PDFT pdft = new DFT ();
  GDT_BOOLEAN InCustom;
  size_t val_start;
  int val_len;
  size_t val_end;

  InCustom = GDT_FALSE;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++) {
    /*
    size_t val_start;
    int val_len;
    size_t val_end;
    */

    if ((*tags_ptr)[0] == '/') {
      PAMD_Element pTmp;
      // BUGFIX #1: was `strcmp(*tags_ptr,"/custom")` without negation - always true (non-zero result)
      if (!strcmp(*tags_ptr,"/custom")) {

	STRING Tag;
	STRINGINDEX x;
	PCHR cx;

	Tag = *tags_ptr;
	x=Tag.Search('/');
	Tag.EraseBefore(x+1);
      
	pTmp = (PAMD_Element)Nested.Top();
	if (Tag == pTmp->get_tag()) {
	  pTmp = (PAMD_Element)Nested.Pop();
//	  cout << "Popped " << pTmp->get_tag() << " off the stack.  ";
	  delete pTmp;
	  if (Nested.GetSize() != 0) {
	    pTmp = (PAMD_Element)Nested.Top();
//	    cout << "Still inside " << pTmp->get_tag() << ".\n";
	    x = FullFieldname.SearchReverse('_');
	    FullFieldname.EraseAfter(x-1);
//	    cout << "Full fieldname is now " << FullFieldname << ".\n";
	  }
	}
      } else
	InCustom=GDT_FALSE;

      continue;
    }

    const PCHR p = find_end_tag (tags_ptr, *tags_ptr);
    size_t tag_len = strlen (*tags_ptr);
    int have_attribute_val = (nullptr != strchr (*tags_ptr, '='));

    if (p != nullptr) {
      // We have a tag pair
      val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
      val_len = (p - *tags_ptr) - tag_len - 2;

      // Skip leading white space
      while (isspace (RecBuffer[val_start]))
	val_start++, val_len--;
      // Leave off trailing white space
      while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	val_len--;

      // Don't bother storing empty fields
      if (val_len > 0) {
	// Cut the complex values from field name
	CHR orig_char = 0;
	PAMD_Element pTag = new AMD_Element();
	char* tcp;

	for (tcp = *tags_ptr; *tcp; tcp++) {
	  if (isspace (*tcp)) {
	    orig_char = *tcp;
	    *tcp = '\0';
	    break;
	  }
	}

	const CHR *unified_name = UnifiedName(*tags_ptr);
	// Ignore "unclassified" fields
	if (unified_name == nullptr) 
	  continue; // ignore these
	FieldName = unified_name;
	if (!(FieldName.IsPrint())) {
	  FieldName.MakePrintable();
	  cout << "Non-ascii characters found in " << FieldName << endl;
	}

	if (FieldName == "custom")
	  InCustom=GDT_TRUE;

	if (!InCustom) {
	  // Fieldname.UpperCase();
	  if (orig_char)
	    *tcp = orig_char;
	  
	  val_end = val_start + val_len - 1;
	  
	  pTag->set_tag(FieldName);
	  pTag->set_start(val_start);
	  pTag->set_end(val_end);
	  
	  if (Nested.GetSize() != 0) {
	    PAMD_Element pTmp;
	    if (val_start < LastEnd) {
	      pTmp = (PAMD_Element)Nested.Top();
	    }
	  }
	  if (FullFieldname.GetLength() > 0)
	    FullFieldname.Cat("_");
	  FullFieldname.Cat(FieldName);
	  if (!(FullFieldname.IsPrint())) {
	    FullFieldname.MakePrintable();
	    cout << "Non-ascii characters found in " << FullFieldname << endl;
	  }

	  STRING FieldType;
	
	  if(UsefulSearchField(FieldName)) {
	    FieldName.UpperCase();
	    Db->FieldTypes.GetValue(FieldName, &FieldType);

	    if (FieldType.Equals(""))
	      FieldType = "text";
	    dfd.SetFieldName (FieldName);
	    dfd.SetFieldType (FieldType);
	    Db->DfdtAddEntry (dfd);

	    fc.SetFieldStart (val_start);
	    fc.SetFieldEnd (val_end);
	    PFCT pfct = new FCT ();
	    pfct->AddEntry (fc);
	    df.SetFct (*pfct);
	    df.SetFieldName (FieldName);
	    pdft->AddEntry (df);
	    delete pfct;

	    // Now, do the same for the long fieldname
	    FullFieldname.UpperCase();
	    
	    STRING NewType;
	    NewType = FullFieldname;
	    NewType.Cat("=");
	    NewType.Cat(FieldType);
	    NewType.UpperCase();
	    Db->FieldTypes.AddEntry(NewType);

	    dfd_full.SetFieldName (FullFieldname);
	    dfd_full.SetFieldType (FieldType);
//	    cout << "Found " << FullFieldname << " of type " << FieldType
//	      << endl;
	    Db->DfdtAddEntry (dfd_full);
	    fc_full.SetFieldStart (val_start);
	    fc_full.SetFieldEnd (val_end);
	    PFCT pfct1 = new FCT ();
	    pfct1->AddEntry (fc_full);
	    df_full.SetFct (*pfct1);
	    df_full.SetFieldName (FullFieldname);
	    pdft->AddEntry (df_full);
	    delete pfct1;
	  }
	  Nested.Push(pTag);
	  LastEnd = val_end;
	}
      }
    }
    if (have_attribute_val) {
      SGMLNORM::store_attributes (pdft, RecBuffer, *tags_ptr);
    } else if (p == nullptr) {
#if 1
      // Give some information
      cout << doctype << " Warning: \""
	<< fn << "\" offset " << (*tags_ptr - RecBuffer) << ": "
	  << "No end tag for <" << *tags_ptr << "> found, skipping field.\n";
#endif
    }
  }
  
  NewRecord->SetDft (*pdft);
  
  // Clean up;
  delete [] tags;
  delete pdft;
  delete[]RecBuffer;
}

GDT_BOOLEAN ANZLIC::GetCleanedFieldData(const RESULT& ResultRecord, 
				const STRING& FieldName,
				const STRING& FieldType,
				STRING& Buffer)
{
  GDT_BOOLEAN Status;
  Status = Db->GetFieldData(ResultRecord, FieldName, FieldType, &Buffer);
  if (Status) {
    Buffer.Replace("\n"," ");
    Buffer.Replace("\r"," ");
  } else
    Buffer = "(not found)";
  return Status;
}

void 
ANZLIC::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING *StringBuffer)
{
  STRING FieldName;

// if ElementSet B - present TITLE
// if ElementSet A - present TITLE plus ABSTRACT
// if ElementSet S - present TITLE
// if ElementSet F - present full document (version depends on Record Syntax)

  //----- ElementSet B ----------------------------

  if (ElementSet.CaseEquals(BRIEF_MAGIC)) {
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
  }

  //----- ElementSet A ----------------------------

  else if (ElementSet.CaseEquals("A")) {
    STRLIST Strlist;
    STRING Title, Abstract;
    GDT_BOOLEAN Status;
    FieldName = "TITLE";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "(title not found)";

    //   if Record Syntax is HTML or XML
    //     then send HTML fragment with TITLE and ABSTRACT
    //     else send plain text TITLE (because ABSTRACT may hold HTML markup)
    //    if ((RecordSyntax.Equals(HTML_OID)) ||
    //        (RecordSyntax.Equals(NEW_HTML_OID)) ||
    //        (RecordSyntax.Equals(NEW2_HTML_OID)) ||
    //        (RecordSyntax.Equals(XML_OID))) {
    if ((RecordSyntax.CaseEquals(HtmlRecordSyntax)) ||
        (RecordSyntax.CaseEquals(XmlRecordSyntax))) {
      *StringBuffer = "<b>";
      *StringBuffer += Title;
      *StringBuffer += "</b>";
      FieldName = "ABSTRACT";
      Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
      if (Status) {
        // Note: the ANZMETA ABSTRACT allows max 2000 characters
        // so we may need to truncate this to a resonable presentation length
        Strlist.Join("\n",&Abstract);
        Abstract.Replace("\n"," ");
        Abstract.Replace("\r"," ");
        *StringBuffer += "<blockquote><small>";
        *StringBuffer += Abstract;
        *StringBuffer += "</small></blockquote>";
      }
    }
    else
      *StringBuffer = Title;
  }

  //----- ElementSet S ----------------------------

  else if (ElementSet.CaseEquals("S")) {
    // ANZMETA does not have all of the fields for the S element set
    // from the GEO profile, so we deliver only the elements that we can

   /*  These fields define the S element set
         Title                              ANZMETA
         Edition                            no
         Geospatial_Data_Presentation_Form  no
         Indirect_Spatial_Reference         no
         West_Bounding_Coordinate           ANZMETA
         East_Bounding_Coordinate           ANZMETA
         North_Bounding_Coordinate          ANZMETA
         South_Bounding_Coordinate          ANZMETA
         Beginning_Date                     ANZMETA (has extra)
         Ending_Date                        ANZMETA (has extra)
         Calendar_Date (need field name...) no
         Maintenance_and_Update_Frequency   ANZMETA
         Browse_Graphic_File_Name           no
   */
    STRLIST Strlist;
    STRING Hold, FieldType,ESN_S;
    STRING Title,Edition,GeoForm,Spatial,West,East,North,South;
    STRING BegDate,EndDate,CalDate,Update,BrowseGraphic;
    GDT_BOOLEAN Status;

    ESN_S = "";

    FieldName = "TITLE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
    if (Status) {
      Hold = "TITLE=";
      Hold.Cat(Title);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    } 

    FieldName = "WESTBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, West);
    if (Status) {
      Hold = "WESTBC=";
      Hold.Cat(West);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "EASTBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, East);
    if (Status) {
      Hold = "EASTBC=";
      Hold.Cat(East);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "NORTHBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, North);
    if (Status) {
      Hold = "NORTHBC=";
      Hold.Cat(North);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "SOUTHBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, South);
    if (Status) {
      Hold = "SOUTHBC=";
      Hold.Cat(South);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    // FieldName = "BEGDATE";
    // Db->FieldTypes.GetValue(FieldName,&FieldType);
    // Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, BegDate);
    // if (Status) {
      // Hold = "BEGDATE=";
      // Hold.Cat(BegDate);
      // Hold.Cat("\n");
      // ESN_S.Cat(Hold);
    // }

    // FieldName = "ENDDATE";
    // Db->FieldTypes.GetValue(FieldName,&FieldType);
    // Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, EndDate);
    // if (Status) {
      // Hold = "ENDDATE=";
      // Hold.Cat(EndDate);
      // Hold.Cat("\n");
      // ESN_S.Cat(Hold);
    // }

    FieldName = "ANZMETA_STATUS_UPDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Update);
    if (Status) {
      Hold = "UPDATE=";
      Hold.Cat(Update);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    *StringBuffer = ESN_S;
  }

  //----- ElementSet F ----------------------------

  else {
    STRING FullFilename, HoldFilename;
    STRING b;
    INT n;

    STRLIST StrList;
    Db->GetDocTypeOptions(&StrList);

    ResultRecord.GetFullFileName(&FullFilename);
    HoldFilename = FullFilename;

    n = FullFilename.SearchReverse('.');
    FullFilename.EraseAfter(n);

    //    if ((RecordSyntax.Equals(HTML_OID)) ||
    //        (RecordSyntax.Equals(NEW_HTML_OID)) ||
    //        (RecordSyntax.Equals(NEW2_HTML_OID)))
    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
      FullFilename.Cat(ANZLIC_HTML_EXTENSION);  // extension=".html"
      //    else if (RecordSyntax.Equals(XML_OID))
    else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
      FullFilename.Cat(ANZLIC_XML_EXTENSION);   // extension=".xml"
//    else if (RecordSyntax.Equals(SUTRS_OID))
    else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
      FullFilename.Cat(ANZLIC_TEXT_EXTENSION);  // extension=".txt"
    else
      FullFilename=HoldFilename; // Just use the file that was indexed

    if (IsFile(FullFilename)) {
      // The file is there - read it in
      b.ReadFile(FullFilename);

    } else {
      // Hmmm...  We didn't find the requested file
      // Let's try sticking on the short extensions instead
      n = FullFilename.SearchReverse('.');
      FullFilename.EraseAfter(n);

      //      if ((RecordSyntax.Equals(HTML_OID)) ||
      //          (RecordSyntax.Equals(NEW_HTML_OID)) ||
      //          (RecordSyntax.Equals(NEW2_HTML_OID)))
      if (RecordSyntax.CaseEquals(HtmlRecordSyntax))
	FullFilename.Cat(SHORT_ANZLIC_HTML_EXTENSION);  // extension=".htm"
      //      else if (RecordSyntax.Equals(XML_OID))
      else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
        FullFilename.Cat(ANZLIC_XML_EXTENSION);         // extension=".xml"
      //      else if (RecordSyntax.Equals(SUTRS_OID))
      else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	FullFilename.Cat(SHORT_ANZLIC_TEXT_EXTENSION);  // extension=".txt"
      else
	FullFilename=HoldFilename; // Just use the file that was indexed
      
      if (IsFile(FullFilename)) {
	// The file is there - read it in
	b.ReadFile(FullFilename);

      } else {
	// Hmmm...  We didn't find that one either
	// Let's try upper case extensions
	FullFilename.EraseAfter(n);

	//        if ((RecordSyntax.Equals(HTML_OID)) ||
	//            (RecordSyntax.Equals(NEW_HTML_OID)) ||
	//            (RecordSyntax.Equals(NEW2_HTML_OID)))
        if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
	  FullFilename.Cat(ANZLIC_HTML_EXTENSION_UC);  // extension=".HTML"
	//        else if (RecordSyntax.Equals(XML_OID))
        else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
          FullFilename.Cat(ANZLIC_XML_EXTENSION_UC);   // extension=".XML"
	//	else if (RecordSyntax.Equals(SUTRS_OID))
	else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	  FullFilename.Cat(ANZLIC_TEXT_EXTENSION_UC);  // extension=".TXT"
	else
	  FullFilename=HoldFilename; // Just use the file that was indexed
      
	if (IsFile(FullFilename)) {
	  // The file is there - read it in
	  b.ReadFile(FullFilename);

	} else {
	  // Nope - one more possibility
	  FullFilename.EraseAfter(n);

	  //          if ((RecordSyntax.Equals(HTML_OID)) ||
	  //              (RecordSyntax.Equals(NEW_HTML_OID)) ||
	  //              (RecordSyntax.Equals(NEW2_HTML_OID)))
          if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
	    FullFilename.Cat(SHORT_ANZLIC_HTML_EXTENSION_UC); // extension=".HTM"
	  //          else if (RecordSyntax.Equals(XML_OID))
          else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
            FullFilename.Cat(ANZLIC_XML_EXTENSION_UC);        // extension=".XML"
	  //	  else if (RecordSyntax.Equals(SUTRS_OID))
	  else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	    FullFilename.Cat(SHORT_ANZLIC_TEXT_EXTENSION_UC); // extension=".TXT"
	  else
	    FullFilename=HoldFilename; // Just use the file that was indexed
	  
	  if (IsFile(FullFilename)) {
	    // The file is there - read it in
	    b.ReadFile(FullFilename);

	  } else {
            cout << "Error: presentation file not found." << endl;
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

ANZLIC::~ANZLIC ()
{
}

/*
   What:        Given a buffer of sgml-tagged data:
   It searches for "tags" and
   (1) returns a list of char* to all characters immediately
   following each '<' character in the buffer.
   
   Pre: b = character buffer with valid sgml marked-up text
   len = length of b
   tags = Empty char**
   
   Post: tags is filled with char pointers to first character of every sgml 
   tag (first character after the '<').  The tags array is 
   terminated by a nullptr.
   Returns the total number of tags found or -1 if out of memory
   */
PCHR *ANZLIC::parse_tags (PCHR b, GPTYPE len) const
{
  PCHR *t;		/* array of pointers to first char of tags */
  size_t tc = 0; /* tag count */
  size_t max_num_tags; /* max num tags for which space is allocated */
  int bracket = 0; /* Declaration bracket depth */
  enum {
    OK, NEED_END, IN_DECL
    }
  State = OK;
  
#ifndef TAG_GROW_SIZE
#define TAG_GROW_SIZE 128
#endif
  const INT grow_size = TAG_GROW_SIZE;
#undef TAG_GROW_SIZE
  
  // You should allocate character pointers (to tags) as you need them.
  // Start with TAG_GROW_SIZE of them.
  max_num_tags = grow_size;
  t = new PCHR[max_num_tags];
  
  // Step though every character in the buffer looking for '<' and '>'
  for (GPTYPE i = 0; i < len; i++)
    {
      switch (b[i])
	{
	case '[':
	  if (State == IN_DECL)
	    bracket++;
	  break;
	case ']':
	  if (State == IN_DECL)
	    if (--bracket <= 0)
	      bracket = 0;
	  break;
	  
	case '>':
	  if (State == IN_DECL && bracket == 0)
	    State = OK;
	  else if (State == NEED_END)
	    {
	      State = OK;
	      b[i] = '\0';
	      // Expand memory if needed
	      if (++tc == max_num_tags - 1)
		{
		  // allocate more space
		  max_num_tags += grow_size;
		  PCHR *New = new PCHR[max_num_tags];
		  if (New == nullptr)
		    {
		      delete[]t;
		      return nullptr;		// NO MORE CORE!
		    }
		  memcpy (New, t, tc * sizeof (PCHR));
		  delete[]t;
		  t = New;
		}
	    }
	  break;
	  
	case '<':
	  // Is the tag a comment or control?
	  if (b[i + 1] == '!')
	    {
	      /* The SGML was not parsed! */
	      i++;
	      if (b[i + 1] == '-' && b[i + 2] == '-')
		{
		  // SGML comment <!-- blah blah ... -->
		  while (i < len)
		    {
		      if (b[i++] != '-') continue;
		      if (b[i++] != '-') continue;
		      if (b[i++] != '>') continue;
		      break;		// End of comment found
		    }				// while
		}
	      else		// Declaration <!XXX [ <...> <...> ]>
		{
		  State = IN_DECL;
		}
	    }			// if <!
	  
	  else if (State == OK)
	    {
	      // Skip over leading white space
	      do
		{
		  i++;
		}
	      while (isspace (b[i]));
	      t[tc] = &b[i];	// Save tag
#if ANZLIC_ACCEPT_EMPTY_TAGS
	      if (b[i] == '>')
		i--;		// empty tag so back up.. 
#endif
	      State = NEED_END;
	    }
	  break;
	  
	default:
	  break;
	}			// switch
      
    }			// for
  
  // Any Errors underway?
  if (State != OK)
    {
      delete[]t;
      return nullptr;		// Parse ERROR
    }
  
  t[tc] = (PCHR) nullptr;	// Mark end of list
  return t;
}


/*
   Searches through string list t look for "/" followed by tag, e.g. if
   tag = "TITLE REL=XXX", looks for "/TITLE" or a empty end tag (</>).
   
   Pre: t is is list of string pointers each nullptr-terminated.  The list
   should be terminated with a nullptr character pointer.
   
   Post: Returns a pointer to found string or nullptr.
   */

//const PCHR ANZLIC::find_end_tag (const char *const *t, const char *tag) const
const PCHR ANZLIC::find_end_tag (char **t, const char *tag) const
{
  size_t len;
  if (t == nullptr || *t == nullptr)
    return nullptr;		// Error

  // BUGFIX #2: added additional null check before dereferencing **t
  if (tag == nullptr)
    return nullptr;		// Invalid tag pointer

  if (*t[0] == '/')
    return nullptr;		// I'am confused!
  
  // Look for "real" tag name
  for (len = 0; tag[len]; len++)
    if (isspace (tag[len]))
      break;			// Have length
  
  size_t i = 0;
  const char *tt = *t;
  do
    {
      if (tt[0] == '/')
	{
#if ANZLIC_ACCEPT_EMPTY_TAGS
	  // for empty end tags: see SGML Handbook Annex C.1.1.1, p.68
	  if (tt[1] == '\0' || tag[0] == '\0')
	    return (const PCHR) tt;
#endif
	  
	  if (tt[1 + len] != '\0' && !isspace (tt[1 + len]))
	    continue;		// Nope
	  
	  // SGML tags are case INDEPENDENT
	  if (0 == StrNCaseCmp (&tt[1], tag, len))
	    return (const PCHR) tt; // Found it
	  
	}
    }
  while ((tt = t[++i]) != nullptr);
  
#if 0
  // No end tag, assume that the document was valid
  // and the end-tag is implicit with the start of the
  // next tag
  return t[1];
#else
  return nullptr;		// No end tag found
#endif
}
