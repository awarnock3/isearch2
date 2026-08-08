// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/* $Id: dif.cxx,v 1.18 2000/10/12 20:55:25 cnidr Exp $ */
/*@@@
File:		dif.cxx
Version:	1.02
$Revision: 1.18 $
Description:	Class DIF - Colon-delimited Text w/ static output files
Author:		Archie Warnock, warnock@clark.net
                Bug fixes from Ed Zimmerma
		Enhancements by Ken Lambert Hughes STX 3/97
		Enhancements by Chris Gokey Hughes STX 3/98
@@@*/

/**
 * @file dif.cxx
 * @brief Implements DIF, the GCMD/DIF (Directory Interchange Format) DOCTYPE.
 *
 * DIF documents are "Fieldname: value" lines and "Group: name ...
 * End_Group" blocks. This file implements a small hand-written
 * recursive-descent parser over that grammar:
 *
 * @code
 * [START]     --> [ATOM] [ATOMTAIL] | lambda
 * [ATOMTAIL]  --> [ATOM] [ATOMTAIL] | lambda
 * [ATOM]      --> [GROUP] | [FIELD]
 * [FIELD]     --> FieldType TextType
 * [GROUP]     --> GroupType FieldWithoutColon [GROUPBODY] EndGroupType
 * [GROUPBODY] --> [ATOMTAIL] | [ML]
 * [ML]        --> textMLType [ML] | EndGroupType
 * @endcode
 *
 * start()/atom()/atomtail()/field()/group()/groupbody()/textML() each
 * implement one non-terminal above. They're driven by nextToken() (a
 * small hand-rolled DFA switching on DIF::state), which in turn reads
 * characters through a scanner (sgetc()/sungetc()/tell()) over
 * RecBuffer, bounds-checked against RecBufferLen (set once in
 * ParseFields(); see sgetc()'s doc comment for why that bound exists
 * and what it fixed). Matched fields are recorded via writeField(),
 * which normalizes a handful of DIF field names to the FGDC-style
 * names the rest of the tree searches on (e.g. "Northernmost_Latitude"
 * -> "NORTHBC").
 */
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "glist.hxx"
#include "gstack.hxx"
#include "strstack.hxx"
#include "dif.hxx"
// to get pid
#if defined(_MSDOS) || defined(_WIN32)
#include <io.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#define MaxDIFSize 50000
#define SUTRS_OID  "1.2.840.10003.5.101"
#define USMARC_OID "1.2.840.10003.5.10"
#define HTML_OID   "1.2.840.10003.5.1000.34.1"
#define SGML_OID   "1.2.840.10003.5.1000.34.2"
#define FGDCHTML_OID   "1.2.840.10003.5.1000.34.3"
#define GILSHTML_OID   "1.2.840.10003.5.1000.34.4"
#define RAW_OID   "1.2.840.10003.5.1000.34.5"
#define FULL_BRIEF_OID "1.2.840.10003.5.1000.34.6"
#define FGDC_MP_OID   "1.2.840.10003.5.1000.34.7"
#define NBII_ASCII_OID   "1.2.840.10003.5.1000.34.8"
#define HTML_DICT "display_html.dic"
#define NEW_HTML_OID   "1.2.840.10003.5.108"
#define SUPP_OID "1.2.840.10003.5.1000.34.9"
#define DIFHTML_OID "1.2.840.10003.5.1000.34.10"
#define DICT_ENV 0
#define DICT_COMPILED 1
/*
  const int eofType        = 0;
  const int fieldType      = 1;
  const int groupType      = 2;
  const int textMLType     = 3;
  const int textType       = 4;
  const int fldWOcolonType = 5;
  const int endGroupType   = 6;
  const int errorType      = 7;
*/
typedef INT* PINT;
#define NO_MULTILINE_GROUPS 11
char multilineGroup[NO_MULTILINE_GROUPS][25] = { "Quality",
						 "Access_Constraints",
						 "Use_Constraints",
						 "Description",
						 "Reference",
						 "Summary",
						 "Address",
                                                 "Data_Center_Text",
                                                 "Project_Text",
                                                 "Source_Text",
                                                 "Sensor_Text" };
/**
 * @brief Parser trace hook; a permanent no-op (its printf is commented
 * out below), left in place rather than removed since the parser calls
 * it at almost every grammar rule.
 * @param s Trace message (unused while disabled).
 */
void dbg(const char * /* s */) {
  // printf("%s\n",s);
}
/* ========================= FROM FGDC doctype ========================*/
/**
 * @brief Fetches a field's data and strips embedded newlines/carriage
 * returns so it's safe to display on a single line.
 * @param ResultRecord The result record to read the field from.
 * @param FieldName Name of the field to fetch.
 * @param FieldType Type of the field (passed through to GetFieldData()).
 * @param Buffer Receives the cleaned field text, or "(not found)" if
 * the field isn't present.
 * @return Whatever Db->GetFieldData() returned (true if the field was
 * found).
 */
