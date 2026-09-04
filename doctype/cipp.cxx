// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/* $Id: cipp.cxx,v 1.2 2000/04/01 23:37:34 cnidr Exp $ */
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
File:		cipp.cxx
Version:	$Revision: 1.2 $
Description:	Class CIPP - NASA/CIP Product Metadata Document Type
Options:	-o fieldtype=<filename>
			assigns data types to field names
Author:		Archie Warnock, warnock@clark.net
			Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and NASA
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
#include "cipp.hxx"

extern DOUBLE GetNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag);

/* ------- CIPP Support --------------------------------------------- */

CIPP::CIPP (PIDBOBJ DbParent) : SGMLNORM (DbParent)
{
}


// Reads the file named by the "-o fieldtype=<filename>" doctype
// option (prompting interactively if missing) and loads one
// "FIELDNAME TYPE" entry per line into Db->FieldTypes.
void
CIPP::LoadFieldTable() {
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

  // BUGFIX #5 (docs/BUG_CATALOG.md#doctypecippcxx): this was a
  // do-while, unconditionally running the body (and thus
  // `Field_and_Type = pBuf;`) once before ever checking pBuf. If the
  // FIELDTYPE file exists but is empty (IsFile() above only checks
  // existence, not content), strtok() returns nullptr on the very
  // first call, and STRING::operator=(const CHR*) calls strlen() on
  // it unconditionally -- a null-pointer-dereference crash. Checking
  // pBuf before the first iteration too, not just between iterations,
  // fixes it. Same bug as, and fixed the same way as,
  // doctype/cipc.cxx's BUGFIX #5.
  while (pBuf) {
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
    pBuf = strtok((CHR*)nullptr,"\n");
  }

  delete [] b;
}


