// $ID$
// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		marcdump.cxx
Version:	$Revision: 1.2 $
Description:	Class MARCDUMP - output from Yaz utility marcdump
Author:		Archibald Warnock (warnock@clark.net), A/WWW Enterprises
Copyright:	A/WWW Enterprises, Columbia, MD
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "termobj.hxx"
#include "sterm.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "operator.hxx"
#include "tokengen.hxx"

#include "marcdump.hxx"

// Local prototypes
static CHR **parse_tags (CHR *b, GPTYPE len);
int          usefulMarcDumpField(char *fieldStr);
void         marcdumpFieldNumToName(STRING& fieldStr, STRING *fieldName);
int          SplitLine(const STRING& TheLine, STRING* Tag, STRING* Buffer);
GDT_BOOLEAN  HasSubfield(const STRING& TheContents, STRING* tag);
GDT_BOOLEAN  HasSubfieldTag(const STRING& TheContents, STRING& tag);
GDT_BOOLEAN  GetSubfield(const STRING& TheContents, const STRING& SubfieldTag,
			 STRING* Buffer);
INT          GetSubfields(const STRING& TheContents, STRLIST* TheSubfields);


MARCDUMP::MARCDUMP (PIDBOBJ DbParent)
  : DOCTYPE (DbParent)
{
}


void 
MARCDUMP::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}


// Splits FileRecord's underlying marcdump file into one RECORD per
// MARC record, delimited by lines whose tag is "001" (see the
// in-function comment below for the exact scan), and adds each to Db
// via DocTypeAddRecord().
void
MARCDUMP::ParseRecords (const RECORD& FileRecord)
{

  //Finding the range of a MARC record is easy:  The first line always
  //starts with 001.  We assume that your files will consist of a bunch 
  //of records concatenated into one big file, so we:
  //   while not end of file do
  //       read three bytes
  //       if it's "001", assume we've reached the start of a new record
  //       indicate the previous range as a record
  //       increment the file pointer to the start of the next record.
  //   done

  STRING fn;
  FileRecord.GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp) {
    cout << "MARCDUMP::ParseRecords(): Could not access '" << fn << "'\n";
    return;			// File not accessed
  }
  
  RECORD Record;
  STRING s;
  FileRecord.GetPathName(&s);
  Record.SetPathName( s );

  FileRecord.GetFileName(&s);
  Record.SetFileName( s );

  FileRecord.GetDocumentType(&s);
  Record.SetDocumentType ( s );

  GPTYPE RS;

  if (fseek(fp, 0L, SEEK_END) == -1) {
    cout << "MARCDUMP::ParseRecords(): Seek failed - ";
    cout << fn << "\n";
    fclose(fp);
    return;	
  }
	
  GPTYPE FileStart, FileEnd, FileLength;
  FileStart = 0;
  FileEnd = ftell(fp);
  if(FileEnd == 0) {
    cout << "MARCDUMP::ParseRecords(): Skipping ";
    cout << " zero-length record -" << fn << "...\n";
    fclose(fp);
    return;
  }
  if(fseek(fp, (long)FileStart, SEEK_SET) == -1) {
    cout << "MARCDUMP::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
	
  FileLength = FileEnd - FileStart;

  GPTYPE bytePos = 0;    // we're going to start reading with bytePos = 0 so we
                      // always know how many bytes we've read.
  GPTYPE marcLength = 0; // we keep track of the record length
  RS=0;               // we also know the first record will begin @ 0.

  STRING StartTag;
  char LenBuff[16];
  char c;

  while (FileLength >= bytePos + 3) { // if there are more records to read
    // start reading until we get to the newline
    while (((c=getc(fp)) != '\n') && (FileLength >= bytePos+3)) {
      bytePos++;
      marcLength++;
    }

    // Presumably, we hit a newline.  Is the line following a new record
    // or not?  Check for the 001 tag
    
    LenBuff[0]=getc(fp);
    LenBuff[1]=getc(fp);
    LenBuff[2]=getc(fp);
    LenBuff[3]='\0';      // make a null-term string, and do it every time.
    bytePos +=3;          // we read 3 bytes, so increment our Pos by 3.

    StartTag = LenBuff;

    if (StartTag.Equals("001")) {
      // start of a new record
#ifdef DEBUG
      cout << "MARCDUMP::ParseRecords(): RecordStart=" << RS
	   << ", RecordEnd=" << RS+marcLength << endl;
#endif
      Record.SetRecordStart(RS);
      Record.SetRecordEnd(RS+marcLength); // former bug : 1 over end of buffer!

      Db->DocTypeAddRecord(Record);
      RS = RS+marcLength+1;
      marcLength = 3; // we already read 3 bytes
    } else {
      marcLength +=4;
    }
  }

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): the loop above
  // only calls DocTypeAddRecord() when it finds the *next* record's
  // "001" tag, so the last record in the file -- for a single-record
  // file, the *only* record -- never had a following "001" to trigger
  // that, and was silently dropped, never indexed at all. Flush it here
  // the same way the loop itself does, using the RS/marcLength already
  // accumulated for it. Confirmed via a before/after test-revert: with
  // this flush removed, a file with exactly one record produced zero
  // calls to DocTypeAddRecord().
  Record.SetRecordStart(RS);
  Record.SetRecordEnd(RS+marcLength);
  Db->DocTypeAddRecord(Record);
}


