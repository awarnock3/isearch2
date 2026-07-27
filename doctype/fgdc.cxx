/* $Id: fgdc.cxx,v 1.33 2000/09/06 18:20:30 cnidr Exp $ */
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
File:		fgdc.cxx
Version:	$Revision: 1.33 $
Description:	Class FGDC - FGDC Document Type
Options:	-o fieldtype=<filename>
			assigns data types to field names
		-o filter=<command-line>
			defines a program to call to dynamically format output
Author:		Archie Warnock, warnock@clark.net
			Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and USGS/FGDC
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "isearch.hxx"
#include "date.hxx"
#include "sgmlnorm.hxx"
#include "fgdc.hxx"

#define PRESENT_PROGRAM "/home/cnidr/bin/mp -c /home/cnidr/bin/deluxe.cfg"

void BuildCommandLine(const STRING& Command, const STRING& FullFilename, 
		      const STRING& RecordSyntax, CHR * OutputFilename, 
		      STRING *cmd);
//DOUBLE GetNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag);


/* ------- FGDC Support --------------------------------------------- */

FGDC::FGDC (PIDBOBJ DbParent) : SGMLNORM (DbParent)
{
}


void 
FGDC::LoadFieldTable() {
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
#ifdef WIN32
	// We might have to clean up the line endings for text files in Windows
	if (!Field_and_Type.IsPrint()) {
		Field_and_Type.MakePrintable();
		Field_and_Type.Trim();
	}
#endif
    Db->FieldTypes.AddEntry(Field_and_Type);
  } while ( (pBuf = strtok((CHR*)NULL,"\n")) );

  delete [] b;
}