GDT_BOOLEAN
DIF::GetCleanedFieldData(const RESULT& ResultRecord,
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
/**
 * @brief Loads the FIELDTYPE file (the `-o fieldtype=<filename>` doctype
 * option) into Db->FieldTypes, one "FIELDNAME TYPE" entry per line.
 *
 * Reads the whole file into memory and tokenizes it with strtok() on
 * newlines. If no FIELDTYPE option was given, or the named file can't
 * be opened, logs a message and returns with every field left to
 * default to type "text" (see writeField()).
 */
void DIF::LoadFieldTable() {
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
  fseek(fp, 0, 2);
  RecStart = 0;
  RecEnd = ftell(fp);
  fseek(fp, RecStart, 0);
  len = RecEnd - RecStart;
  b = new CHR[len + 1];
  ActualLength = fread(b, 1, len, fp);
  b[ActualLength] = '\0';
  fclose(fp);
  pBuf = strtok(b,"\n");
  // BUGFIX #5 (docs/BUG_CATALOG.md#doctypedifcxx): this was a
  // do-while, unconditionally running the body (and thus
  // `Field_and_Type = pBuf;`) once before ever checking pBuf. An
  // empty (but existing) FIELDTYPE file makes strtok() return nullptr
  // on the very first call, and STRING::operator=(const CHR*) calls
  // strlen() on it unconditionally -- a null-pointer-dereference
  // crash. Same bug as, and fixed the same way as,
  // doctype/cipc.cxx's BUGFIX #5.
  while (pBuf) {
    Field_and_Type = pBuf;
    Field_and_Type.UpperCase();
    Db->FieldTypes.AddEntry(Field_and_Type);
    //cout << "Write field to dfd -> " << Field_and_Type << endl;
    pBuf = strtok((CHR*)nullptr,"\n");
  }
  delete [] b;
}
//
// Overrides DOCTYPE's ParseNumeric method
//
/**
 * @brief Parses a GCMD-style coordinate string ("34.5N", "12.5 W", ...)
 * into a signed decimal degree value.
 *
 * Strips spaces and the N/S/E/W (or lowercase) hemisphere letter; a
 * "S" or "W" suffix makes the value negative (via `Insert(1,"-")`).
 * @param Buffer Coordinate text to parse.
 * @return The parsed value, or 0 if what's left after stripping isn't
 * a valid number.
 */
DOUBLE DIF::ParseNumeric(const CHR *Buffer){
  STRING Hold;
  Hold = Buffer;
  //GCMD NEW CODE
  Hold.Replace(" ","");
  Hold.Replace("N","");
  if (Hold.Replace("S","")) Hold.Insert(1,"-");
  Hold.Replace("E","");
  if (Hold.Replace("W","")) Hold.Insert(1,"-");
  Hold.Replace("n","");
  if (Hold.Replace("s","")) Hold.Insert(1,"-");
  Hold.Replace("e","");
  if (Hold.Replace("w","")) Hold.Insert(1,"-");
  if (Hold.IsNumber())
    return(Hold.GetFloat());
  else
    return 0;
}
/**
 * @brief Parses a single DIF date value into both @p fStart and
 * @p fEnd (a single date has no range, so both outputs get the same
 * value).
 *
 * Recognizes the keywords "present"/"unknown" (case-insensitive) as
 * DATE_PRESENT/DATE_UNKNOWN; a plain numeric value (dashes stripped)
 * is parsed as-is; anything else yields DATE_ERROR.
 * @param Buffer Date text to parse.
 * @param fStart Receives the parsed value.
 * @param fEnd Receives the same parsed value as @p fStart.
 */
void DIF::ParseDate(const CHR *Buffer, DOUBLE* fStart, DOUBLE* fEnd) {
  STRING Hold;
  // cout << "ParseDate:" << Buffer << endl;
  Hold = Buffer;
  Hold.Replace("-","");
  if (Hold.CaseEquals("present")) {
    *fStart = DATE_PRESENT;
    *fEnd = *fStart;
    return;
  }
  else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd = *fStart;
    return;
  }
  else if (Hold.IsNumber()) {
    *fStart = Hold.GetFloat();
    *fEnd = *fStart;
    return;
  }
  else {  
    *fStart = DATE_ERROR;
    *fEnd = *fStart;
    return;
  }
};
//
// From FGDC Document type (with DIF specific modifications)
//
/**
 * @brief Parses a single date value to a sortable numeric form, using
 * DIF's own (non-DATE_PRESENT/DATE_UNKNOWN) sentinel convention:
 * "present" -> 99999999, "unknown" or anything unparseable -> -1.0.
 * @param Buffer Date text to parse.
 * @return The parsed value, or one of the two sentinels above.
 */
DOUBLE DIF::ParseDateSingle(const CHR *Buffer) {
  DOUBLE fVal;
  STRING Hold;
  // BUGFIX #3 (docs/BUG_CATALOG.md#doctypedifcxx): this unconditional
  // debug print read fVal before it was ever assigned on any path
  // below (a real "used uninitialized" per the compiler, not just
  // stylistic) -- removed; the properly #ifdef DEBUG-guarded print
  // right below already covers the "was this called" question.
#ifdef DEBUG
  cout << "Parse Single Date." << endl;
#endif
  Hold = Buffer;
  //cout << "Hold = " << Hold << "." << endl;
  Hold.Replace("-","");
  //cout << "Hold = " << Hold << "." << endl;
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
  if (fVal == 0) fVal = 99999999;
  return fVal;
}
/**
 * @brief Parses a DIF date range ("START_DATE: ... \n STOP_DATE: ...")
 * into @p fStart / @p fEnd.
 *
 * Searches @p Buffer (uppercased, dashes stripped) for the
 * "START_DATE: " and "STOP_DATE: " labels independently. A missing
 * START_DATE is an error for the whole range (both outputs set to
 * DATE_ERROR); a missing STOP_DATE defaults to DATE_PRESENT (an
 * open-ended range). Year/month-only values are promoted to a full
 * day boundary (start of year/month for @p fStart, end for @p fEnd)
 * via SRCH_DATE::PromoteToDayStart()/PromoteToDayEnd().
 * @param Buffer Text containing the START_DATE/STOP_DATE labels.
 * @param fStart Receives the parsed start date, or DATE_ERROR/
 * DATE_UNKNOWN.
 * @param fEnd Receives the parsed end date, DATE_PRESENT if absent, or
 * DATE_UNKNOWN.
 */