// Reads NewRecord's bytes off disk (or, if RecordEnd is unset, treats
// the whole file as a single record -- see BUGFIX #2), splits them
// into tag/value pairs via the file-local parse_tags(), and adds one
// DF field (name from UnifiedName(tag), plus a second alias name from
// marcdumpFieldNumToName() when one exists) per pair to NewRecord's
// DFT.
void
MARCDUMP::ParseFields (RECORD *NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp) {
    cout << "MARCDUMP::ParseFields(): Could not access '" << fn << endl;
    return;			// File not accessed
  }
  
  // Get the offsets to the start and end of the current record,
  // and figure out how big the record is
  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0) {
    fseek (fp, 0L, SEEK_END);
    RecStart = 0;
    // BUGFIX #2 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): this used to
    // be `ftell(fp) - 1`, the same off-by-one truncation already fixed
    // in doctype/colondoc.cxx's BUGFIX #1 -- confirmed via a before/
    // after test-revert to silently drop the last byte of every record
    // read through this fallback. GPTYPE is also unsigned (UINT4,
    // src/defs.hxx): for a genuinely empty file, ftell(fp) is 0, so the
    // old `- 1` underflowed to UINT_MAX, which then wrapped `RecLength
    // + 1` back to 0 too (`new CHR[RecLength + 1]` ended up allocating
    // 0 bytes), making the very next line (`RecBuffer[ActualLength] =
    // '\0'`) write one byte past a 0-byte heap allocation -- undefined
    // behavior regardless, though a standalone repro found this
    // particular build's allocator/ASan combination doesn't actually
    // flag it (likely allocator slack around a 0-byte `new[]`). Fixed
    // either way, since relying on that is not something to build on.
    RecEnd = ftell (fp);
  }
  fseek (fp, (long)RecStart, SEEK_SET);
  GPTYPE RecLength = RecEnd - RecStart;
  CHR *RecBuffer = new CHR[RecLength + 1];

  // Read the entire record into the buffer
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  fclose (fp);
  RecBuffer[ActualLength] = '\0';
  CHR tag[4];
  strncpy(tag,RecBuffer,3);
  tag[3]='\0';

  // BUGFIX #3 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): sscanf() leaves
  // StartTag completely untouched (confirmed with a standalone repro)
  // when `tag` doesn't parse as a leading integer -- e.g. an empty
  // RecBuffer, reachable via the RecEnd==0 fallback above once
  // BUGFIX #2 stops it from crashing first. Left uninitialized, the
  // `StartTag != 1` check below read indeterminate memory. Initialized
  // to 0, which safely fails that check and takes the existing "bogus
  // first tag" error path.
  INT StartTag = 0;
  sscanf(tag,"%d", &StartTag);  // turn the string into an int
  if (StartTag != 1) {
    cout << "MARCDUMP::ParseField(): Bogus first tag at RecStart = "
	 << RecStart << endl;
    // BUGFIX #4 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): this early
    // return used to skip freeing RecBuffer entirely -- a genuine
    // heap-memory leak on every "bogus first tag" record, confirmed via
    // ASan (`make tests-asan` caught a real 1-byte leak from this exact
    // path, exercised by the empty-file test above).
    delete [] RecBuffer;
    return;
  }

  PCHR *tags = parse_tags (RecBuffer, ActualLength);

  // Now we've got pairs of tags & values
  if (tags == nullptr || tags[0] == nullptr) {
    STRING doctype;
    NewRecord->GetDocumentType(&doctype);
    if (tags) {
      delete [] tags;
      cout << "Warning: No `" << doctype << "' fields/tags in \"" 
	   << fn << "\"\n";
    } else {
      cout << "Unable to parse `" << doctype << "' record in \"" 
	   << fn << "\"\n";
    }
    delete [] RecBuffer;
    return;
  }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName,newFieldName;

  // Walk though tags
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++) {
    strncpy(tag,*tags_ptr,3);
    tag[3]='\0';

    PCHR p = tags_ptr[1]; // start of the next field value
    if (p == nullptr) // If no end of field
      p = &RecBuffer[RecLength]; // use end of buffer

    //    size_t off = strlen (*tags_ptr) + 1; // offset from tag to field start
    size_t off = 4;
    INT val_start = (*tags_ptr + off) - RecBuffer;

    // Also leave off the \n
    INT val_len = strlen(*tags_ptr) - off;

    // Strip potential trailing white space
    while (val_len > 0 && isspace (RecBuffer[val_len + val_start]))
      val_len--;
    if (val_len <= 0) continue; // Don't bother with empty fields (J. Mandel)
    
    CHR* unified_name = UnifiedName(tag);

    // Ignore "unclassified" fields
    if (unified_name == nullptr)
      continue; // ignore these
    FieldName = unified_name;

    dfd.SetFieldName (FieldName);
    Db->DfdtAddEntry (dfd);
    fc.SetFieldStart (val_start);
    fc.SetFieldEnd (val_start + val_len);
#ifdef DEBUG
    cout << "Stored field " << FieldName << " data from offsets "
	 << val_start << " to " << val_start + val_len << endl;
#endif
    PFCT pfct = new FCT ();
    pfct->AddEntry (fc);
    df.SetFct (*pfct);
    df.SetFieldName (FieldName);
    pdft->AddEntry (df);
    delete pfct;

    marcdumpFieldNumToName(FieldName, &newFieldName);
    if (newFieldName.GetLength() > 0) {  // if we want the same field to have two names
      FieldName = newFieldName;
      dfd.SetFieldName(FieldName);
      Db->DfdtAddEntry(dfd);
      fc.SetFieldStart(val_start);
      fc.SetFieldEnd(val_start + val_len);
      pfct = new FCT();
      pfct->AddEntry(fc);
      df.SetFct(*pfct);
      df.SetFieldName(FieldName);
      pdft->AddEntry(df);
      delete pfct;
    } // end of if we have a second name
  }

  NewRecord->SetDft (*pdft);
  delete pdft;
  delete[]RecBuffer;
  delete [] tags;
}