GDT_BOOLEAN 
FGDC::UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  if (FieldName.Search("TITLE"))
    return GDT_TRUE;
  else if (FieldName.Search("PUBDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("DESCRIPT"))
    return GDT_TRUE;
  else if (FieldName.Search("ABSTRACT"))
    return GDT_TRUE;
  else if (FieldName.Search("EDITION"))
    return GDT_TRUE;
  else if (FieldName.Search("PLACEKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("PURPOSE"))
    return GDT_TRUE;
  else if (FieldName.Search("SRCSCALE"))
    return GDT_TRUE;
  else if (FieldName.Search("LINEAGE"))
    return GDT_TRUE;
  else if (FieldName.Search("THEMEKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("THEMEKT"))
    return GDT_TRUE;
  else if (FieldName.Search("PLACEKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("PLACEKT"))
    return GDT_TRUE;
  else if (FieldName.Search("STRATKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("STRATKT"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPKEYT"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPKT"))
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
  else if (FieldName.Search("ORIGIN"))
    return GDT_TRUE;
  else if (FieldName.Search("PUBLISH"))
    return GDT_TRUE;
  else if ((FieldName.Search("BEGDATE")) && !(FieldName.Search("BEGDATEA")))
    return GDT_TRUE;
  else if ((FieldName.Search("ENDDATE")) && !(FieldName.Search("ENDDATEA")))
    return GDT_TRUE;
  else if (FieldName.Search("CALDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("GEOFORM"))
    return GDT_TRUE;
  else if (FieldName.Search("BROWSE"))
    return GDT_TRUE;
  else if (FieldName.Search("BROWSET"))
    return GDT_TRUE;
  else if (FieldName.Search("BROWSED"))
    return GDT_TRUE;
  else if (FieldName.Search("BROWSEN"))
    return GDT_TRUE;
  else if (FieldName.Search("DIRECT"))
    return GDT_TRUE;
  else if (FieldName.Search("INDSPREF"))
    return GDT_TRUE;
  else if (FieldName.Search("DSGPOLY"))
    return GDT_TRUE;
  else if (FieldName.Search("DSGPOLYX"))
    return GDT_TRUE;
  else if (FieldName.Search("CNTORGP"))
    return GDT_TRUE;
  else if (FieldName.Search("TIMEINFO"))
    return GDT_TRUE;
  else if (FieldName.Search("TIMEPERD"))
    return GDT_TRUE;
  else if (FieldName.Search("RNGDATES"))
    return GDT_TRUE;
  else if (FieldName.Search("PROGRESS"))
    return GDT_TRUE;
  else if (FieldName.Search("UPDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("ONLINK"))
    return GDT_TRUE;
  else if (FieldName.Search("EXTENT"))
    return GDT_TRUE;
  else if (FieldName.Search("ENTTYPL"))
    return GDT_TRUE;
  else if (FieldName.Search("ATTRLABL"))
    return GDT_TRUE;
  else if (FieldName.Search("KEYWORDS"))
    return GDT_TRUE;
  // Added for NBII
  else if (FieldName.Search("TAXONKEY"))
    return GDT_TRUE;
  else if (FieldName.Search("TAXONCOV"))
    return GDT_TRUE;
  else if (FieldName.Search("TAXONINF"))
    return GDT_TRUE;
  else if (FieldName.Search("KINGDOM"))
    return GDT_TRUE;
  else if (FieldName.Search("PHYLUM"))
    return GDT_TRUE;
  else if (FieldName.Search("CLASS"))
    return GDT_TRUE;
  else if (FieldName.Search("ORDER"))
    return GDT_TRUE;
  else if (FieldName.Search("FAMILY"))
    return GDT_TRUE;
  else if (FieldName.Search("GENUS"))
    return GDT_TRUE;
  else if (FieldName.Search("COMMON"))
    return GDT_TRUE;
  else if (FieldName.Search("TAXONGEN"))
    return GDT_TRUE;
  else if (FieldName.Search("SPECIES"))
    return GDT_TRUE;
  else
    return GDT_FALSE;
}


void 
FGDC::ParseRecords (const RECORD& FileRecord)
{
  SGMLNORM::ParseRecords (FileRecord);
}


void 
FGDC::ParseDate(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}


void 
FGDC::ParseDate(const CHR *Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
//  CHR *found;
//  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;

  Hold = Buffer;

  // Check to see if we just got a single numeric value
  // - it might start with a digit, or one of a couple of characters
  // Single values map to trivial intervals (startdate=enddate)
  if (Hold.IsNumber()) {
    *fStart = Hold.GetFloat();
    *fEnd = *fStart;
    return;
  } else if (Hold.CaseEquals("present")) {
    *fStart = DATE_PRESENT;
    *fEnd = *fStart;
    return;
  } else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd = *fStart;
    return;
  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {
    *fStart = Hold.GetFloat();
    *fEnd = *fStart;
    return;
  }

  // It might contain a block of tags instead of a single date string,
  // so we'd better look for sensible tags.  If it's a single date, it'll
  // contains <CALDATE> (well, and <SNGDATE>, but we don't need that) and
  // if it's a range, it has to contain <BEGDATE> and <ENDDATE>.
  //
  // Let's do the single date case first
  Hold.UpperCase();
  Start = Hold.Search("<CALDATE>");

  if (Start > 0) {              // Found it
    Start += strlen("<CALDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</CALDATE");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[FGDC::ParseDate] <CALDATE> found, missing </CALDATE>" << endl;
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = DATE_PRESENT;
      return;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = *fStart;
      return;
    } else if (Hold.IsNumber()) {
      *fStart = Hold.GetFloat();
      *fEnd = *fStart;
      return;
    }
  }

  // Otherwise, it had better be a buffer containing an interval
  // If so, the dates will be tagged with <begdate> and <enddate>
  //  Hold.UpperCase();

  Start = Hold.Search("<BEGDATE>");

  if (Start > 0) {                 // Found the opening tag
    Start += strlen("<BEGDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</BEGDATE");
    if (End == 0) {                // but not the closing tag
      cerr << "[FGDC::ParseDate] <BEGDATE> found, missing </BEGDATE>" 
	   << endl;
      *fStart = DATE_ERROR;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_ERROR;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
    } else if (Hold.IsNumber()) {
      *fStart = Hold.GetFloat();
    } else {
      cerr << "[FGDC::ParseDate] Didn't parse <BEGDATE>, value=" 
	   << Hold << endl;
      *fStart = DATE_ERROR;
    }

    // Copy the input buffer again so we can look for the enddate tag
    Hold = Buffer;
    Hold.UpperCase();

    Start = Hold.Search("<ENDDATE>");
    if (Start > 0) {
      Start += strlen("<ENDDATE>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</ENDDATE");
      if (End == 0) {
	cerr << "[FGDC::ParseDate] <ENDDATE> found, missing </ENDDATE>" 
	     << endl;
	*fEnd = DATE_ERROR;
	return;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")) {
	*fEnd = DATE_PRESENT;
      } else if (Hold.CaseEquals("unknown")) {
	*fEnd = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	*fEnd = Hold.GetFloat();
      } else {
	cerr << "[FGDC::ParseDate] Didn't parse <ENDDATE>, value=" 
	     << Hold << endl;
	*fEnd = DATE_ERROR;
      }
    } else {
      *fEnd = DATE_ERROR;
    }
#ifdef DEBUG
    cerr << "Buffer: " << Buffer << endl
	 << "\t[" << (INT*)fStart << ", " << (INT*)fEnd << "]" << endl;
#endif
  } else {             // We're out of tags to look for...
    *fStart = DATE_ERROR;
    *fEnd = *fStart;
    return;
  }
  return;
}


void 
FGDC::ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}


void 
FGDC::ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
//  CHR *found;
//  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;
  SRCH_DATE dStart, dEnd;

  Hold = Buffer;

  // Check to see if we just got a single numeric value
  // - it might start with a digit, or one of a couple of characters
  // Single values map to trivial intervals (startdate=enddate)
  if (Hold.IsNumber()) {
    //    *fStart = Hold.GetFloat();
    //    *fEnd = *fStart;
    dStart = Hold.GetFloat();
    dEnd   = dStart;
    if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
      dStart.PromoteToDayStart();
      dEnd.PromoteToDayEnd();
    }
    *fStart = dStart.GetValue();
    *fEnd   = dEnd.GetValue();
    return;
  } else if (Hold.CaseEquals("present")) {
    //    *fStart = DATE_PRESENT;
    //    *fEnd = *fStart;
    *fStart = DATE_UNKNOWN;
    *fEnd   = DATE_PRESENT;
    return;
  } else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd   = *fStart;
    return;
  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {
    //    *fStart = Hold.GetFloat();
    //    *fEnd = *fStart;
    dStart = Hold.GetFloat();
    dEnd   = dStart;
    if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
      dStart.PromoteToDayStart();
      dEnd.PromoteToDayEnd();
    }
    *fStart = dStart.GetValue();
    *fEnd   = dEnd.GetValue();
    return;
  }

  // It might contain a block of tags instead of a single date string,
  // so we'd better look for sensible tags.  If it's a single date, it'll
  // contains <CALDATE> (well, and <SNGDATE>, but we don't need that) and
  // if it's a range, it has to contain <BEGDATE> and <ENDDATE>.
  //
  // Let's do the single date case first
  Hold.UpperCase();
  Start = Hold.Search("<CALDATE>");

  if (Start > 0) {              // Found it
    Start += strlen("<CALDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</CALDATE");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[FGDC::ParseDate] <CALDATE> found, missing </CALDATE>" << endl;
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = DATE_PRESENT;
      return;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = *fStart;
      return;
    } else if (Hold.IsNumber()) {
      //      *fStart = Hold.GetFloat();
      //      *fEnd = *fStart;
      dStart = Hold.GetFloat();
      dEnd   = dStart;
      if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
	dStart.PromoteToDayStart();
	dEnd.PromoteToDayEnd();
      }
      *fStart = dStart.GetValue();
      *fEnd   = dEnd.GetValue();
      return;
    }
  }

  // Otherwise, it had better be a buffer containing an interval
  // If so, the dates will be tagged with <begdate> and <enddate>
  //  Hold.UpperCase();

  Start = Hold.Search("<BEGDATE>");

  if (Start > 0) {                 // Found the opening tag
    Start += strlen("<BEGDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</BEGDATE");
    if (End == 0) {                // but not the closing tag
      cerr << "[FGDC::ParseDate] <BEGDATE> found, missing </BEGDATE>" 
	   << endl;
      *fStart = DATE_ERROR;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_ERROR;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
    } else if (Hold.IsNumber()) {
      //      *fStart = Hold.GetFloat();
      dStart = Hold.GetFloat();
      if ((dStart.IsYearDate()) || dStart.IsMonthDate()) 
      	dStart.PromoteToDayStart();

      *fStart = dStart.GetValue();

    } else {
      cerr << "[FGDC::ParseDate] Didn't parse <BEGDATE>, value=" 
	   << Hold << endl;
      *fStart = DATE_ERROR;
    }

    // Copy the input buffer again so we can look for the enddate tag
    Hold = Buffer;
    Hold.UpperCase();

    Start = Hold.Search("<ENDDATE>");
    if (Start > 0) {
      Start += strlen("<ENDDATE>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</ENDDATE");
      if (End == 0) {
	cerr << "[FGDC::ParseDate] <ENDDATE> found, missing </ENDDATE>" 
	     << endl;
	*fEnd = DATE_ERROR;
	return;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")) {
	*fEnd = DATE_PRESENT;
      } else if (Hold.CaseEquals("unknown")) {
	*fEnd = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	//	*fEnd = Hold.GetFloat();
	dEnd = Hold.GetFloat();
	if ((dEnd.IsYearDate()) || dEnd.IsMonthDate()) {
	  dEnd.PromoteToDayEnd();
	}
	*fEnd   = dEnd.GetValue();
      } else {
	cerr << "[FGDC::ParseDate] Didn't parse <ENDDATE>, value=" 
	     << Hold << endl;
	*fEnd = DATE_ERROR;
      }
#ifdef DEBUG
    cerr << "Buffer: " << Buffer << endl
	 << "\t[" << (INT)*fStart << ", " << (INT)*fEnd << "]" << endl;
#endif
    } else {
      *fEnd = DATE_ERROR;
    }
  } else {             // We're out of tags to look for...
    *fStart = DATE_ERROR;
    *fEnd = *fStart;
    return;
  }

  return;
}


// The FGDC metadata records only has one computed field - the physical
// extent of the bounding box.  In general, this routine will walk through
// the list of computed fields desired, and call the routine that does the
// actual calculation
DOUBLE 
FGDC::ParseComputed(const STRING& FieldName, const CHR *Buffer)
{
  DOUBLE extent;
  if (FieldName.CaseEquals("EXTENT")) {
    ParseExtent(Buffer, &extent);
    return(extent);
  } else {
    return(0.0);
  }
}


// Handy routine to make sure we always get back a numeric value where
// appropriate and where expected.  It correctly handles the (obviously)
// non-numeric value UNKNOWN by sending back a 0.
DOUBLE
GetNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag) {
  STRING Hold;
  STRINGINDEX Start, End;
  DOUBLE fValue;

  Hold = Buffer;
  Hold.UpperCase();
  Start = Hold.Search(Tag);

  if (Start > 0) {                 // Found the opening tag
    Start += strlen(Tag);
    Hold.EraseBefore(Start);
    End = Hold.Search(eTag);
    if (End == 0) {                // but not the closing tag
      return(0.0);
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("UNKNOWN")) {
      return(0.0);
    } else if (Hold.IsNumber()) {
      fValue = Hold.GetFloat();
      return(fValue);
    }
  }
  return(0.0);
}


// Parses the buffer, looking for the coordinates of the corners of
// the bounding box, and returns them in the array Vertices.  The order
// is significant - it is the FGDC-prescribed ordered-pair syntax for the
// Northwest and Southeast corners:
// (West,North East,South)
INT
FGDC::ParseGPoly(const CHR *Buffer, DOUBLE Vertices[])
{

  DOUBLE North,South,East,West;
  DOUBLE Left;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<WESTBC>");
  strcpy(eTag,"</WESTBC>");
  West = GetNumericValue(Buffer,Tag,eTag);
  Vertices[0] = West;

  strcpy(Tag,"<NORTHBC>");
  strcpy(eTag,"</NORTHBC>");
  North = GetNumericValue(Buffer,Tag,eTag);
  Vertices[1] = North;

  strcpy(Tag,"<EASTBC>");
  strcpy(eTag,"</EASTBC>");
  East = GetNumericValue(Buffer,Tag,eTag);
  Vertices[2] = East;

  strcpy(Tag,"<SOUTHBC>");
  strcpy(eTag,"</SOUTHBC>");
  South = GetNumericValue(Buffer,Tag,eTag);
  Vertices[3] = South;

  //  sprintf(Vertices,"%10.5f,%10.5f %10.5f,%10.5f\0",West,North,East,South);
  
  return 2;
}


// Calculates the extent from the BOUNDING text buffer.  Right now, it is
// hideously inaccurate, because it assumes that the bounding box is an
// actual rectangle, rather than projected on the surface of a sphere, and 
// it does the calculations in decimal degrees instead of radians, but 
// that is what the GEO profile specifies right now.
void 
FGDC::ParseExtent(const CHR* Buffer, DOUBLE* extent)
{
  DOUBLE North,South,East,West;
  DOUBLE NewEast;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<NORTHBC>");
  strcpy(eTag,"</NORTHBC>");
  North = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<SOUTHBC>");
  strcpy(eTag,"</SOUTHBC>");
  South = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<EASTBC>");
  strcpy(eTag,"</EASTBC>");
  East = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<WESTBC>");
  strcpy(eTag,"</WESTBC>");
  West = GetNumericValue(Buffer,Tag,eTag);

  if (East < West) {
    //    Left = -180 + West;
    NewEast = East + 360;
  } else {
    NewEast = East;
  }

  *extent = fabs((NewEast - West) * (North - South));

#ifdef DEBUG
  cout 
    << "North=" << North
    << ", South=" << South
    << ", East=" <<East
    << ", West=" <<West
    << endl;
  cout
    << "Extent " << *extent
    << " = (" << NewEast
    << " - " << West
    << ") * (" << North
    << " - " << South
    << ") = " << NewEast-West
    << " * " << North-South
    << endl;
#endif

  return;
}


void 
FGDC::ParseFields (RECORD *NewRecord)
{
  PFILE fp;
  STRING fn;

  if (NewRecord == (RECORD*)NULL) 
    return;                      // ERROR

  // Open the file
  NewRecord->GetFullFileName (&fn);
  if (!(fp = fopen (fn, "rb")))
    return;			 // ERROR

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
  CHR *RecBuffer;
  RecBuffer = new CHR[RecLength + 1];
  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  fclose (fp);

  STRING FieldName;
  STRING FullFieldname;
  STRING ExtentName;

  FC fc, fc_full;
  DF df, df_full;
  DFD dfd, dfd_full;
  STRING doctype;
  NUMERICFLD nc, nc_full;

  FullFieldname = "";
  NewRecord->GetDocumentType(&doctype);

  CHR **tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL) {
    cout << "Unable to parse `" << doctype << "' tags in file " << fn << "\n";
    // Clean up
    delete [] RecBuffer;
    return;
  }

  GSTACK Nested;
  size_t LastEnd=(size_t)0;
//  PMD_Element pCurrentTag;
  PDFT pdft = new DFT ();
  GDT_BOOLEAN InCustom;
  size_t val_start;
  int val_len;
  size_t val_end;

  InCustom = GDT_FALSE;
  for (CHR **tags_ptr = tags; *tags_ptr; tags_ptr++) {
    if ((*tags_ptr)[0] == '/') {
      PMD_Element pTmp;
      if (strcmp(*tags_ptr,"/custom")) {

	STRING Tag;
	STRINGINDEX x;
	//CHR *cx;

	Tag = *tags_ptr;
	x=Tag.Search('/');
	Tag.EraseBefore(x+1);

	// We keep a stack of the fields we have currently open.  This
	// handles nested fields by making a long field name out of the
	// nested values.
	pTmp = (PMD_Element)Nested.Top();
	if (Tag == pTmp->get_tag()) {
	  pTmp = (PMD_Element)Nested.Pop();
//	  cout << "Popped " << pTmp->get_tag() << " off the stack.  ";
	  delete pTmp;
	  if (Nested.GetSize() != 0) {
	    pTmp = (PMD_Element)Nested.Top();
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

    const CHR *p = find_end_tag (tags_ptr, *tags_ptr);
    size_t tag_len = strlen (*tags_ptr);
    int have_attribute_val = (NULL != strchr (*tags_ptr, '='));

    if (p != NULL) {
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
	PMD_Element pTag = new MD_Element();
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
	if (unified_name == NULL) 
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
	    PMD_Element pTmp;
	    if (val_start < LastEnd) {
	      pTmp = (PMD_Element)Nested.Top();
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

	    // Check to see if it's the BOUNDING field.  If so, handle the
	    // EXTENT field, too.
	    if (FieldName.Equals("BOUNDING")) {
	      ExtentName = "EXTENT";
	      NewType = ExtentName;
	      NewType.Cat("=");
	      NewType.Cat("computed");
	      NewType.UpperCase();
	      Db->FieldTypes.AddEntry(NewType);

	      dfd_full.SetFieldName (ExtentName);
	      dfd_full.SetFieldType ("computed");
	      //  cout << "Found " << FullFieldname << " of type " << FieldType
	      //       << endl;
	      Db->DfdtAddEntry (dfd_full);
	      fc_full.SetFieldStart (val_start);
	      fc_full.SetFieldEnd (val_end);
	      PFCT pfct2 = new FCT ();
	      pfct2->AddEntry (fc_full);
	      df_full.SetFct (*pfct2);
	      df_full.SetFieldName (ExtentName);
	      pdft->AddEntry (df_full);
	      delete pfct2;
	    }
	  }
	  Nested.Push(pTag);
	  LastEnd = val_end;
	}
      }
    }
    if (have_attribute_val) {
      SGMLNORM::store_attributes (pdft, RecBuffer, *tags_ptr);
    } else if (p == NULL) {
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


GDT_BOOLEAN 
FGDC::GetCleanedFieldData(const RESULT& ResultRecord, 
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
FGDC::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING *StringBuffer)
{
  STRING FieldName;
  STRING XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n";
  STRING SGML_Header = "<!DOCTYPE METADATA PUBLIC \"-//FGDC//DTD METADATA 2.0//EN\">\n";
  
  if (ElementSet.Equals(BRIEF_MAGIC)) {
    STRLIST Strlist;
    STRING Title;
    GDT_BOOLEAN Status;

    FieldName = "METADATA_IDINFO_CITATION_CITEINFO_TITLE";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else {
      Title = "(title not found)";
    }

    if (RecordSyntax.CaseEquals("XML")) {
      *StringBuffer = XML_Header;
      StringBuffer->Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
      StringBuffer->Cat("<title>");
      // Clean the title buffer to make sure it is legal XML
      Title.Replace("'","&apos;");
      Title.Replace("\"","&quot;");
      StringBuffer->Cat(Title);
      StringBuffer->Cat("</title>\n");
      StringBuffer->Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");
    } else if (RecordSyntax.CaseEquals("HTML")) {
      //      *StringBuffer = "<html>\n<head>\n";
      //      StringBuffer->Cat("<title>");
      //      StringBuffer->Cat(Title);
      *StringBuffer = Title;
      //      StringBuffer->Cat("</title>");
      //      StringBuffer->Cat("</head>\n<body>\n</body>\n</html>\n");
    } else {
      *StringBuffer = Title;
    }

  // added for title-geoform element set identified by C dbergstedt
  } else if (ElementSet.CaseEquals("C")) {
     STRLIST Strlist;
     STRING Hold, FieldType,ESN_C;
     STRING Title,GeoForm;
     GDT_BOOLEAN Status;
 
     if (RecordSyntax.CaseEquals("XML")) {
       ESN_C = XML_Header;
       ESN_C.Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
     } else {
       ESN_C = "";
     }
 
     FieldName = "METADATA_IDINFO_CITATION_CITEINFO_TITLE";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<title>";
	 Title.Replace("'","&apos;");
	 Title.Replace("\"","&quot;");
	 Hold.Cat(Title);
	 Hold.Cat("</title>\n");
	 ESN_C.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P><B>TITLE: </B>";
	 Hold.Cat(Title);
	 Hold.Cat("</P>\n");
	 ESN_C.Cat(Hold);
       } else {
	 Hold = "TITLE: ";
	 Hold.Cat(Title);
	 ESN_C.Cat(Hold);
       }
     }
 
     FieldName = "METADATA_IDINFO_CITATION_CITEINFO_GEOFORM";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, GeoForm);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<geoform>";
	 GeoForm.Replace("'","&apos;");
	 GeoForm.Replace("\"","&quot;");
	 Hold.Cat(GeoForm);
	 Hold.Cat("</geoform>\n");
	 ESN_C.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P><B>FORM: </B>";
	 Hold.Cat(GeoForm);
	 Hold.Cat("</P>\n");
	 ESN_C.Cat(Hold);
       } else {
	 Hold = "FORM: ";
	 Hold.Cat(GeoForm);
	 ESN_C.Cat(Hold);
       }
     }
 
     if (RecordSyntax.CaseEquals("XML")) {
       ESN_C.Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");
     }

     *StringBuffer = ESN_C;

  } else if (ElementSet.CaseEquals("R")) {
     STRLIST Strlist;
     STRING Hold, FieldType,ESN_R;
     STRING Title,GeoForm;
     GDT_BOOLEAN Status;
 
     if (RecordSyntax.CaseEquals("XML")) {
       ESN_R = XML_Header;
       ESN_R.Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
     } else {
       ESN_R = "";
     }
 
     FieldName = "METADATA_IDINFO_CITATION_CITEINFO_TITLE";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<title>";
	 Title.Replace("'","&apos;");
	 Title.Replace("\"","&quot;");
	 Hold.Cat(Title);
	 Hold.Cat("</title>\n");
	 ESN_R.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P><B>TITLE: </B>";
	 Hold.Cat(Title);
	 Hold.Cat("</P>\n");
	 ESN_R.Cat(Hold);
       } else {
	 Hold = "TITLE: ";
	 Hold.Cat(Title);
	 ESN_R.Cat(Hold);
       }
     }
     FieldName = "METADATA_IDINFO_CITATION_CITEINFO_ONLINK";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, GeoForm);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<onlink>";
	 GeoForm.Replace("'","&apos;");
	 GeoForm.Replace("\"","&quot;");
	 Hold.Cat(GeoForm);
	 Hold.Cat("</onlink>\n");
	 ESN_R.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P><B>Link: </B>";
	 Hold.Cat(GeoForm);
	 Hold.Cat("</P>\n");
	 ESN_R.Cat(Hold);
       } else {
	 Hold = "LINK: ";
	 Hold.Cat(GeoForm);
	 ESN_R.Cat(Hold);
       }
     }
 
     if (RecordSyntax.CaseEquals("XML")) {
       ESN_R.Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");
     }

     *StringBuffer = ESN_R;

  } else if (ElementSet.CaseEquals("A")) {
     STRLIST Strlist;
     STRING Hold, FieldType,ESN_A;
     STRING Title,Abstract;
     GDT_BOOLEAN Status;
 
     if (RecordSyntax.CaseEquals("XML")) {
       ESN_A = XML_Header;
       ESN_A.Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
     } else {
       ESN_A = "";
     }
 
     FieldName = "METADATA_IDINFO_CITATION_CITEINFO_TITLE";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<title>";
	 Title.Replace("'","&apos;");
	 Title.Replace("\"","&quot;");
	 Hold.Cat(Title);
	 Hold.Cat("</title>\n");
	 ESN_A.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P><B>TITLE: </B>";
	 Hold.Cat(Title);
	 Hold.Cat("</P>\n");
	 ESN_A.Cat(Hold);
       } else {
	 Hold = "TITLE: ";
	 Hold.Cat(Title);
	 ESN_A.Cat(Hold);
       }
     }

     if (RecordSyntax.CaseEquals("XML")) {
	 ESN_A.Cat("</citeinfo>\n</citation>\n");
	 ESN_A.Cat("<descript>\n");
     }

     FieldName = "METADATA_IDINFO_DESCRIPT_ABSTRACT";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Abstract);
     if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<abstract>";
	 Abstract.Replace("'","&apos;");
	 Abstract.Replace("\"","&quot;");
	 Hold.Cat(Abstract);
	 Hold.Cat("</abstract>\n");
	 ESN_A.Cat(Hold);
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>";
	 Hold.Cat(Abstract);
	 Hold.Cat("</P>\n");
	 ESN_A.Cat(Hold);
       } else {
	 Hold = "FORM: ";
	 Hold.Cat(Abstract);
	 ESN_A.Cat(Hold);
       }
     }
 
     if (RecordSyntax.CaseEquals("XML")) {
	 ESN_A.Cat("</descript>\n");
	 ESN_A.Cat("</idinfo>\n</metadata>\n");
     }

     *StringBuffer = ESN_A;

  } else if (ElementSet.CaseEquals("S")) {
/*  These fields define the S element set
         Title
         Edition
         Geospatial_Data_Presentation_Form
         Indirect_Spatial_Reference
         West_Bounding_Coordinate
         East_Bounding_Coordinate
         North_Bounding_Coordinate
         South_Bounding_Coordinate
         Beginning_Date
         Ending_Date
         Calendar_Date (need field name...)
         Maintenance_and_Update_Frequency
         Browse_Graphic_File_Name
*/
    STRLIST Strlist;
    STRING Hold, FieldType,ESN_S;
    STRING Title,Edition,GeoForm,Spatial,West,East,North,South;
    STRING BegDate,EndDate,CalDate,Update,BrowseGraphic;
    GDT_BOOLEAN Status;

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S = XML_Header;
      ESN_S.Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
    } else {
      ESN_S = "";
    }

    FieldName = "METADATA_IDINFO_CITATION_CITEINFO_TITLE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
    if (Status) {
      if (RecordSyntax.CaseEquals("XML")) {
	Hold = "<title>";
	Title.Replace("'","&apos;");
	Title.Replace("\"","&quot;");
	Hold.Cat(Title);
	Hold.Cat("</title>\n");
      } else if (RecordSyntax.CaseEquals("HTML")) {
	Hold = "<P><B>";
	Hold.Cat(Title);
	Hold.Cat("</B></P>");
      } else {
	Hold = "TITLE=";
	Hold.Cat(Title);
	Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    } 

    FieldName = "METADATA_IDINFO_CITATION_CITEINFO_EDITION";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Edition);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<edition>";
	 Edition.Replace("'","&apos;");
	 Edition.Replace("\"","&quot;");
	 Hold.Cat(Edition);
	 Hold.Cat("</edition>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>EDITION=";
	 Hold.Cat(Edition);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "EDITION=";
	 Hold.Cat(Edition);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    FieldName = "METADATA_IDINFO_CITATION_CITEINFO_GEOFORM";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, GeoForm);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<geoform>";
	 GeoForm.Replace("'","&apos;");
	 GeoForm.Replace("\"","&quot;");
	 Hold.Cat(GeoForm);
	 Hold.Cat("</geoform>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>GEOFORM=";
	 Hold.Cat(GeoForm);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "GEOFORM=";
	 Hold.Cat(GeoForm);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</citeinfo>\n</citation>\n");
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("<timeperd>\n<timeinfo>\n<rngdates>\n");
    }

    FieldName = "METADATA_IDINFO_TIMEPERD_TIMEINFO_RNGDATES_BEGDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, BegDate);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<begdate>";
	 Hold.Cat(BegDate);
	 Hold.Cat("</begdate>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>BEGDATE=";
	 Hold.Cat(BegDate);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "BEGDATE=";
	 Hold.Cat(BegDate);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    FieldName = "METADATA_IDINFO_TIMEPERD_TIMEINFO_RNGDATES_ENDDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, EndDate);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<enddate>";
	 Hold.Cat(EndDate);
	 Hold.Cat("</enddate>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>ENDDATE=";
	 Hold.Cat(EndDate);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "ENDDATE=";
	 Hold.Cat(EndDate);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</rngdates>\n</timeinfo>\n</timeperd>\n");
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("<status>\n");
    }

    FieldName = "METADATA_IDINFO_STATUS_UPDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Update);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<update>";
	 Update.Replace("'","&apos;");
	 Update.Replace("\"","&quot;");
	 Hold.Cat(Update);
	 Hold.Cat("</update>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>UPDATE=";
	 Hold.Cat(Update);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "UPDATE=";
	 Hold.Cat(Update);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</status>\n");
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("<spdom>\n<bounding>\n");
    }

    FieldName = "METADATA_IDINFO_SPDOM_BOUNDING_WESTBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, West);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<westbc>";
	 Hold.Cat(West);
	 Hold.Cat("</westbc>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>WESTBC=";
	 Hold.Cat(West);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "WESTBC=";
	 Hold.Cat(West);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    FieldName = "METADATA_IDINFO_SPDOM_BOUNDING_EASTBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, East);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<eastbc>";
	 Hold.Cat(East);
	 Hold.Cat("</eastbc>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>EASTBC=";
	 Hold.Cat(East);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "EASTBC=";
	 Hold.Cat(East);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    FieldName = "METADATA_IDINFO_SPDOM_BOUNDING_NORTHBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, North);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<northbc>";
	 Hold.Cat(North);
	 Hold.Cat("</northbc>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>NORTHBC=";
	 Hold.Cat(North);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "NORTHBC=";
	 Hold.Cat(North);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    FieldName = "METADATA_IDINFO_SPDOM_BOUNDING_SOUTHBC";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, South);
    if (Status) {
       if (RecordSyntax.CaseEquals("XML")) {
	 Hold = "<southbc>";
	 Hold.Cat(South);
	 Hold.Cat("</southbc>\n");
       } else if (RecordSyntax.CaseEquals("HTML")) {
	 Hold = "<P>SOUTHBC=";
	 Hold.Cat(South);
	 Hold.Cat("</P>\n");
       } else {
	 Hold = "SOUTHBC=";
	 Hold.Cat(South);
	 Hold.Cat("\n");
      }
      ESN_S.Cat(Hold);
    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</bounding>\n</spdom>\n");
    }

    FieldName = "METADATA_IDINFO_BROWSE_BROWSEN";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, BrowseGraphic);
    if (Status) {
      if (RecordSyntax.CaseEquals("XML")) {
	Hold = "<browsen>";
	BrowseGraphic.Replace("'","&apos;");
	BrowseGraphic.Replace("\"","&quot;");
	BrowseGraphic.Replace("&gt;"," ");
	BrowseGraphic.Replace("&lt;"," ");
	BrowseGraphic.Replace("|","</browsen>\n</browse>\n<browse>\n<browsen>");
	Hold.Cat(BrowseGraphic);
	Hold.Cat("</browsen>\n");
      } else if (RecordSyntax.CaseEquals("HTML")) {
	Hold = "<P>BROWSEN=";
	Hold.Cat(BrowseGraphic);
	Hold.Cat("</P>\n");
      } else {
	Hold = "BROWSEN=";
	Hold.Cat(BrowseGraphic);
	Hold.Cat("\n");
      }

      if (RecordSyntax.CaseEquals("XML")) {
	ESN_S.Cat("<browse>\n");
      }

      ESN_S.Cat(Hold);

      if (RecordSyntax.CaseEquals("XML")) {
	ESN_S.Cat("</browse>\n");
      }

    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</idinfo>\n");
    }

    FieldName = "METADATA_SPDOINFO_INDSPREF";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Spatial);
    if (Status) {
      if (RecordSyntax.CaseEquals("XML")) {
	Hold = "<indspref>";
	Spatial.Replace("'","&apos;");
	Spatial.Replace("\"","&quot;");
	Hold.Cat(Spatial);
	Hold.Cat("</indspref>\n");
      } else if (RecordSyntax.CaseEquals("HTML")) {
	Hold = "<P>INDSPREF=";
	Hold.Cat(Spatial);
	Hold.Cat("</P>\n");
      } else {
	Hold = "INDSPREF=";
	Hold.Cat(Spatial);
	Hold.Cat("\n");
      }
      if (RecordSyntax.CaseEquals("XML")) {
	ESN_S.Cat("<spdoinfo>\n");
      }

      ESN_S.Cat(Hold);

      if (RecordSyntax.CaseEquals("XML")) {
	ESN_S.Cat("</spdoinfo>\n");
      }

    }

    if (RecordSyntax.CaseEquals("XML")) {
      ESN_S.Cat("</metadata>\n");
    }

    *StringBuffer = ESN_S;

  } else if (ElementSet.CaseEquals("F")) {
    STRING FullFilename, HoldFilename;
    STRING b;
    INT n;

    STRLIST StrList;
    STRING mpCommand;
    Db->GetDocTypeOptions(&StrList);

    StrList.GetValue("filter", &mpCommand);

    ResultRecord.GetFullFileName(&FullFilename);
    HoldFilename = FullFilename;

    n = FullFilename.SearchReverse('.');
    FullFilename.EraseAfter(n);

    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      FullFilename.Cat(FGDC_HTML_EXTENSION);  // extension=".html"

    } else if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      FullFilename.Cat(FGDC_XML_EXTENSION);  // extension=".xml"

    } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
      FullFilename.Cat(FGDC_SGML_EXTENSION);  // extension=".sgml"

    } else if (RecordSyntax.CaseEquals(SutrsRecordSyntax)) {
      FullFilename.Cat(FGDC_TEXT_EXTENSION);  // extension=".text"

    } else {
      FullFilename=HoldFilename; // Just use the filename in the result
    }

    if (IsFile(FullFilename)) {
      // The file is there - read it in
      b.ReadFile(FullFilename);

    } else {
      // Hmmm...  We didn't find the requested file
      // Let's try sticking on the short extensions instead
      n = FullFilename.SearchReverse('.');
      FullFilename.EraseAfter(n);

      if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
	FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION);  // extension=".htm"

      } else if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
	FullFilename.Cat(SHORT_FGDC_XML_EXTENSION);  // extension=".xml"

      } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
	FullFilename.Cat(SHORT_FGDC_SGML_EXTENSION);  // extension=".sgm"

      } else if (RecordSyntax.CaseEquals(SutrsRecordSyntax)) {
       	FullFilename.Cat(SHORT_FGDC_TEXT_EXTENSION);  // extension=".txt"

      } else {
	FullFilename=HoldFilename; // Just use the filename in the result
      }
      
      if (IsFile(FullFilename)) {
	// The file is there - read it in
	b.ReadFile(FullFilename);

      } else {
	// Hmmm...  We didn't find that one either
	// Let's try upper case extensions
	FullFilename.EraseAfter(n);

	if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
	  FullFilename.Cat(FGDC_HTML_EXTENSION_UC);  // extension=".HTML"

	} else if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
	  FullFilename.Cat(FGDC_XML_EXTENSION_UC);  // extension=".XML"

	} else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
	  FullFilename.Cat(FGDC_SGML_EXTENSION_UC);  // extension=".SGML"

	} else if (RecordSyntax.CaseEquals(SutrsRecordSyntax)) {
	  FullFilename.Cat(FGDC_TEXT_EXTENSION_UC);  // extension=".TEXT"

	} else {
	  FullFilename=HoldFilename; // Just use the filename in the result
	}
      
	if (IsFile(FullFilename)) {
	  // The file is there - read it in
	  b.ReadFile(FullFilename);

	} else {
	  // Nope - one more possibility
	  FullFilename.EraseAfter(n);

	  if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
	    FullFilename.Cat(SHORT_FGDC_HTML_EXTENSION_UC); // extension=".HTM"

	  } else if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
	    FullFilename.Cat(SHORT_FGDC_XML_EXTENSION_UC); // extension=".XML"

	  } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
	    FullFilename.Cat(SHORT_FGDC_SGML_EXTENSION_UC); // extension=".SGM"

	  } else if (RecordSyntax.CaseEquals(SutrsRecordSyntax)) {
	    FullFilename.Cat(SHORT_FGDC_TEXT_EXTENSION_UC); // extension=".TXT"

	  } else {
	    FullFilename=HoldFilename; // Just use the filename in the result
	  }

	  if (IsFile(FullFilename)) {
	    // The file is there - read it in
	    b.ReadFile(FullFilename);

	  } else {
	    // OK - none of them are here, so try running mp to generate
	    // the document on the fly
	    if (mpCommand.GetLength() == 0) {
	      // Not defined, so bail out
	      *StringBuffer = "Requested file not found";
	      return;
	    } else {
	      STRING s_cmd;
	      CHR TmpName[] = "/tmp/mpoutXXXXXX";
	      int TmpFd = mkstemp(TmpName);
	      if (TmpFd < 0) {
		*StringBuffer = "Unable to create temporary file";
		return;
	      }
	      close(TmpFd);

	      BuildCommandLine(mpCommand, HoldFilename, RecordSyntax, 
			       TmpName, &s_cmd);
	      if (system(s_cmd) != 0) {
		unlink(TmpName);
		*StringBuffer = "Failed to generate requested file";
		return;
	      }

	      b.ReadFile(TmpName);
	      unlink(TmpName);
	    }
	  }
	}
      }
    }

    STRINGINDEX head;

    if (RecordSyntax.CaseEquals("XML")) {
      head = b.Search("<metadata>");
      b.EraseBefore(head);
      b.Insert(1,XML_Header);
    } else  if (RecordSyntax.CaseEquals("SGML")) {
      head = b.Search("<metadata>");
      b.EraseBefore(head);
      b.Insert(1,SGML_Header);
    }
    
    if (b.GetLength() <= 0)
      *StringBuffer = "";
    else
      *StringBuffer = b;
  } else {
    SGMLNORM::Present (ResultRecord, ElementSet, StringBuffer);
  }
  return;
  
}