void
DIF::ParseDateRange(const CHR *Buffer, DOUBLE* fStart,
		DOUBLE* fEnd) {
  SRCH_DATE dStart,dEnd;
  STRING Hold;
  STRINGINDEX Start, End;
  Hold = Buffer;
  Hold.UpperCase();
  Hold.Replace("-","");
  Start = Hold.Search("START_DATE: ");
  if (Start > 0) {                 // Found the opening tag
    Start += strlen("START_DATE: ");
    Hold.EraseBefore(Start);
    
    End = Hold.Search("\n");
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
      cerr << "[DIF::ParseDateRange] Didn't parse START_DATE, value=" 
	   << Hold << endl;
      *fStart = DATE_ERROR;
    }
  }
  else {
    cerr << "[DIF::ParseDateRange] Didn't parse START_DATE, value=" 
	 << Hold << endl;
    *fStart = DATE_ERROR;
    *fEnd = *fStart;
    return;
  }    
  // Copy the input buffer again so we can look for the stop_date tag
  Hold = Buffer;
  Hold.UpperCase();
  Hold.Replace("-","");
  
  Start = Hold.Search("STOP_DATE: ");
  if (Start > 0) {
    Start += strlen("STOP_DATE: ");
    Hold.EraseBefore(Start);
    End = Hold.Search("\n");    
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
      *fEnd = dEnd.GetValue();
    }
  } else {
    // STOP_DATE not found, assign it DATE_PRESENT
    *fEnd = DATE_PRESENT;
  }
  // cout << "[DIF::ParseDateRange]" << endl;
  // cout << "fstart = " << (long)*fStart << "." << endl;
  // cout << "fend = " << (long)*fEnd << "." << endl << endl;
  return;
}
// Local prototypes
// BUGFIX #2 (docs/BUG_CATALOG.md#doctypedifcxx): RecBuffer/pos/state/
// status/toktype/RecBufferLen were all left uninitialized here. Every
// one is set at the top of ParseFields() before use, so this was
// never reachable as a live bug (nothing else in this class touches
// them first -- ~DIF() is empty), but it's the same class of fix as
// every other "constructor leaves members uninitialized" turn this
// session (e.g. src/gstack.cxx, src/index.cxx).
/**
 * @brief Constructs a DIF doctype handler for database @p DbParent.
 * @param DbParent The owning database object (forwarded to COLONDOC).
 */
DIF::DIF(PIDBOBJ DbParent) : COLONDOC(DbParent), RecBuffer(nullptr),
  RecBufferLen(0), state(0), status(0), toktype(eofType), pos(0),
  count(1), pdft(nullptr) {
}
/**
 * @brief Formats a DIF record for display, per the requested element
 * set and record syntax.
 *
 * Element sets: "G" (brief hit-list line: Entry_ID + Entry_Title, HTML
 * `<LI>`-wrapped for HTML-family syntaxes), "B" (colon-joined
 * "EntryID:Title"), "I" (Entry_ID only, HTML-wrapped for HTML-family
 * syntaxes), "S" (a small GEO-profile-style field=value listing), and
 * the default/full element set (the raw indexed record, optionally
 * piped through an external `docmorph.pl` transform when compiled with
 * `USE_DIFMORPH` and an HTML-family syntax was requested — not the
 * default build).
 * @param ResultRecord The record to present.
 * @param ElementSet Which element set to render ("G", "B", "I", "S",
 * or anything else for the full record).
 * @param RecordSyntax Requested output syntax (HTML/XML/SUTRS/etc. OID),
 * used to decide HTML wrapping and (in the full-record branch) which
 * morph dictionary to use.
 * @param StringBufferPtr Receives the formatted output.
 */