CHR*
MARCDUMP::UnifiedName (CHR *tag)
{
  return tag; // Identity
}


void 
MARCDUMP::BeforeSearching(SQUERY* SearchQueryPtr)
{
  return;
}


void 
MARCDUMP::Present (const RESULT& ResultRecord, const STRING& ElementSet, 
		   STRING *StringBufferPtr)
{
  STRING RecSyntax;
  RecSyntax = SutrsRecordSyntax;
  Present(ResultRecord, ElementSet, RecSyntax, StringBufferPtr);
}


void 
MARCDUMP::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		     const STRING& RecordSyntax, STRING* StringBufferPtr) 
{
  STRING FieldName;

  if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
    PresentHtml(ResultRecord,ElementSet,StringBufferPtr);
  } else {
    PresentSutrs(ResultRecord,ElementSet,StringBufferPtr);
  }
  return;
}


void
MARCDUMP::PresentSutrs(const RESULT& ResultRecord, const STRING& ElementSet,
		   STRING *StringBufferPtr)
{
  STRING FieldName;

  if (ElementSet.CaseEquals("B")) {
    FieldName = "245"; // Brief headline is "title"
    Db->GetFieldData(ResultRecord, FieldName, StringBufferPtr);
    StringBufferPtr->EraseBefore(7);
  } else if (ElementSet.CaseEquals("F")) {
    DOCTYPE::Present (ResultRecord, ElementSet, StringBufferPtr);
  } else {
    FieldName = ElementSet;
    DOCTYPE::Present (ResultRecord, FieldName, StringBufferPtr);
  }
  return;
}