FGDC::~FGDC ()
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
   terminated by a NULL.
   Returns the total number of tags found or -1 if out of memory
   */
CHR**
FGDC::parse_tags (CHR *b, GPTYPE len) const
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
		  if (New == NULL)
		    {
		      delete[]t;
		      return NULL;		// NO MORE CORE!
		    }
		  memcpy (New, t, tc * sizeof (CHR*));
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
#if FGDC_ACCEPT_EMPTY_TAGS
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
      return NULL;		// Parse ERROR
    }
  
  t[tc] = (CHR*) NULL;	// Mark end of list
  return t;
}


/*
   Searches through string list t look for "/" followed by tag, e.g. if
   tag = "TITLE REL=XXX", looks for "/TITLE" or a empty end tag (</>).
   
   Pre: t is is list of string pointers each NULL-terminated.  The list
   should be terminated with a NULL character pointer.
   
   Post: Returns a pointer to found string or NULL.
   */


//const PCHR FGDC::find_end_tag (const char *const *t, const char *tag) const
const CHR* 
FGDC::find_end_tag (char **t, const char *tag) const
{
  size_t len;
  if (t == NULL || *t == NULL)
    return NULL;		// Error
  
  if (*t[0] == '/')
    return NULL;		// I'am confused!
  
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
#if FGDC_ACCEPT_EMPTY_TAGS
	  // for empty end tags: see SGML Handbook Annex C.1.1.1, p.68
	  if (tt[1] == '\0' || tag[0] == '\0')
	    return (const CHR*) tt;
#endif
	  
	  if (tt[1 + len] != '\0' && !isspace (tt[1 + len]))
	    continue;		// Nope
	  
	  // SGML tags are case INDEPENDENT
	  if (0 == StrNCaseCmp (&tt[1], tag, len))
	    return (const CHR*) tt; // Found it
	  
	}
    }
  while ((tt = t[++i]) != NULL);
  
#if 0
  // No end tag, assume that the document was valid
  // and the end-tag is implicit with the start of the
  // next tag
  return t[1];
#else
  return NULL;		// No end tag found
#endif
}


void
BuildCommandLine(const STRING& Command, const STRING& FullFilename, 
		 const STRING& RecordSyntax, CHR *OutputFilename,
		 STRING *cmdline)
{
  STRING hold;

  hold = Command;
  if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
    hold.Cat(" -h ");
  } else if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
    hold.Cat(" -x ");
  } else if (RecordSyntax.CaseEquals(SgmlRecordSyntax)) {
    hold.Cat(" -s ");
  } else {
    hold.Cat(" -t ");
  }
  hold.Cat(OutputFilename);
  hold.Cat(" ");
  hold.Cat(FullFilename);
  *cmdline = hold;
  return;
}