void DIF::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		     const STRING& RecordSyntax, PSTRING StringBufferPtr)
{
  *StringBufferPtr = "";
  if (ElementSet.Equals("G")) {       //Brief DIF presentation (hit list)
    STRLIST Strlist;
    STRING TitleTag,Title,EntryIDTag,EntryID;

    EntryIDTag = "Entry_ID";  //Send in brief for statistics informatio
    Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist);
    Strlist.Join("\n",&EntryID);
    EntryID.Replace("\n"," ");
    EntryID.Replace("\r"," ");
    TitleTag = "Entry_Title";  //Brief headline is "title"
    Db->GetFieldData(ResultRecord, TitleTag, &Strlist);
    Strlist.Join("\n",&Title);
    Title.Replace("\n"," ");
    Title.Replace("\r"," ");
    if ((RecordSyntax.Equals(HTML_OID)) || (RecordSyntax.Equals(FGDCHTML_OID))
       || (RecordSyntax.Equals(GILSHTML_OID)) || (RecordSyntax.Equals(DIFHTML_OID)) 
       || (RecordSyntax.Equals(RAW_OID))
       || (RecordSyntax.Equals(FGDC_MP_OID))
       || (RecordSyntax.Equals(NBII_ASCII_OID))){
      STRING Temp;
      Temp.Cat(EntryID);
      Temp.Cat("<LI>");
      Temp.Cat(Title);
      *StringBufferPtr = Temp;
    } else
      *StringBufferPtr = Title;
  } else if (ElementSet.Equals("B")) {
    STRLIST Strlist1,Strlist2;
    STRING TitleTag,Title,EntryIDTag,EntryID;
    GDT_BOOLEAN Status1;
    TitleTag = "Entry_Title";
    EntryIDTag = "Entry_ID";
    Status1 = Db->GetFieldData(ResultRecord, TitleTag, &Strlist1);
    Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist2);

    if (Status1) {
      Strlist1.Join("",&Title);
      Strlist2.Join("",&EntryID);
    } else
      Title = "No Title";
    STRING Temp = "";
    Temp+=EntryID;
    Temp+=":";
    Temp+=Title;
    *StringBufferPtr = Temp;
    return;
  
  } else if (ElementSet.Equals("I")) {
    STRLIST Strlist;
    STRING EntryIDTag,EntryID;

    EntryIDTag = "Entry_ID";  //Send in brief for statistics informatio
    Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist);
    Strlist.Join("\n",&EntryID);
    EntryID.Replace("\n"," ");
    EntryID.Replace("\r"," ");
    STRING Temp;
    Temp.Cat(EntryID);
    if ((RecordSyntax.Equals(HTML_OID)) || (RecordSyntax.Equals(FGDCHTML_OID))
        || (RecordSyntax.Equals(GILSHTML_OID)) || (RecordSyntax.Equals(DIFHTML_OID))
        || (RecordSyntax.Equals(NBII_ASCII_OID))
        || (RecordSyntax.Equals(FGDC_MP_OID))) {
                Temp.Cat("<LI>");
                Temp.Cat(EntryID);
    }
  } else if (ElementSet.Equals("S")) {
/*  These fields define the S element set
         Title
         Editio
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
    STRING FieldName;
    STRING EntryIDTag,EntryID;
    STRING Hold, FieldType,ESN_S;
    STRING Title,Edition,GeoForm,Spatial,West,East,North,South;
    STRING BegDate,EndDate,CalDate,Update,BrowseGraphic;
    GDT_BOOLEAN Status;
    /*
      EntryIDTag = "Entry_ID";  //Send in brief for statistics informatio
      Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist);
      Strlist.Join("\n",&EntryID);
      EntryID.Replace("\n"," ");
      EntryID.Replace("\r"," ");
      STRING Temp;
      Temp.Cat(EntryID);
      if ((RecordSyntax.Equals(HTML_OID)) 
      || (RecordSyntax.Equals(FGDCHTML_OID))
      || (RecordSyntax.Equals(GILSHTML_OID)) 
      || (RecordSyntax.Equals(DIFHTML_OID))
      || (RecordSyntax.Equals(NBII_ASCII_OID))
      || (RecordSyntax.Equals(FGDC_MP_OID))) {
      Temp.Cat("<LI>");
      Temp.Cat(EntryID);
      }
      Temp.Cat("<br>");
    */
    ESN_S = "";
    FieldName = "ENTRY_ID";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
    if (Status) {
      Hold = "LOCAL_IDENTIFIER=";
      Hold.Cat(Title);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    } 
    FieldName = "ENTRY_TITLE";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Title);
    if (Status) {
      Hold = "TITLE=";
      Hold.Cat(Title);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    } 
    FieldName = "RELATED_URL";
    Db->FieldTypes.GetValue(FieldName,&FieldType);
    Status = GetCleanedFieldData(ResultRecord, FieldName, FieldType, Edition);
    if (Status) {
      Hold = "LINKAGE=";
      Hold.Cat(Edition);
      Hold.Cat("\n");
      ESN_S.Cat(Hold);
    }
    *StringBufferPtr = ESN_S;
    //    *StringBufferPtr = Temp;
 
  // 
  // Display the full DIF!
  // 
  } else { 
    STRING TitleTag, Title;
    CHR *pRawData;
    STRING DataBuffer;
    STRLIST Strlist;
    STRING EntryIDTag,EntryID;

    EntryIDTag = "Entry_ID";  //Send in brief for statistics informatio
    Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist);
    Strlist.Join("\n",&EntryID);
    EntryID.Replace("\n"," ");
    EntryID.Replace("\r"," ");
    ResultRecord.GetRecordData(&DataBuffer);
    pRawData = DataBuffer.NewCString();
    //HTML Header informatio
    DataBuffer="";
    if (!RecordSyntax.Equals(SUTRS_OID)) {
      DataBuffer="<html>";
      DataBuffer+="<TITLE>GCMD Data Set Description</TITLE><BODY TEXT=\"#000000\" BGCOLOR=\"#FFFFFF\" LINK=\"#006666\" VLINK=\"#993300\" ALINK=\"#FF0000\">";
#ifdef IMGPATH
      DataBuffer+="<center><table><tr><td>";
      DataBuffer+="<img src=\"http://";
      DataBuffer+=IMGPATH;
      DataBuffer+="/gcmd_icon.gif\" alt=\"[GCMD logo]\">";
      DataBuffer+="</TD><TD><P><h1>Data Set Description</h1></TD></TR></TABLE>";
      DataBuffer+="<br></center>";
#endif
    }