void
MARCDUMP::PresentHtml(const RESULT& ResultRecord, const STRING& ElementSet,
		   STRING *StringBufferPtr)
{
  STRING FieldName;

  if (ElementSet.CaseEquals("B")) {
    FieldName = "245"; // Brief headline is "title"
    Db->GetFieldData(ResultRecord, FieldName, StringBufferPtr);
    StringBufferPtr->EraseBefore(7);

  } else if (ElementSet.CaseEquals("F")) {
    STRING Buffer,Full,TheLine,URL;
    STRING Title,Contents,TheTag;
    STRLIST FullRecord, SubFields;
    INT nLines,i,intTag;
    STRING SubTag,SubField,ThisSubfield;

    // Get the whole record into a buffer
    ResultRecord.GetRecordData(&Full);

    // Convert the full record to a bunch of string
    FullRecord.Split('\n',Full);
    nLines = FullRecord.GetTotalEntries(); // count # of lines

    Buffer = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n";
    Buffer.Cat("<HTML>\n<HEAD>\n");

    FieldName = "245"; // Brief headline is "title"
    Db->GetFieldData(ResultRecord, FieldName, &Title);
    Title.EraseBefore(7);

    Buffer.Cat("<TITLE>");
    Buffer.Cat(Title);
    Buffer.Cat("</TITLE>\n</HEAD>\n");
    Buffer.Cat("<BODY BGCOLOR=\"#ffffff\" TEXT=\"#000000\">\n");

    for (i=1;i<nLines;i++) {
      FullRecord.GetEntry(i,&TheLine);
      intTag = SplitLine(TheLine,&TheTag,&Contents);
      if (intTag == 5)
	continue;
      else if (intTag == 7)
	continue;
      else if (intTag == 8)
	continue;
      else if (intTag == 256)
	continue;
      else if (intTag == 516)
	continue;
      else if (intTag == 538)
	continue;
      else if (intTag == 856) {
	// Are there subfields?
	SubTag = "$u";
	if (HasSubfieldTag(Contents,SubTag)) {
	  GetSubfield(Contents, SubTag, &SubField);
	  Contents = SubField;
	}

	GetSubfields(TheLine,&SubFields);
	SubFields.GetValue("$u",&ThisSubfield);

	URL = "<a href=\"";
	URL.Cat(Contents);
	URL.Cat("\">");
	URL.Cat(Contents);
	URL.Cat("</a>");
	Contents = URL;
      }

      marcdumpFieldNumToName(TheTag, &FieldName);
      if (FieldName.GetLength() > 0) {
	Buffer.Cat("<B>");
	Buffer.Cat(FieldName);
	Buffer.Cat(": </B>");
	Buffer.Cat(Contents);
	Buffer.Cat("<br>\n");
      } else {
	Buffer.Cat("<B>");
	Buffer.Cat(TheTag);
	Buffer.Cat(": </B>");
	Buffer.Cat(Contents);
	Buffer.Cat("<br>\n");
      }
    }

    Buffer.Cat("</BODY>\n</HTML>\n");
    *StringBufferPtr = Buffer;

  } else {
    FieldName = ElementSet;
    DOCTYPE::Present (ResultRecord, ElementSet, StringBufferPtr);
  }
  return;
}


