// $Id: anzmeta.cxx,v 1.2 2000/10/11 14:02:15 cnidr Exp $
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
File:		anzmeta.cxx
Description:	Class ANZMETA - adaptation of FGDC Document Type
Options:	-o fieldtype=<filename>
			assigns data types to field names
		-o filter=<command-line>
			defines a program to call to dynamically format output

Author:         David Crossley <crossley@indexgeo.com.au>
IG_version:     $Revision: 1.2 $

Previous:       fgdc.cxx 1.19 1998/02/26 01:50:24 cnidr
Author:         Archie Warnock, warnock@clark.net
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
#include "anzmeta.hxx"

// IG 19980205 - replaced all MD_Element with ZMD_Element
//
// Record Syntaxes are now defined in Isearch/src/defs.cxx

//#define ANZDEBUG 1

#define PRESENT_PROGRAM "/home/cnidr/bin/mp -c /home/cnidr/bin/deluxe.cfg"

void BuildFlyCommandLine(const STRING& Command, const STRING& FullFilename, 
		      const STRING& RecordSyntax, CHR * OutputFilename, 
		      STRING *cmd);

ANZMETA::ANZMETA (PIDBOBJ DbParent) : SGMLNORM (DbParent)
{
}

void 
ANZMETA::LoadFieldTable() {
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
ANZMETA::UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  if (FieldName.Search("ABSTRACT"))
    return GDT_TRUE;
  else if (FieldName.Search("AVLFORM"))
    return GDT_TRUE;
  else if (FieldName.Search("BEGDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("BOUNDING"))
    return GDT_TRUE;
  else if (FieldName.Search("CUSTOD"))
    return GDT_TRUE;
  else if (FieldName.Search("EASTBC"))
    return GDT_TRUE;
  else if (FieldName.Search("ENDDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("KEYWORD"))
    return GDT_TRUE;
  else if (FieldName.Search("JURISDIC"))
    return GDT_TRUE;
  else if (FieldName.Search("METD"))
    return GDT_TRUE;
  else if (FieldName.Search("NORTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("ONLITITL"))
    return GDT_TRUE;
  else if (FieldName.Search("ONLITYPE"))
    return GDT_TRUE;
  else if (FieldName.Search("ORIGIN"))
    return GDT_TRUE;
  else if (FieldName.Search("POSTAL"))
    return GDT_TRUE;
  else if (FieldName.Search("PROGRESS"))
    return GDT_TRUE;
  else if (FieldName.Search("SOUTHBC"))
    return GDT_TRUE;
  else if (FieldName.Search("TIMEPERD"))
    return GDT_TRUE;
  else if (FieldName.Search("TITLE"))
    return GDT_TRUE;
  else if (FieldName.Search("UNIQUEID"))
    return GDT_TRUE;
  else if (FieldName.Search("WESTBC"))
    return GDT_TRUE;
  else
    return GDT_FALSE;
}


void 
ANZMETA::ParseRecords (const RECORD& FileRecord)
{
  SGMLNORM::ParseRecords (FileRecord);
}


void 
ANZMETA::ParseDate(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}

// IG 19980205 start

// ANZMETA::ParseDate
//
// deal with ANZMETA date structure and ISO8601 dates
// replaced complete FGDC routine

void 
ANZMETA::ParseDate(const CHR *Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
//  CHR *found;
//  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;

  Hold = Buffer;

  // A single date will hold either a DATE element or a KEYWORD element
  
  // first look for a KEYWORD element

  Hold.UpperCase();
  Start = Hold.Search("<KEYWORD");
  if (Start > 0) {                          // Found KEYWORD element
    Start = Hold.Search(">");
    //Start += strlen("<KEYWORD");
    //Hold.EraseBefore(Start);
    Hold.EraseBefore(Start+1);
    End = Hold.Search("</KEYWORD");
    Hold.EraseAfter(End-1);
    // we now have the KEYWORD content
    // trim leading and trailing whitespace
    Hold.Trim();
    Hold.TrimLeading();
//    #ifdef ANZDEBUG
//      cout << "Found KEYWORD, value=" << Hold << "###" << endl;
//    #endif
    if (Hold.CaseEquals("current")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = DATE_PRESENT;
      return;
    } else if (Hold.CaseEquals("not known")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = *fStart;
      return;
      }
      else {
        cerr << "[ANZMETA::ParseDate] Unknown DATE-KEYWORD: " << Hold << endl;
        *fEnd = DATE_ERROR;
    }
  }

  // now look for a DATE element

  Start = Hold.Search("<DATE>");
  if (Start > 0) {                          // Found DATE element
    Start += strlen("<DATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</DATE");
    Hold.EraseAfter(End-1);
    // we now have the DATE content
    // it will be an ISO 8601 format date: 1998-01-29 or 1998-01 or 1998
    // parse the string using strtok
    CHR Date[256];
    char *Year, *Month, *Day;
    const char Delimiters[] = " \t\n\r-";  // whitespace or dash

    Hold.GetCString(Date,256);   // get a copy of the string
    Year  = strtok(Date,Delimiters);
    Month = strtok(NULL,Delimiters);
    Day   = strtok(NULL,Delimiters);

//    #ifdef ANZDEBUG
//      cout << "Year=" << Year << "###" << endl;
//      cout << "Month=" << Month << "###" << endl;
//      cout << "Day=" << Day << "###" << endl;
//    #endif

    if (strlen(Year) != 4) {
      cerr << "[ANZMETA::ParseDate] Invalid Date: Year (" << Year;
      cerr << ") must be 4 digits" << endl;
      if (strlen(Year) > 4) {
        cerr << "     ISO 8601 dates should look like ";
        cerr << "1998-01-29 or 1998-01 or 1998" << endl;
      }
    }
    if ((Month != NULL) && (strlen(Month) != 2)) {
      cerr << "[ANZMETA::ParseDate] Invalid Date: Month (" << Month;
      cerr << ") must be 2 digits" << endl;
    }
    if ((Day != NULL) && (strlen(Day) != 2)) {
      cerr << "[ANZMETA::ParseDate] Invalid Date: Day (" << Day;
      cerr << ") must be 2 digits" << endl;
    }

    // rebuild the date
    CHR NewDate[9];
    strcpy(NewDate,Year);
    if (Month != NULL) 
      strcat(NewDate,Month);
    if (Day != NULL) 
      strcat(NewDate,Day);

    Hold = (STRING) NewDate;

//    #ifdef ANZDEBUG
//      cout << "NewDate=" << Hold << "###" << endl;
//    #endif

    if (Hold.IsNumber()) {
      *fStart = Hold.GetFloat();
      *fEnd = *fStart;
      return;
    }
    else {
      cerr << "[ANZMETA::ParseDate] Unknown ISO 8601 DATE: " << Hold << endl;
      cerr << "     ISO 8601 dates should look like ";
      cerr << "1998-01-29 or 1998-01 or 1998" << endl;
      *fEnd = DATE_ERROR;
      return;
    }
   
  }
  return;
}

// IG 19980205 end

void 
ANZMETA::ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}

// IG 19980205 start

// ANZMETA::ParseDateRange
//
// deal with ANZMETA date structure and ISO8601 dates
// replaced complete FGDC routine

void 
ANZMETA::ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
//  CHR *found;
//  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;
  SRCH_DATE dStart, dEnd;

  Hold = Buffer;

  // A range of dates (TIMEPERD) will hold BEGDATE and ENDDATE
  // Each of these can hold a DATE element or a KEYWORD element
  
  Hold.UpperCase();

  Start = Hold.Search("<BEGDATE>");

  if (Start > 0) {                 // Found the opening tag
    Start += strlen("<BEGDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</BEGDATE");
    if (End == 0) {                // but not the closing tag
      cerr << "[ANZMETA::ParseDateRange] <BEGDATE> found, missing </BEGDATE>" 
	   << endl;
      *fStart = DATE_ERROR;
    }
    Hold.EraseAfter(End-1);
//    #ifdef ANZDEBUG
//      cout << "Found BEGDATE, value=" << Hold << "###" << endl;
//    #endif

  // first look for a KEYWORD element
  // DATE KEYWORD is one of a set of terms: CURRENT, NOT KNOWN

  Hold.UpperCase();
  Start = Hold.Search("<KEYWORD");
  if (Start > 0) {                          // Found KEYWORD element
    Start = Hold.Search(">");
    //Start += strlen("<KEYWORD");
    //Hold.EraseBefore(Start);
    Hold.EraseBefore(Start+1);
    End = Hold.Search("</KEYWORD");
    Hold.EraseAfter(End-1);
    // we now have the KEYWORD content
    // trim leading and trailing whitespace
    Hold.Trim();
    Hold.TrimLeading();
//    #ifdef ANZDEBUG
//      cout << "Found KEYWORD, value=" << Hold << "###" << endl;
//    #endif
  }

  // now look for a DATE element

  Start = Hold.Search("<DATE>");
  if (Start > 0) {                          // Found DATE element
    Start += strlen("<DATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</DATE");
    Hold.EraseAfter(End-1);
    // we now have the DATE content
    // trim leading and trailing whitespace
    Hold.Trim();
    Hold.TrimLeading();
//    #ifdef ANZDEBUG
//      cout << "Found DATE, value=" << Hold << "###" << endl;
//    #endif
    // it will be an ISO 8601 format date: 1998-01-29 or 1998-01 or 1998
    // parse the string using strtok
    CHR Date[256];
    char *Year, *Month, *Day;
    const char Delimiters[] = " \t\n\r-";  // whitespace or dash

    Hold.GetCString(Date,256);   // get a copy of the string
    Year  = strtok(Date,Delimiters);
    Month = strtok(NULL,Delimiters);
    Day   = strtok(NULL,Delimiters);

//    #ifdef ANZDEBUG
//      cout << "Year=" << Year << "###" << endl;
//      cout << "Month=" << Month << "###" << endl;
//      cout << "Day=" << Day << "###" << endl;
//    #endif

    // rebuild the date
    CHR NewDate[9];
    strcpy(NewDate,Year);
    if (Month != NULL) 
      strcat(NewDate,Month);
    if (Day != NULL) 
      strcat(NewDate,Day);

    Hold = (STRING) NewDate;

//    #ifdef ANZDEBUG
//      cout << "NewDate=" << Hold << "###" << endl;
//    #endif
  }

    if (Hold.CaseEquals("current")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_ERROR;
    } else if (Hold.CaseEquals("not known")) {
      *fStart = DATE_UNKNOWN;
    } else if (Hold.IsNumber()) {
      //      *fStart = Hold.GetFloat();
      dStart = Hold.GetFloat();
      if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
	dStart.PromoteToDayStart();
      }
      *fStart = dStart.GetValue();

    } else {
      cerr << "[ANZMETA::ParseDateRange] Did not parse <BEGDATE>, value=" 
	   << Hold << endl;
      *fStart = DATE_ERROR;
    }

    // Copy the input buffer again so we can look for the ENDDATE tag
    Hold = Buffer;
    Hold.UpperCase();

    Start = Hold.Search("<ENDDATE>");
    if (Start > 0) {
      Start += strlen("<ENDDATE>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</ENDDATE");
      if (End == 0) {
	cerr << "[ANZMETA::ParseDateRange] <ENDDATE> found, missing </ENDDATE>" 
	     << endl;
	*fEnd = DATE_ERROR;
	return;
      }
      Hold.EraseAfter(End-1);
//    #ifdef ANZDEBUG
//      cout << "Found ENDDATE, value=" << Hold << "###" << endl;
//    #endif

  // first look for a KEYWORD element
  // DATE KEYWORD is one of a set of terms: CURRENT, NOT KNOWN

  Hold.UpperCase();
  Start = Hold.Search("<KEYWORD");
  if (Start > 0) {                          // Found KEYWORD element
    Start = Hold.Search(">");
    //Start += strlen("<KEYWORD");
    //Hold.EraseBefore(Start);
    Hold.EraseBefore(Start+1);
    End = Hold.Search("</KEYWORD");
    Hold.EraseAfter(End-1);
    // we now have the KEYWORD content
    // trim leading and trailing whitespace
    Hold.Trim();
    Hold.TrimLeading();
//    #ifdef ANZDEBUG
//      cout << "Found KEYWORD, value=" << Hold << "###" << endl;
//    #endif
  }

  // now look for a DATE element

  Start = Hold.Search("<DATE>");
  if (Start > 0) {                          // Found DATE element
    Start += strlen("<DATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</DATE");
    Hold.EraseAfter(End-1);
    // we now have the DATE content
    // trim leading and trailing whitespace
    Hold.Trim();
    Hold.TrimLeading();
    #ifdef ANZDEBUG
      cout << "Found DATE, value=" << Hold << "###" << endl;
    #endif
    // it will be an ISO 8601 format date: 1998-01-29 or 1998-01 or 1998
    // parse the string using strtok
    CHR Date[256];
    char *Year, *Month, *Day;
    const char Delimiters[] = " \t\n\r-";  // whitespace or dash

    Hold.GetCString(Date,256);   // get a copy of the string
    Year  = strtok(Date,Delimiters);
    Month = strtok(NULL,Delimiters);
    Day   = strtok(NULL,Delimiters);

//    #ifdef ANZDEBUG
//      cout << "Year=" << Year << "###" << endl;
//      cout << "Month=" << Month << "###" << endl;
//      cout << "Day=" << Day << "###" << endl;
//    #endif

    // rebuild the date
    CHR NewDate[9];
    strcpy(NewDate,Year);
    if (Month != NULL) 
      strcat(NewDate,Month);
    if (Day != NULL) 
      strcat(NewDate,Day);

    Hold = (STRING) NewDate;

    #ifdef ANZDEBUG
      cout << "NewDate=" << Hold << "###" << endl;
    #endif
  }

      if (Hold.CaseEquals("current")) {
	*fEnd = DATE_PRESENT;
      } else if (Hold.CaseEquals("not known")) {
	*fEnd = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	//	*fEnd = Hold.GetFloat();
	dEnd = Hold.GetFloat();
	if ((dEnd.IsYearDate()) || dEnd.IsMonthDate()) {
	  dEnd.PromoteToDayEnd();
	}
	*fEnd   = dEnd.GetValue();
      } else {
	cerr << "[ANZMETA::ParseDateRange] Did not parse <ENDDATE>, value=" 
	     << Hold << endl;
	*fEnd = DATE_ERROR;
      }
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
ANZMETA::ParseComputed(const STRING& FieldName, const CHR *Buffer)
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
GetAnzNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag) {
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
    //cout << "[ANZMETA::GetAnzNumericValue ... " << Tag << " = " << Hold << endl;
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
ANZMETA::ParseGPoly(const CHR *Buffer, DOUBLE Vertices[])
{

  DOUBLE North,South,East,West;
  DOUBLE Left;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<WESTBC>");
  strcpy(eTag,"</WESTBC>");
  West = GetAnzNumericValue(Buffer,Tag,eTag);
  Vertices[0] = West;

  strcpy(Tag,"<NORTHBC>");
  strcpy(eTag,"</NORTHBC>");
  North = GetAnzNumericValue(Buffer,Tag,eTag);
  Vertices[1] = North;

  strcpy(Tag,"<EASTBC>");
  strcpy(eTag,"</EASTBC>");
  East = GetAnzNumericValue(Buffer,Tag,eTag);
  Vertices[2] = East;

  strcpy(Tag,"<SOUTHBC>");
  strcpy(eTag,"</SOUTHBC>");
  South = GetAnzNumericValue(Buffer,Tag,eTag);
  Vertices[3] = South;

  //  sprintf(Vertices,"%10.5f,%10.5f %10.5f,%10.5f\0",West,North,East,South);
#ifdef ANZDEBUG
  cout << "[ANZMETA::ParseGPoly ... ";
  cout
    << "North=" << North
    << ", West=" << West
    << ", South=" << South
    << ", East=" << East
    << endl;
#endif
  
  return 2;
}


// Calculates the extent from the BOUNDING text buffer.  Right now, it is
// hideously inaccurate, because it assumes that the bounding box is an
// actual rectangle, rather than projected on the surface of a sphere, and 
// it does the calculations in decimal degrees instead of radians, but 
// that is what the GEO profile specifies right now.
void 
ANZMETA::ParseExtent(const CHR* Buffer, DOUBLE* extent)
{
  DOUBLE North,South,East,West;
  DOUBLE NewEast;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<NORTHBC>");
  strcpy(eTag,"</NORTHBC>");
  North = GetAnzNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<SOUTHBC>");
  strcpy(eTag,"</SOUTHBC>");
  South = GetAnzNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<EASTBC>");
  strcpy(eTag,"</EASTBC>");
  East = GetAnzNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<WESTBC>");
  strcpy(eTag,"</WESTBC>");
  West = GetAnzNumericValue(Buffer,Tag,eTag);

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
ANZMETA::ParseFields (RECORD *NewRecord)
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
//  PZMD_Element pCurrentTag;
  PDFT pdft = new DFT ();
  GDT_BOOLEAN InCustom;
  size_t val_start;
  int val_len;
  size_t val_end;

  InCustom = GDT_FALSE;
  for (CHR **tags_ptr = tags; *tags_ptr; tags_ptr++) {
    if ((*tags_ptr)[0] == '/') {
      PZMD_Element pTmp;
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
	pTmp = (PZMD_Element)Nested.Top();
	if (Tag == pTmp->get_tag()) {
	  pTmp = (PZMD_Element)Nested.Pop();
//	  cout << "Popped " << pTmp->get_tag() << " off the stack.  ";
	  delete pTmp;
	  if (Nested.GetSize() != 0) {
	    pTmp = (PZMD_Element)Nested.Top();
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
	PZMD_Element pTag = new ZMD_Element();
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
	    PZMD_Element pTmp;
	    if (val_start < LastEnd) {
	      pTmp = (PZMD_Element)Nested.Top();
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

	    // Check to see if it is the BOUNDING field.  If so, handle the
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
ANZMETA::GetCleanedFieldData(const RESULT& ResultRecord, 
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
ANZMETA::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING *StringBuffer)
{
  STRING FieldName;
  STRING XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n";

#ifdef ANZDEBUG
  cout << "[ANZMETA::Present] ElementSet=" << ElementSet << "..." ;
  cout << " RecordSyntax=" << RecordSyntax << "..." << endl;
#endif

  // if ElementSet B - present TITLE
  // if ElementSet A - present TITLE plus ABSTRACT
  // if ElementSet S - present TITLE
  // if ElementSet F - present full document (depends on Record Syntax)

  // Note: AMWG have not yet decided on metadata for resource linkage
  //       so cannot do R Element Set yet (get from fgdc.cxx)

  //----- ElementSet B ----------------------------

  if (ElementSet.Equals(BRIEF_MAGIC)) {
    #ifdef ANZDEBUG
      cout << "[ANZMETA::Present] going to deliver B ElementSet" << endl;
    #endif

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

    if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      // ANZMETA XML does not have the same stucture as FGDC XML
      // however, we will send an FGDC structure here to simplify
      // the interoperability of servers and gateways
      *StringBuffer = XML_Header;
      StringBuffer->Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
      StringBuffer->Cat("<title>");
      // Clean the title buffer to make sure it is legal XML
      Title.Replace("'","&apos;");
      Title.Replace("\"","&quot;");
      StringBuffer->Cat(Title);
      StringBuffer->Cat("</title>\n");
      StringBuffer->Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");
    } else if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
        *StringBuffer = Title;
    } else {
      *StringBuffer = Title;
    }

    #ifdef ANZDEBUG
      cout << "[ANZMETA::Present] delivered B ElementSet" << endl;
    #endif
  } // end of ElementSet=B

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

    if ((RecordSyntax.CaseEquals(HtmlRecordSyntax)) ||
        (RecordSyntax.CaseEquals(XmlRecordSyntax))) {
      *StringBuffer = "<b>";
      *StringBuffer += Title;
      *StringBuffer += "</b>";
      FieldName = "ABSTRACT";
      Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
      if (Status) {
        // Note: the ANZMETA ABSTRACT allows max 2000 characters
        //       so later we may need to truncate this to a resonable
        //       presentation length
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
  } // end of ElementSet=A

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

    FieldName = "ANZMETA_STATUS_UPDATE_KEYWORD";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Update);
    if (Status) {
      Hold = "UPDATE=";
      Hold.Cat(Update);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    *StringBuffer = ESN_S;
  } // end of ElementSet=S

  //----- ElementSet F ----------------------------

  else {
    #ifdef ANZDEBUG
      cout << "[ANZMETA::Present] going to deliver F ElementSet" << endl;
    #endif

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


    //----- HTML ------------------------------------

    if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      FullFilename.Cat(ANZ_HTML_EXTENSION);  // extension=".html"
      cout << "[ANZMETA::Present] RecordSyntax=HTML" << endl;
    }

    //----- XML -------------------------------------
    else if ((RecordSyntax.CaseEquals(XmlRecordSyntax)) ||
             (RecordSyntax.CaseEquals(SgmlRecordSyntax))) {
      FullFilename.Cat(ANZ_XML_EXTENSION);   // extension=".xml"
      cout << "[ANZMETA::Present] RecordSyntax=XML" << endl;
    }

    //----- SUTRS -----------------------------------
    else if (RecordSyntax.CaseEquals(SutrsRecordSyntax)) {
      FullFilename.Cat(ANZ_TEXT_EXTENSION);  // extension=".txt"
      cout << "[ANZMETA::Present] RecordSyntax=SUTRS" << endl;
    }

    //----- Unkown   (deal with others --------------
    else {
      // it must be a Record Syntax that we cannot handle
      // so just present the file that was indexed
      cout << "[ANZMETA::Present] RecordSyntax=Unknown" << endl;
      FullFilename=HoldFilename;
    }

    //----- now present the relevant document -------
    cout << "[ANZMETA::Present] FullFilename=" << FullFilename << endl;
    if (IsFile(FullFilename)) {
      // The file is there - read it in
      b.ReadFile(FullFilename);
      cout << "[ANZMETA::Present] File was found" << endl;
    }
    else {

      //----- could not find, so try other filenames --
      cerr << "[ANZMETA::Present] File was not found" << endl;

      // Hmmm...  We did not find the requested file
      // Let us try sticking on the short extensions instead
      n = FullFilename.SearchReverse('.');
      FullFilename.EraseAfter(n);

      if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
	    FullFilename.Cat(SHORT_ANZ_HTML_EXTENSION);  // extension=".htm"
      else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
        FullFilename.Cat(ANZ_XML_EXTENSION);         // extension=".xml"
      else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	    FullFilename.Cat(SHORT_ANZ_TEXT_EXTENSION);  // extension=".txt"
      else
        // it must be a Record Syntax that we cannot handle
        // so just present the file that was indexed
	FullFilename=HoldFilename;
      
      if (IsFile(FullFilename)) {
	// The file is there - read it in
	b.ReadFile(FullFilename);

      }
      else {
	// Hmmm...  We did not find that one either
	// Let us try upper case extensions
	FullFilename.EraseAfter(n);

        if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
	  FullFilename.Cat(ANZ_HTML_EXTENSION_UC);  // extension=".HTML"
        else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
          FullFilename.Cat(ANZ_XML_EXTENSION_UC);   // extension=".XML"
	else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	  FullFilename.Cat(ANZ_TEXT_EXTENSION_UC);  // extension=".TXT"
	else
          // it must be a Record Syntax that we cannot handle
          // so just present the file that was indexed
	  FullFilename=HoldFilename;
      
	if (IsFile(FullFilename)) {
	  // The file is there - read it in
	  b.ReadFile(FullFilename);

	} else {
	  // Nope - one more possibility
	  FullFilename.EraseAfter(n);

          if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) 
	    FullFilename.Cat(SHORT_ANZ_HTML_EXTENSION_UC); // extension=".HTM"
          else if (RecordSyntax.CaseEquals(XmlRecordSyntax))
            FullFilename.Cat(ANZ_XML_EXTENSION_UC);        // extension=".XML"
	  else if (RecordSyntax.CaseEquals(SutrsRecordSyntax))
	    FullFilename.Cat(SHORT_ANZ_TEXT_EXTENSION_UC); // extension=".TXT"
	  else
            // it must be a Record Syntax that we cannot handle
            // so just present the file that was indexed
	    FullFilename=HoldFilename;
	  
	  if (IsFile(FullFilename)) {
	    // The file is there - read it in
	    b.ReadFile(FullFilename);

	  } else {
	    // OK - none of them are here, so try running the defined program
            // to generate the document on the fly
	    if (mpCommand.GetLength() == 0) {
	      // Not defined, so bail out
	      *StringBuffer = "Requested program not found";
	      return;
	    } else {
	      STRING s_cmd;
	      //CHR* c_cmd;
	      CHR TmpName[] = "/tmp/mpoutXXXXXX";
	      int TmpFd = mkstemp(TmpName);
	      if (TmpFd < 0) {
		*StringBuffer = "Unable to create temporary file";
		return;
	      }
	      close(TmpFd);

          cout << "[ANZMETA::Present] no docs found, so build Fly cmd" << endl;

	      BuildFlyCommandLine(mpCommand, HoldFilename, RecordSyntax, 
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

    if (b.GetLength() <= 0)
      *StringBuffer = "";
    else
      *StringBuffer = b;

    #ifdef ANZDEBUG
      cout << "[ANZMETA::Present] delivered F ElementSet" << endl;
    #endif
  } // end of ElementSet=F

  #ifdef ANZDEBUG
    cout << "[ANZMETA::Present] finished Present" << endl;
  #endif

  return;
  
}

ANZMETA::~ANZMETA ()
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
ANZMETA::parse_tags (CHR *b, GPTYPE len) const
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
#if ANZ_ACCEPT_EMPTY_TAGS
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


//const PCHR ANZMETA::find_end_tag (const char *const *t, const char *tag) const
const CHR* 
ANZMETA::find_end_tag (char **t, const char *tag) const
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
#if ANZ_ACCEPT_EMPTY_TAGS
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
BuildFlyCommandLine(const STRING& Command, const STRING& FullFilename, 
		 const STRING& RecordSyntax, CHR *OutputFilename,
		 STRING *cmdline)
{
  STRING hold;
  STRING RecordSyntaxOID;

  hold = Command;
  if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
    hold.Cat(" -h ");
    RecordSyntaxOID = HtmlRecordSyntaxOID;
  } 
  else {
    if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      hold.Cat(" -x ");
      RecordSyntaxOID = XmlRecordSyntaxOID;
    }
    else {  // SUTRS or unknown Record Syntax
      hold.Cat(" -t ");
      RecordSyntaxOID = SutrsRecordSyntaxOID;
    }
  }
  hold.Cat(OutputFilename);
  hold.Cat(" ");
  hold.Cat(FullFilename);

  hold.Cat(" ");
  hold.Cat(RecordSyntaxOID);

  // so the call will be:
  //   fly-command <rs-hint> <outfile> <infile> <record_syntax_OID>

  *cmdline = hold;
  return;
}