#ifdef AGGREGATIO
    /** ROUTINES TO HANDLE PARENT/CHILD RELATIONSHIP **/
    char *temp=EntryID.NewCString();
    // search the parent_list.txt file and see if the entry_id we are presenting has
    // children.  Later, when the # of parents in the text file grows very
    // large, a faster search algoritm maybe needed (like a binary search
    // on a sorted list of entry_ids).
    char buffer[1024];
    ifstream infile("list_of_parent_difs.txt");
    while (infile) {
      infile.getline(buffer,1024);
      if (!strcmp(temp,buffer)) {       
	// add butto
	char child_link[1024];
	snprintf(child_link,sizeof(child_link),"<CENTER><A HREF=\"/cgi-bin/md/zgatedriver.pl?ESNAME=B&SERVICE=SEARCH&DBNAME=CHILD&ATTRSET=1.2.840.10003.3.4&USE_1=3704&maxrecords=15&RECSYNTAX=1.2.840.10003.5.1000.34.10&TERM_1=%s&ACTION=SEARCH\"><img border=0 src=\"http://%s/children_button.gif\" alt=\"This entry has subsets, Click for a list\"></img></A></CENTER>",temp,IMGPATH);
	delete [] temp;
	DataBuffer+=child_link;
	break;
      }
    }
    infile.close();
#endif
    
    if (RecordSyntax.Equals(HTML_OID) ||
	RecordSyntax.Equals(DIFHTML_OID) ||
	RecordSyntax.Equals(FGDCHTML_OID) || 
	RecordSyntax.Equals(GILSHTML_OID) ||
	RecordSyntax.Equals(FGDC_MP_OID) ||
	RecordSyntax.Equals(NBII_ASCII_OID)) {
      //Open the MorphFile
#ifndef USE_DIFMORPH
     DataBuffer.Cat("<pre>");
     DataBuffer.Cat(pRawData);
     DataBuffer.Cat("</pre>");
     //     fclose(fp);
#else
      //
      //Morph the DIF - assume always FGDC (for now)
      //     transform() reads TempFile (raw DIF), morphs, and
      //     places result into mfp
      //
      
     char dictfile[1024];
     strcpy(dictfile,"dif_to_dif-display-html.dic");
     
     if (RecordSyntax.Equals(DIFHTML_OID))
       strcpy(dictfile,"dif_to_dif-display-html.dic");
     if (RecordSyntax.Equals(FGDCHTML_OID))
       strcpy(dictfile,"dif_to_fgdc-mp-html.dic");
     if (RecordSyntax.Equals(GILSHTML_OID))
       strcpy(dictfile,"dif_to_gils-html.dic");
     if (RecordSyntax.Equals(FGDC_MP_OID))
       strcpy(dictfile,"dif_to_fgdc-mp.dic");
     if (RecordSyntax.Equals(NBII_ASCII_OID))
       strcpy(dictfile,"dif_to_nbii-ascii.dic");
     
     char command[1024];
     char line[102400];
     FILE *fp;
     // Write raw DIF buffer to a temporary file
     char *TempFile = new CHR[256];
     snprintf(TempFile, 256, "/tmp/rawdif.%d", getpid());
     fp = fopen(TempFile, "w");
     fprintf(fp, "%s", pRawData);
     fclose(fp);
     snprintf(command,sizeof(command),"docmorph.pl -document=%s -dictionary=%s -format=colon-dif", TempFile, dictfile);
     fp = popen(command,"r");
     
     if (fp != nullptr) {
       while ((fgets(line,102400,fp) != nullptr)) {
	 DataBuffer.Cat(line);
       }
       pclose(fp);
     }
     // BUGFIX #6 (docs/BUG_CATALOG.md#doctypedifcxx): TempFile (the
     // new CHR[256] above) was never freed -- a leak on every record
     // presented through this (USE_DIFMORPH-gated, not compiled by
     // default in this build) path.
     delete [] TempFile;
#endif
    } else {
      DataBuffer.Cat(pRawData);
    }
    //Copy in trail info
    if (!RecordSyntax.Equals(SUTRS_OID)) 
      DataBuffer.Cat("</pre></body></html>");
    *StringBufferPtr = DataBuffer;
    //Clean up
    delete [] pRawData;
    return;
  }
}
/**
 * @brief Destroys the DIF handler. No owned resources to release —
 * RecBuffer/pdft are freed at the end of ParseFields() itself, not
 * held past it.
 */
DIF::~DIF() {
}
/**
 * @brief Reads @p NewRecord's bytes off disk into RecBuffer, runs the
 * recursive-descent parser (start()) over them, and attaches the
 * resulting field table (DFT) to @p NewRecord.
 *
 * Adds one DF entry per matched field, each with the byte-offset range
 * (start, end) of its value within the record. RecBufferLen is set
 * here, right after RecBuffer is allocated and NUL-terminated — see
 * sgetc()'s doc comment for how the scanner uses it to stay in bounds.
 * @param NewRecord Record to parse and attach field data to. A
 * nullptr is a silent no-op.
 */
void DIF::ParseFields (PRECORD NewRecord)
{
  PFILE fp;
  STRING fn;
  pdft = new DFT();
  state=0;
  status=true;
  pos=0;
  STRING token;
  if (NewRecord == (RECORD*)nullptr) 
    return;                      // ERROR
  // Open the file
  NewRecord->GetFullFileName (&fn);
  if (!(fp = fopen (fn, "r")))
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
  RecBuffer = (char *)calloc(RecLength+1,1);
  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ
  RecBufferLen = (int)ActualLength;
  fclose (fp);
  start();
  NewRecord->SetDft(*pdft);
  free(RecBuffer);
  delete pdft;
}
/*
 * Functions directly corresponding to non-terminals
 * for the grammar.
 *
1    [START]     --> [ATOM] [ATOMTAIL]
2                --> lamda
3    [ATOMTAIL]  --> [ATOM] [ATOMTAIL]
4                --> lamda
5    [ATOM]      --> [GROUP]
6                --> [FIELD]
7    [FIELD]     --> FieldType TextType
 
8    [GROUP]     --> GroupType FieldWithoutColon [GROUPBODY] EndGroupType
9    [GROUPBODY] --> [ATOMTAIL]
10               --> [ML]
11   [ML]        --> textMLType [ML]
12               --> EndGroupType
 *
 */