MARCDUMP::~MARCDUMP()
{
}

/*-
   What: Given a buffer containing the output of the marcdump program,
   returns a list of char* to all characters pointing to the TAG

   marcdump Records:
001 ...
005 ...
007 ...
....

1) Each field starts on a new line
2) The first 3 digits define the field name
3) Someday we'll parse for subfields

-*/
static CHR **parse_tags (CHR *b, GPTYPE len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 48
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated

  // You should allocate these as you need them, but for now...
  max_num_tags = TAG_GROW_SIZE;
  t = new CHR* [max_num_tags];

  CHR* p;
  p = strtok(b,"\n");
  t[tc++] = p;
  while ((p=strtok(nullptr,"\n"))) {
    t[tc] = p;
    // Expand memory if needed
    if (++tc == max_num_tags - 1) {
      // allocate more space
      max_num_tags += TAG_GROW_SIZE;
      CHR **New = new CHR* [max_num_tags];
      if (New == nullptr) {
	delete [] t;
	return nullptr; // NO MORE CORE!
      }
      memcpy(New, t, tc*sizeof(CHR*));
      delete [] t;
      t = New;
    }
  }
  t[tc] = (CHR*)nullptr;
  return t;
}


int 
usefulMarcDumpField(char *fieldStr) 
{
  return 1;
}


// This function converts a numeric field name into an equivalent string, 
// except that it provides a nice way to search multiple fields at once.
// If you map more than one field number (say, 245 and 246) to the same
// text name (say, title), then you can search both fields by asking for
// a search in the text name, ie.
//
//     Isearch -d foo 245/bar
//
// will search field 245 for "bar", while
//
//     Isearch -d foo title/bar
//
// will search both fields 245 and 246 for "bar", assuming you've
// configured this routine to map both into the string "title".  This
// works for the Z39.50 field mapping, too, as long as the map file
// associates the attribute number with the text field name.
void
marcdumpFieldNumToName(STRING& fieldStr, STRING *fieldName) 
{
  int fieldNum;

  fieldNum = fieldStr.GetInt();
  *fieldName="";

  switch (fieldNum) {
  case 1: 
    *fieldName = "Identifier"; break;
    /*
      case 10: 
      *fieldName = "LCCN"; break;
      case 20: 
      *fieldName = "ISBN"; break;
      case 22: 
      *fieldName = "ISSN"; break;
      */
  case 40: 
    *fieldName = "Source"; break;
    /*
      case 82: 
      *fieldName = "Dewey"; break;
      case 100: 
      *fieldName = "Author"; break;
    */
  case 245: 
    *fieldName = "Title"; break;
  case 246: 
    *fieldName = "Title"; break;
    /*
      case 250: 
      *fieldName = "Edition/Version"; break;
      case 260: 
      *fieldName = "Publisher"; break;
      case 310: 
      *fieldName = "Frequency"; break;
      case 362: 
      *fieldName = "Beginning_Date"; break;
      */
  case 440: 
    *fieldName = "Series"; break;
  case 490: 
    *fieldName = "Series"; break;
    /*
      case 500: 
      *fieldName = "Note"; break;
      */
  case 520: 
    *fieldName = "Description"; break;
    /*
      case 524: 
      *fieldName = "Citation"; break;
      case 540: 
      *fieldName = "Terms"; break;
    */
  case 600: 
    *fieldName = "Subject"; break;
  case 610: 
    *fieldName = "Subject"; break;
  case 611: 
    *fieldName = "Subject"; break;
  case 630: 
    *fieldName = "Subject"; break;
  case 650: 
    *fieldName = "Subject"; break;
  case 651: 
    *fieldName = "Subject"; break;
  case 655: 
    *fieldName = "Genre"; break;
  case 700: 
    *fieldName = "Author"; break;
  case 710: 
    *fieldName = "Author"; break;
  case 711: 
    *fieldName = "Author"; break;
    /*
      case 740: 
      *fieldName = "Title"; break;
      case 830: 
      *fieldName = "Uniform_Title"; break;
    */
  case 856: 
    *fieldName = "Location"; break;
  case 997: 
    *fieldName = "Course"; break;
  default: 
    *fieldName =""; break;
  }
}