GDT_BOOLEAN 
CIPP::UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  /* Mandatory Product Leaf Use Attributes - Table A-6 of CIP Spec */
  if (FieldName.Search("ITEMDESCRIPTORID"))
    return GDT_TRUE;
  else if (FieldName.Search("EASTBOUNDINGCOORDINATE"))
    return GDT_TRUE;
  else if (FieldName.Search("ENDDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("NORTHBOUNDINGCOORDINATE"))
    return GDT_TRUE;
  else if (FieldName.Search("SOUTHBOUNDINGCOORDINATE"))
    return GDT_TRUE;
  else if (FieldName.Search("STARTDATE"))
    return GDT_TRUE;
  else if (FieldName.Search("WESTBOUNDINGCOORDINATE"))
    return GDT_TRUE;

  /* Mandatory Product Compound Use Attributes - Table A-7 of CIP Spec */
  else if (FieldName.Search("BOUNDINGRECTANBLE"))
    return GDT_TRUE;
  else if (FieldName.Search("PRODUCTDESCRIPTOR"))
    return GDT_TRUE;
  else if (FieldName.Search("SPATIALCOVERAGE"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPORALCOVERAGE"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPORALRANGE"))
    return GDT_TRUE;

  /* Optional Product Leaf Use Attributes - Table A-8 of CIP Spec */
  else if (FieldName.Search("ABSTRACT"))
    return GDT_TRUE;
  else if (FieldName.Search("ARCHIVINGCENTREID"))
    return GDT_TRUE;
  else if (FieldName.Search("AUTHORITATIVE"))
    return GDT_TRUE;
  else if (FieldName.Search("ELEVATIONMAXIMUM"))
    return GDT_TRUE;
  else if (FieldName.Search("ELEVATIONMINIMUM"))
    return GDT_TRUE;
  else if (FieldName.Search("FRAME"))
    return GDT_TRUE;
  else if (FieldName.Search("GENERALKEYWORD"))
    return GDT_TRUE;
  else if (FieldName.Search("GROUPID"))
    return GDT_TRUE;
  else if (FieldName.Search("ITEMDESCRIPTORLANGUAGE"))
    return GDT_TRUE;
  else if (FieldName.Search("ITEMLANGUAGE"))
    return GDT_TRUE;
  else if (FieldName.Search("ITEMSIZE"))
    return GDT_TRUE;
  else if (FieldName.Search("LATITUDE"))
    return GDT_TRUE;
  else if (FieldName.Search("LONGITUDE"))
    return GDT_TRUE;
  else if (FieldName.Search("MISSIONID"))
    return GDT_TRUE;
  else if (FieldName.Search("ORDERINGCENTREID"))
    return GDT_TRUE;
  else if (FieldName.Search("ORIGINATOR"))
    return GDT_TRUE;
  else if (FieldName.Search("PERIODDEFINITION"))
    return GDT_TRUE;
  else if (FieldName.Search("PROCESSINGCENTRE"))
    return GDT_TRUE;
  else if (FieldName.Search("PROCESSINGLEVELID"))
    return GDT_TRUE;
  else if (FieldName.Search("PRODUCTMEDIUM"))
    return GDT_TRUE;
  else if (FieldName.Search("QAPERCENTCLOUDCOVER"))
    return GDT_TRUE;
  else if (FieldName.Search("QAPERCENTINTERPOLATEDDATA"))
    return GDT_TRUE;
  else if (FieldName.Search("QAPERCENTMISSINGDATA"))
    return GDT_TRUE;
  else if (FieldName.Search("QAPERCENTOUTOFBOUNDSDATA"))
    return GDT_TRUE;
  else if (FieldName.Search("RADIUSVALUE"))
    return GDT_TRUE;
  else if (FieldName.Search("SCALE"))
    return GDT_TRUE;
  else if (FieldName.Search("SENSORID"))
    return GDT_TRUE;
  else if (FieldName.Search("SPATIALRESOLUTION"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPORALRESOLUTION"))
    return GDT_TRUE;
  else if (FieldName.Search("TRACK"))
    return GDT_TRUE;

  /* Optional Product Compound Use Attributes - Table A-9 of CIP Spec */
  else if (FieldName.Search("CIRCLE"))
    return GDT_TRUE;
  else if (FieldName.Search("DATAORIGINATOR"))
    return GDT_TRUE;
  else if (FieldName.Search("GPOLYGON"))
    return GDT_TRUE;
  else if (FieldName.Search("GPOLYGONOUTERGRING"))
    return GDT_TRUE;
  else if (FieldName.Search("INSTRUMENTSENSOR"))
    return GDT_TRUE;
  else if (FieldName.Search("LOCALSCHEMAELEMENTS"))
    return GDT_TRUE;
  else if (FieldName.Search("ORDEROPTIONSGROUP"))
    return GDT_TRUE;
  else if (FieldName.Search("PLATFORM"))
    return GDT_TRUE;
  else if (FieldName.Search("POINT"))
    return GDT_TRUE;
  else if (FieldName.Search("PROCESSING"))
    return GDT_TRUE;
  else if (FieldName.Search("PROCESSINGLEVEL"))
    return GDT_TRUE;
  else if (FieldName.Search("PRODUCTDELIVERYOPTION"))
    return GDT_TRUE;
  else if (FieldName.Search("PRODUCTORDEROPTIONS"))
    return GDT_TRUE;
  else if (FieldName.Search("PRODUCTSERVICEOPTIONS"))
    return GDT_TRUE;
  else if (FieldName.Search("QAPRODUCTSTATISTICS"))
    return GDT_TRUE;
  else if (FieldName.Search("TEMPORALPERIOD"))
    return GDT_TRUE;
  else if (FieldName.Search("VERTICALEXTENT"))
    return GDT_TRUE;
  else if (FieldName.Search("WRSGRSPASS"))
    return GDT_TRUE;
  else if (FieldName.Search("WRSGRSSCENE"))
    return GDT_TRUE;

  /* Nothing else is searchable */
  else
    return GDT_FALSE;
}


// Record splitting is entirely inherited from SGMLNORM; CIPP only
// customizes field parsing (ParseFields() below).
void
CIPP::ParseRecords (const RECORD& FileRecord)
{
  SGMLNORM::ParseRecords (FileRecord);
}


void 
CIPP::ParseDate(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}


// Parses Buffer as either a single date value (a bare number,
// "present", "unknown") or a <StartDate>...<EndDate> pair, writing
// both *fStart and *fEnd in either case (a single value maps to the
// trivial interval fStart==fEnd). The <CALDATE> single-date-block
// shape (see CIPC::ParseDate) is present but commented out here. Sets
// DATE_ERROR on any recognized-but-malformed shape.
void
CIPP::ParseDate(const CHR *Buffer, DOUBLE* fStart,
		DOUBLE* fEnd) {
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

    //  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
    //	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {

  } else if (isdigit(Buffer[0])) {

    //    *fStart = Hold.GetFloat();
    *fStart = ParseIsoDate(Hold);
    *fEnd = *fStart;
    return;
  }

  // It might contain a block of tags instead of a single date string,
  // so we'd better look for sensible tags.  If it's a single date, it'll
  // contains <CALDATE> (well, and <SNGDATE>, but we don't need that) and
  // if it's a range, it has to contain <StartDate> and <EndDate>.
  //
  // Let's do the single date case first
  Hold.UpperCase();
  /*
  Start = Hold.Search("<CALDATE>");

    if (Start > 0) {              // Found it
    Start += strlen("<CALDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</CALDATE");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[CIPP::ParseDate] <CALDATE> found, missing </CALDATE>" << endl;
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


      } else if (isdigit(Buffer[0])) {
      *fStart = Hold.GetFloat();
      *fEnd = *fStart;
      return;
    }
  }
  */

  // Otherwise, it had better be a buffer containing an interval
  // If so, the dates will be tagged with <begdate> and <EndDate>
  //  Hold.UpperCase();

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypecippcxx): Hold was already
  // uppercased above (line ~400) -- originally for the now fully
  // commented-out <CALDATE> block, so it serves no other purpose here
  // any more -- and is never reset to mixed-case before this point, so
  // searching it for the mixed-case needle "<StartDate>" could never
  // match via strstr(). This whole interval-parsing branch was
  // unreachable dead code, silently falling through to the "out of
  // tags to look for" DATE_ERROR case at the bottom of this function
  // for every input, even a well-formed <StartDate>...<EndDate> block.
  // Search for the uppercase form instead (strlen() below is
  // unaffected, same length either way). Same bug as, and fixed the
  // same way as, doctype/cipc.cxx's BUGFIX #1.
  Start = Hold.Search("<STARTDATE>");

  if (Start > 0) {                 // Found the opening tag
    Start += strlen("<StartDate>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</STARTDATE");
    if (End == 0) {                // but not the closing tag
      cerr << "[CIPP::ParseDate] <StartDate> found, missing </StartDate>"
	   << endl;
      // Also fixed here: this branch used to fall through into
      // Hold.EraseAfter(End-1) with End==0 (a STRINGINDEX underflow to
      // SIZE_MAX -- harmless only because EraseAfter() itself already
      // bounds-checks) and then kept going, searching for <EndDate>
      // despite already having flagged the record as malformed,
      // leaving *fEnd unset on this path.
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
    }
    Hold.EraseAfter(End-1);

    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_ERROR;

    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
    
    } else {
      *fStart = ParseIsoDate(Hold);

      //    } else {
      //      cerr << "[CIPP::ParseDate] Didn't parse <StartDate>, value="
      //	   << Hold << endl;
      //      *fStart = DATE_ERROR;
    }

    // Copy the input buffer again so we can look for the EndDate tag
    Hold = Buffer;
    Hold.UpperCase();

    // Same BUGFIX #1 case-mismatch as the <StartDate> search above.
    Start = Hold.Search("<ENDDATE>");
    if (Start > 0) {
      Start += strlen("<EndDate>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</ENDDATE");
      if (End == 0) {
	cerr << "[CIPP::ParseDate] <EndDate> found, missing </EndDate>"
	     << endl;
	*fEnd = DATE_ERROR;
	return;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")) {
	*fEnd = DATE_PRESENT;

      } else if (Hold.CaseEquals("unknown")) {
	*fEnd = DATE_UNKNOWN;

      } else {
	//	*fEnd = Hold.GetFloat();
	*fEnd = ParseIsoDate(Hold);

	//      } else {
	//	cerr << "[CIPP::ParseDate] Didn't parse <EndDate>, value=" 
	//	     << Hold << endl;
	//	*fEnd = DATE_ERROR;
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


void 
CIPP::ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete [] Hold;
  return;
}


// Same grammar and shape as ParseDate() above (a near-duplicate, not
// a wrapper, and unlike ParseDate() here has its <CALDATE> block
// active rather than commented out), but promotes year/month-only
// dates to a full day range via
// SRCH_DATE::PromoteToDayStart()/PromoteToDayEnd().
void
CIPP::ParseDateRange(const CHR *Buffer, DOUBLE* fStart,
		DOUBLE* fEnd) {
  STRING Hold;
  STRINGINDEX Start, End;
  SRCH_DATE dStart, dEnd;

  Hold = Buffer;

  // Check to see if we just got a single numeric value
  // - it might start with a digit, or one of a couple of characters
  // Single values map to trivial intervals (startdate=enddate)
  if (Hold.IsNumber()) {
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
    *fStart = DATE_UNKNOWN;
    *fEnd   = DATE_PRESENT;
    return;

  } else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd   = *fStart;
    return;

  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {
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
  // if it's a range, it has to contain <StartDate> and <EndDate>.
  //
  // Let's do the single date case first
  Hold.UpperCase();
  Start = Hold.Search("<CALDATE>");

  if (Start > 0) {              // Found it
    Start += strlen("<CALDATE>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</CALDATE");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[CIPP::ParseDate] <CALDATE> found, missing </CALDATE>" << endl;
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
  // If so, the dates will be tagged with <StartDate> and <EndDate>
  //  Hold.UpperCase();

  // BUGFIX #2: same case-mismatch-makes-this-branch-unreachable bug,
  // and the same missing-return bug, as ParseDate's BUGFIX #1 above --
  // this is ParseDateRange, a near-duplicate function. Same fix.
  Start = Hold.Search("<STARTDATE>");

  if (Start > 0) {                 // Found the opening tag
    Start += strlen("<StartDate>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</STARTDATE");
    if (End == 0) {                // but not the closing tag
      cerr << "[CIPP::ParseDate] <StartDate> found, missing </StartDate>"
	   << endl;
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
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
      dStart = Hold.GetFloat();
      if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
	dStart.PromoteToDayStart();
      }
      *fStart = dStart.GetValue();

    } else {
      cerr << "[CIPP::ParseDate] Didn't parse <StartDate>, value=" 
	   << Hold << endl;
      *fStart = DATE_ERROR;
    }

    // Copy the input buffer again so we can look for the EndDate tag
    Hold = Buffer;
    Hold.UpperCase();

    // Same BUGFIX #2 case-mismatch as the <StartDate> search above.
    Start = Hold.Search("<ENDDATE>");
    if (Start > 0) {
      Start += strlen("<EndDate>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</ENDDATE");
      if (End == 0) {
	cerr << "[CIPP::ParseDate] <EndDate> found, missing </EndDate>"
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
	dEnd = Hold.GetFloat();
	if ((dEnd.IsYearDate()) || dEnd.IsMonthDate()) {
	  dEnd.PromoteToDayEnd();
	}
	*fEnd   = dEnd.GetValue();

      } else {
	cerr << "[CIPP::ParseDate] Didn't parse <EndDate>, value=" 
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


// Handy routine to make sure we always get back a numeric value where
// appropriate and where expected.  It correctly handles the (obviously)
// non-numeric value UNKNOWN by sending back a 0.
/*DOUBLE
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
*/

// Parses the buffer, looking for the coordinates of the corners of
// the bounding box, and returns them in the array Vertices.  The order
// is significant - it is the CIPP-prescribed ordered-pair syntax for the
// Northwest and Southeast corners:
// (West,North East,South)
INT
CIPP::ParseGPoly(const CHR *Buffer, DOUBLE Vertices[])
{

  DOUBLE North,South,East,West;
  CHR Tag[32];
  CHR eTag[32];

  strcpy(Tag,"<WestBoundingCoordinate>");
  strcpy(eTag,"</WestBoundingCoordinate>");
  West = GetNumericValue(Buffer,Tag,eTag);
  Vertices[0] = West;

  strcpy(Tag,"<NorthBoundingCoordinate>");
  strcpy(eTag,"</NorthBoundingCoordinate>");
  North = GetNumericValue(Buffer,Tag,eTag);
  Vertices[1] = North;

  strcpy(Tag,"<EastBoundingCoordinate>");
  strcpy(eTag,"</EastBoundingCoordinate>");
  East = GetNumericValue(Buffer,Tag,eTag);
  Vertices[2] = East;

  strcpy(Tag,"<SouthBoundingCoordinate>");
  strcpy(eTag,"</SouthBoundingCoordinate>");
  South = GetNumericValue(Buffer,Tag,eTag);
  Vertices[3] = South;

  //  sprintf(Vertices,"%10.5f,%10.5f %10.5f,%10.5f\0",West,North,East,South);
  
  return 2;
}


// Reads NewRecord's bytes off disk, splits them into SGML-style tags
// via parse_tags()/find_end_tag(), and adds one DF field per
// recognized <tag>value</tag> pair -- both under its own short name
// and under a full, underscore-joined name reflecting its nesting
// (tracked via the Nested stack of CIP_Element). Attribute values
// (tag="...") are delegated to SGMLNORM::store_attributes(). See
// doctype/cipc.cxx's CIPC::ParseFields() -- functionally identical.
void
CIPP::ParseFields (RECORD *NewRecord)
{
  PFILE fp;
  STRING fn;

  if (NewRecord == (RECORD*)nullptr) 
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
  if (tags == nullptr) {
    cout << "Unable to parse `" << doctype << "' tags in file " << fn << "\n";
    // Clean up
    delete [] RecBuffer;
    return;
  }

  GSTACK Nested;
  PDFT pdft = new DFT ();
  GDT_BOOLEAN InCustom;
  size_t val_start;
  int val_len;
  size_t val_end;

  InCustom = GDT_FALSE;
  for (CHR **tags_ptr = tags; *tags_ptr; tags_ptr++) {
    if ((*tags_ptr)[0] == '/') {
      PCIP_Element pTmp;
      if (strcmp(*tags_ptr,"/custom")) {

	STRING Tag;
	STRINGINDEX x;

	Tag = *tags_ptr;
	x=Tag.Search('/');
	Tag.EraseBefore(x+1);

	// We keep a stack of the fields we have currently open.  This
	// handles nested fields by making a long field name out of the
	// nested values.
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypecippcxx): Nested.Top()
	// was called here with no GetSize()!=0 guard, unlike its two
	// sibling call sites in this same function -- a closing tag with
	// nothing open on the stack (e.g. an unmatched/extra closing tag
	// in the input) made pTmp a null GSTACK::Top() return, and
	// pTmp->get_tag() below dereferenced it. A stray closing tag with
	// nothing to match is simply ignored. Same bug as, and fixed the
	// same way as, doctype/cipc.cxx's BUGFIX #3.
	if (Nested.GetSize() != 0) {
	  pTmp = (PCIP_Element)Nested.Top();
	  if (Tag == pTmp->get_tag()) {
	    pTmp = (PCIP_Element)Nested.Pop();
	    delete pTmp;
	    if (Nested.GetSize() != 0) {
	      pTmp = (PCIP_Element)Nested.Top();
	      x = FullFieldname.SearchReverse('_');
	      FullFieldname.EraseAfter(x-1);
	    }
	  }
	}

      } else
	InCustom=GDT_FALSE;

      continue;
    } 

    const CHR *p = find_end_tag (tags_ptr, *tags_ptr);
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
	PCIP_Element pTag = new CIP_Element();
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

	  // Removed here (same as doctype/cipc.cxx): a dead `if
	  // (Nested.GetSize()!=0) { ... if (val_start < LastEnd) pTmp =
	  // Nested.Top(); }` block (and the LastEnd variable it was the
	  // only reader of) -- pTmp's value was never used afterward, so
	  // the whole block, including the LastEnd comparison, had no
	  // observable effect. Surfaced by -Wunused-but-set-variable while
	  // bringing this file to a clean -Wall -Wextra build for the
	  // first time.

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

  // BUGFIX #4 (docs/BUG_CATALOG.md#doctypecippcxx): any tag left open
  // (pushed via Nested.Push() above but never matched by a closing tag
  // -- e.g. overlapping/non-LIFO-nested tags like "<A><B></A></B>",
  // where </A> can never match while B is on top of the stack) stayed
  // on Nested forever; Nested only Pop()s+delete()s an element when it
  // finds a matching close. Drain what's left before this local GSTACK
  // goes out of scope, or every such stuck tag leaks its CIP_Element.
  // Same bug as, and fixed the same way as, doctype/cipc.cxx's
  // BUGFIX #4.
  while (Nested.GetSize() != 0) {
    PCIP_Element pLeftover = (PCIP_Element)Nested.Pop();
    delete pLeftover;
  }

  // Clean up;
  delete [] tags;
  delete pdft;
  delete[]RecBuffer;
}


GDT_BOOLEAN 
CIPP::GetCleanedFieldData(const RESULT& ResultRecord, 
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


// ElementSet BRIEF_MAGIC ("B") returns just the ItemDescriptorId; "C"
// returns an HTML-ish ItemDescriptorId paragraph; "S" returns a
// multi-field "KEY=value" summary block; "F" returns the raw record
// file contents; anything else falls back to SGMLNORM::Present().
void
CIPP::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& /* RecordSyntax */, STRING *StringBuffer)
{
  STRING FieldName;
  
  if (ElementSet.Equals(BRIEF_MAGIC)) {
    STRLIST Strlist;
    STRING Title;
    GDT_BOOLEAN Status;
    FieldName = "Provider-Defined-Product-Descriptor_ItemDescriptorId";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "(title not found)";
    *StringBuffer = Title;
   // added for title-geoform element set identified by C dbergstedt

   } else if (ElementSet.CaseEquals("C")) {
     STRLIST Strlist;
     STRING Hold, FieldType,ESN_C;
     STRING Title,GeoForm;
     GDT_BOOLEAN Status;
 
     ESN_C = "";
 
     FieldName = "Provider-Defined-Product-Descriptor_ItemDescriptorId";
     Db->FieldTypes.GetValue(FieldName,&FieldType);
     Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
     if (Status) {
       Hold = "<P><B>ItemDescriptorId: </B>";
       Hold.Cat(Title);
       Hold.Cat("</P>\n");
       ESN_C.Cat(Hold);
     }
 
     *StringBuffer = ESN_C;

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

    ESN_S = "";

    FieldName = "Provider-Defined-Product-Descriptor_ItemDescriptorId";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
    if (Status) {
      Hold = "TITLE=";
      Hold.Cat(Title);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    } 

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_SPATIALCOVERAGE_BOUNDINGRECTANGLE_WESTBOUNDINGCOORDINATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, West);
    if (Status) {
      Hold = "WestBoundingCoordinate=";
      Hold.Cat(West);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_SPATIALCOVERAGE_BOUNDINGRECTANGLE_EASTBOUNDINGCOORDINATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, East);
    if (Status) {
      Hold = "EastBoundingCoordinate=";
      Hold.Cat(East);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_SPATIALCOVERAGE_BOUNDINGRECTANGLE_NORTHBOUNDINGCOORDINATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, North);
    if (Status) {
      Hold = "NorthBoundingCoordinate=";
      Hold.Cat(North);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_SPATIALCOVERAGE_BOUNDINGRECTANGLE_SOUTHBOUNDINGCOORDINATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, South);
    if (Status) {
      Hold = "SouthBoundingCoordinate=";
      Hold.Cat(South);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_TEMPORALCOVERAGE_TEMPORALRANGE_STARTDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, BegDate);
    if (Status) {
      Hold = "StartDate=";
      Hold.Cat(BegDate);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    FieldName = "PROVIDER-DEFINED-PRODUCT-DESCRIPTOR_TEMPORALCOVERAGE_TEMPORALRANGE_ENDDATE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, EndDate);
    if (Status) {
      Hold = "EndDate=";
      Hold.Cat(EndDate);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }

    *StringBuffer = ESN_S;

  } else if (ElementSet.CaseEquals("F")) {
    STRING FullFilename, HoldFilename;
    STRING b;

    STRLIST StrList;
    STRING mpCommand;
    //    Db->GetDocTypeOptions(&StrList);

    //    StrList.GetValue("filter", &mpCommand);

    ResultRecord.GetFullFileName(&FullFilename);
    b.ReadFile(FullFilename);
    if (b.GetLength() <= 0)
      *StringBuffer = "";
    else
      *StringBuffer = b;
  } else {
    SGMLNORM::Present (ResultRecord, ElementSet, StringBuffer);
  }
  return;
  
}

CIPP::~CIPP ()
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
CIPP::parse_tags (CHR *b, GPTYPE len) const
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
#if CIPP_ACCEPT_EMPTY_TAGS
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
  
  t[tc] = (CHR*) nullptr;	// Mark end of list
  return t;
}


/*
   Searches through string list t look for "/" followed by tag, e.g. if
   tag = "TITLE REL=XXX", looks for "/TITLE" or a empty end tag (</>).
   
   Pre: t is is list of string pointers each NULL-terminated.  The list
   should be terminated with a NULL character pointer.
   
   Post: Returns a pointer to found string or NULL.
   */


const CHR* 
CIPP::find_end_tag (char **t, const char *tag) const
{
  size_t len;
  if (t == nullptr || *t == nullptr)
    return nullptr;		// Error
  
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
#if CIPP_ACCEPT_EMPTY_TAGS
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