/*
 * Parse Table
 *
 
             Fld | Group | MLText | Text | FldWithoutColon | EndGroup | lamda
START         1      1                                                    2
ATOMTAIL      3      3                                           4        4
ATOM          6      5
FIELD         7      
GROUP                8
GROUPBODY     9      9       11                                  9
ML                           11                                 12
 *
 */
/**
 * @brief Grammar entry point: `[START] --> [ATOM] [ATOMTAIL] | lambda`
 * (see the file-level parse table above). Consumes the first token
 * itself, then dispatches on it.
 */
void DIF::start() {
  dbg("<start>");
  toktype = nextToken();
  // BUGFIX #4 (docs/BUG_CATALOG.md#doctypedifcxx): the fieldType and
  // groupType cases each used to end with two complementary ifs
  // (toktype != eofType -> ...+break; toktype == eofType -> break)
  // that always break either way, with no code path actually falling
  // through to the next case -- but the compiler can't prove that
  // (-Wimplicit-fallthrough), and it reads as if falling into
  // groupType/eofType were intentional. Collapsed to a single
  // unconditional break with the same effect, removing the ambiguity.
  switch (toktype){
  case fieldType :
    atom();
    if (toktype != eofType) {
      atomtail();
    }
    break;
  case groupType :
    atom();
    if (toktype != eofType) {
      atomtail();
    }
    break;
  case eofType :
    break;
  default :
    parserError("Error: expected <fieldtype>,<grouptype>,or <eof>");
    break;
  }
  dbg("</start>");
}
/**
 * @brief Grammar rule `[ATOM] --> [GROUP] | [FIELD]`: dispatches to
 * group() or field() based on the current token type.
 */
void DIF::atom() {
  dbg("<atom>");
  switch (toktype){
  case fieldType :
    // printf("field_value:%s\n",token.NewCString());
    field();
    break;
  case groupType :
    group();
    break;
  default :
    parserError("Error: expected <fieldtype>,<grouptype>");
    break;
  }
  dbg("</atom>");
}
/**
 * @brief Grammar rule `[ATOMTAIL] --> [ATOM] [ATOMTAIL] | lambda`:
 * recurses through atom()/atomtail() while more fields or groups
 * follow, and returns (lambda) on End_Group or EOF.
 */
void DIF::atomtail() {
  dbg("<atomtail>");
  switch (toktype){
  case fieldType :
    atom();
    atomtail();
    break;
  case groupType :
    atom();
    atomtail();
    break;
  case endGroupType  :
    break;
  case eofType 	   :
    break;
  default :
    // printf("token=%s,toktype=%d\n",token.NewCString(),toktype);
    parserError("Error: expected <fieldtype>,<grouptype>,or <eof>");
    break;
  }
  dbg("</atomtail>");
}
/**
 * @brief Grammar rule `[FIELD] --> FieldType TextType`: the current
 * token is the field name (already scanned as fieldType, e.g.
 * "Entry_ID:"); reads the following text-type token as its value and
 * records the pair via writeField(). Entry_ID fields are additionally
 * echoed to stdout with a running record count, for progress feedback
 * during indexing.
 */
void DIF::field() {
  dbg("<field>");
  STRING fld;
  fld = token;
  toktype=nextToken();
  // printf("field value=%s, toktype=%d\n",token.NewCString(),toktype);
  
  if (toktype == textType) {
    if (token.GetLength() > 0) {
      long start_value = tell()-token.GetLength();
      long stop_value = tell()-1;
      
      char *temp1 = fld.NewCString();
      if (fld.Search("Entry_ID:")) {
	char *temp2 = token.NewCString();
	fprintf(stdout,"(%d) %10s %s\n",(count++),temp1,temp2);
	delete [] temp2;
      }
      temp1[strlen(temp1)-1]='\0';    // chop off ':'
      writeField(temp1,start_value,stop_value);	  
      delete [] temp1;
    }      
    // printf("dbg:token=%s, toktype=%d\n",token.NewCString(),toktype);
    toktype=nextToken();
    dbg("</field>");
    return;
  }
  parserError("Error: Expected field value");
  dbg("</field>");
  return;
}
/**
 * @brief Grammar rule
 * `[GROUP] --> GroupType FieldWithoutColon [GROUPBODY] EndGroupType`:
 * reads the group name, parses its body via groupbody(), then requires
 * (and consumes) the matching End_Group token. Records the whole
 * group's text span (name through just before "End_Group") as one
 * field via writeField(), the same way field() records a single value.
 */
void DIF::group() {
  dbg("<group>");
  STRING fld;
  long start_value;
  long stop_value;
  
  toktype=nextToken();
  if (toktype != fldWOcolonType) parserError("Error: Expect a group name");
  fld = token;
  start_value=tell()+1;
  
  toktype=nextToken();
  groupbody();
  if (toktype != endGroupType)
    parserError("Error: Did not find end of group");
  stop_value=tell()-11;
  char *temp = fld.NewCString();
  writeField(temp,start_value,stop_value);
  delete [] temp;
  
  toktype=nextToken();
  dbg("</group>");
}
/**
 * @brief Grammar rule `[GROUPBODY] --> [ATOMTAIL] | [ML]`: a group's
 * body is either nested fields/groups (atomtail()) or multi-line free
 * text (textML()), decided by the current token type.
 */