// Splits the line into the tag, and the contents
int
SplitLine(const STRING& TheLine, STRING* Tag, STRING* Buffer)
{
  STRING tmp = TheLine;
  INT TagNum;
  *Tag = tmp;
  Tag->EraseAfter(3);
  TagNum = Tag->GetInt();
  tmp.EraseBefore(5);
  *Buffer = tmp;
  return TagNum;
}


// Tests to see if the buffer contains any subtag ($) chars
GDT_BOOLEAN
HasSubfield(const STRING& TheContents, STRING* tag)
{
  STRINGINDEX here;
  STRING tmp;

  *tag="";
  if ((here=TheContents.Search('$'))) {
    // Found a subtag marker, check to see if it's genuine
    tmp = TheContents;
    tmp.EraseBefore(here);
    tmp.EraseAfter(3);

    // It should have the form "$X "
    here=tmp.Search(" ");
    if (here == 3) {
      tmp.EraseAfter(2);
      *tag = tmp;
      return GDT_TRUE;
    }
  }
  return GDT_FALSE;
}


// Tests to see if a particular subtag is present
GDT_BOOLEAN
HasSubfieldTag(const STRING& TheContents, STRING& tag)
{
  STRING tmp;
  CHR *pTag, *pField, *ptr;
  GDT_BOOLEAN status=GDT_FALSE;

  pTag = tag.NewCString();
  pField = TheContents.NewCString();

  if ((ptr=strstr(pField,pTag))) {
    status = GDT_TRUE;
  }
  delete [] pTag;
  delete [] pField;
  return status;
}


GDT_BOOLEAN
GetSubfield(const STRING& TheContents, const STRING& SubfieldTag,
	    STRING* Buffer)
{
  STRING tmp;
  CHR *pTag, *pField, *ptr;
  GDT_BOOLEAN status=GDT_FALSE;

  pTag = SubfieldTag.NewCString();
  pField = TheContents.NewCString();
  *Buffer="";

  if ((ptr=strstr(pField,pTag))) {
    *Buffer = ptr+3;
    status = GDT_TRUE;
  }
  delete [] pTag;
  delete [] pField;
  return status;
}


INT
GetSubfields(const STRING& TheContents, STRLIST* TheSubfields)
{
  STRLIST List;
  STRING Contents=TheContents;
  STRING tmp,Tag,Field;
  STRING NewContents;
  STRINGINDEX here;
  INT nSubs;

  List.Split('$',Contents);
  nSubs = List.GetTotalEntries();

  // If nSubs=1, there are no subfields
  if (nSubs > 1) {
    for (INT i=2;i<=nSubs;i++) {
      List.GetEntry(i,&tmp);
      // The first character is the subfield tag, the rest is the contents
      Tag = "$";
      Tag.Cat(tmp);
      here=Tag.Search(" ");
      Tag.EraseAfter(here);

      Field = tmp;
      here=Field.Search(" ");
      Field.EraseBefore(here+1);

      NewContents = Tag;
      NewContents.Cat("=");
      NewContents.Cat(Field);
      TheSubfields->AddEntry(NewContents);
    }
  }

  return (nSubs-1);
}