void DIF::groupbody() {
  dbg("<groupbody>");
  switch (toktype){
  case fieldType :
    atomtail();
    break;
  case groupType :
    atomtail();
    break;
  case textMLType:
    textML();
    break;
  case endGroupType:
    break;
  default :
    parserError("Error: expected <fieldtype>,<grouptype>,or <text value>");
    break;
  }
  dbg("</groupbody>");
}
/**
 * @brief Grammar rule `[ML] --> textMLType [ML] | EndGroupType`:
 * consumes consecutive multi-line-text tokens (recursively) until the
 * closing End_Group token is reached.
 */
void DIF::textML() {
  dbg("<textML>");
  if (toktype == textMLType) {
    toktype=nextToken();
    dbg("</textML>");
    textML();
  }
  if (toktype == endGroupType) {
    dbg("</textML>");
    return;
  }
  parserError("Error: expected text or end_group");
  dbg("</textML>");
}
/**
 * @brief Reports a grammar-rule mismatch by printing @p s to stdout.
 * Not fatal — the parser continues (typically producing a partial or
 * skewed field table for a malformed record) rather than aborting.
 * @param s Error message to print.
 */
void DIF::parserError(const char *s) {  /* Parser error. */
	fprintf(stdout,"***** %s ***** \n", s);
}
/*
 * Simulate ftell, getc, and ungetc
 *
 */
/**
 * @brief Returns the scanner's current byte offset into RecBuffer.
 * @return Current offset (pos).
 */
long DIF::tell() {
  return pos;
}
// BUGFIX #1 (docs/BUG_CATALOG.md#doctypedifcxx): this used to advance
// pos and return RecBuffer[pos] with no bounds check at all, relying
// entirely on every caller stopping as soon as it saw the '\0'
// terminator. Most scanner methods do, but group() unconditionally
// calls nextToken() (which calls sgetc()) once more after
// groupbody() returns, even when groupbody() already hit EOF (e.g. a
// "Group:" with no matching "End_Group" before the file ends) --
// nextToken()'s own EOF path also doesn't call sungetc() to back off
// afterward, so pos was already one past the terminator, and this
// third call read further still. Confirmed a real heap-buffer-
// overflow with a standalone repro (an unclosed group) before fixing.
// Fixed by clamping pos to RecBufferLen (set once in ParseFields())
// before every read: a single sgetc()/sungetc() pair at EOF behaves
// exactly as before (advance to one past the terminator, then back to
// it), but any further unmatched sgetc() call re-clamps instead of
// advancing past the one-past-terminator position, so it always
// re-reads the terminator safely no matter how many extra calls happen.
/**
 * @brief Reads the next byte from RecBuffer and advances pos, clamping
 * pos to RecBufferLen first (see BUGFIX #1 above) so repeated reads
 * past EOF keep re-reading the terminator instead of running off the
 * end of the buffer.
 * @return The byte at the (possibly clamped) current position, as an
 * int (matches RecBuffer's `char`, sign-extended).
 */
long DIF::sgetc() {
  if (pos > RecBufferLen)
    pos = RecBufferLen;
  return (RecBuffer[pos++]);
}
/**
 * @brief Backs the scanner up one byte (pairs with sgetc()).
 */
void DIF::sungetc() {
  pos--;
}
/**
 * @brief Advances the scanner past any run of spaces, tabs, and
 * newlines, leaving pos positioned at the first non-whitespace byte
 * (or EOF).
 */
void DIF::moveNextWord() {
  int ch = sgetc();
  while (ch == ' ' || ch == '\t' || ch == '\n') {
    ch=sgetc();
  }
  sungetc();
}
/**
 * @brief Advances the scanner past spaces/tabs only (newlines are
 * significant here, unlike moveNextWord()), leaving pos at the first
 * non-space/tab byte.
 */
void DIF::skipWhitespace() {
  int ch = sgetc();
  while (ch == ' ' || ch == '\t') {
    ch=sgetc();
  }
  sungetc();
}
/**
 * @brief Reads one whitespace-delimited word into the `token` member
 * (appending), stopping at (and backing off from) the first space,
 * tab, newline, or NUL.
 */
void DIF::readWord() {
  int ch = sgetc();
  while (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\0') {
    token+=(char)ch;
    ch=sgetc();
  }
  sungetc();
}
/**
 * @brief Reads the rest of the current line into `token` (appending),
 * stopping at (and backing off from) a newline or NUL. An "&" followed
 * by a newline is treated as a line-continuation escape (the "&\n" is
 * dropped rather than copied into `token`, and scanning continues onto
 * the next line) — DIF's convention for wrapping long field values.
 */
void DIF::readNewLine() {
  skipWhitespace();
  int ch = sgetc();
  char buf[40];
  snprintf(buf,sizeof(buf),"rnl:%c,%d",ch,ch);
  dbg(buf);
  while (ch != '\n' && ch != '\0') {
    if (ch == '&') {
      ch = sgetc();
      if (ch == '\n') {
	ch = sgetc();
	continue;
      }
      else sungetc();
    }
    token+=(char)ch;
    ch=sgetc();
    snprintf(buf,sizeof(buf),"rnl:%c,%d",ch,ch);
    dbg(buf);
    if (ch == '\0') dbg("eof");
  }
  sungetc();
}     
/*
 * DFA used in nextToken()
 
 d(0,groupType)=1
 d(0,fldType)=3
 d(1,fldwithoutcolonType)=2
 d(2,groupType)=1
 d(2,fieldType)=3
 d(2,textmultilineType)=4
 d(3,textType)=0
 d(4,textmultilinetype)=4
 d(4,endGroupType)=0
 */
/**
 * @brief Retrieves the next token from the input stream, per the
 * small DFA above (states 0-4, keyed off DIF::state): state 0 is
 * "between fields," expecting a field name, "Group:", or "End_Group";
 * state 1 reads the group name after "Group:"; state 2 is inside a
 * group, expecting another field/group/End_Group, or — for the fixed
 * set of multi-line group names in `multilineGroup[]` — free text via
 * readNewLine(); state 3 reads a field's value as one line of text;
 * state 4 reads successive lines of multi-line group text until
 * End_Group. Stores the token's text in the `token` member.
 * @return The type of token found (see DIF::TokenType), or errorType
 * if `state` is out of range.
 */
enum DIF::TokenType DIF::nextToken() {
  //int DIF::nextToken() {       
  token = "";
  if (state != 3) moveNextWord();
  int ch = sgetc();
  if (ch == '\0') return eofType;
  else sungetc();
  
  switch (state) {
  case 0:
    {
      readWord();
      if (token.Equals("Group:")) {
	state = 1;
	return (groupType);
      }
      if (token.Search(':') ) {
	state=3;
	return (fieldType);
      }
      if (token.Equals("End_Group")) {
	state=0;
	return endGroupType;
      }
    }
    break;
  case 1:
    {
      readWord();
      groupName = token;
      state = 2;
      return (fldWOcolonType);
    }
    break;
  case 2: 
    {
      for (int i=0; i<NO_MULTILINE_GROUPS; i++) {
	if (groupName.Equals(multilineGroup[i])) {
	  readNewLine();
	  if (token.Search("End_Group")) {
	    state=0;
	    return endGroupType;
	  } 
	  else {
	    state=4;
	    return textMLType;	    
	  }	
	}
      }
      readWord();
      if (token.Equals("Group:")) {
	state = 1;
	return (groupType);
      }
      
      if (token.Search(":")) {
	state=3;
	return (fieldType);
      }
      if (token.Equals("End_Group")) {
	state=0;
	return endGroupType;
      }
    }
    break;
  case 3: 
    {
      readNewLine();
      state=0;
      return textType;
    }
    break;
  case 4:
    {
      readNewLine();
      if (token.Search("End_Group")) {
	state=0;
	return endGroupType;
      } 
      else {
	state=4;
	return textMLType;	    
      }
    }
    break;
  default:
    return errorType;
  }
  return errorType;
}
/**
 * @brief Records one parsed field: normalizes @p fld's name (strips
 * ":"/spaces, maps a handful of DIF names to the FGDC-style names the
 * rest of the tree searches on — e.g. "Northernmost_Latitude" ->
 * "NORTHBC" — then uppercases), looks up its type from
 * Db->FieldTypes (defaulting to "text"), and adds both a DFD entry and
 * a DF/FC entry spanning [@p start, @p stop] to `pdft`.
 * @param fld Raw field name as scanned (e.g. "Entry_ID:" or a group
 * name).
 * @param start Byte offset of the field value's first character in
 * RecBuffer.
 * @param stop Byte offset of the field value's last character in
 * RecBuffer.
 */
void DIF::writeField(char *fld, long start, long stop) {
  // if (stop <= start) printf("\n\n\n**** Error, stop <= start\n\n\n");
  FC fc;
  DF df;
  PFCT pfct = new FCT ();
  DFD dfd;
  STRING FieldName = fld;
  STRING FieldType;
  FieldName.Replace(":","");
  FieldName.Replace(" ","");
  if (FieldName.CaseEquals("Northernmost_Latitude")) {
    FieldName = "NORTHBC";
  }
  if (FieldName.CaseEquals("Southernmost_Latitude")) {
    FieldName = "SOUTHBC";
  }
  if (FieldName.CaseEquals("Easternmost_Longitude")) {
    FieldName = "EASTBC";
  }
  if (FieldName.CaseEquals("Westernmost_Longitude")) {
    FieldName = "WESTBC";
  }
  if (FieldName.CaseEquals("Spatial_Coverage")) {    
    FieldName = "bounding";
  }
  if (FieldName.CaseEquals("Temporal_Coverage")) {    
    FieldName = "rngdates";
  }
  FieldName.UpperCase();
  Db->FieldTypes.GetValue(FieldName, &FieldType);
  if (FieldType.Equals(""))
    FieldType = "text";
  dfd.SetFieldName (FieldName);
  dfd.SetFieldType (FieldType);
  Db->DfdtAddEntry (dfd);
  fc.SetFieldStart (start);
  fc.SetFieldEnd (stop);
  pfct->AddEntry (fc);
  df.SetFct (*pfct);
  df.SetFieldName (FieldName);
  pdft->AddEntry (df);
  delete pfct;
  /*
  fprintf(stdout,"************************");
  fprintf(stdout,"%25s%10ld%10ld\n",fld,start,stop);
  fprintf(stdout,"%s^^^",fld);
  for (int i=start; i<=stop; i++) fprintf(stdout,"%c",RecBuffer[i]);
  fprintf(stdout,"^^^\n");
  fprintf(stdout,"************************");
  */
}
