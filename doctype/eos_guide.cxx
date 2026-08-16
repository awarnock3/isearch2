/*@@@
File:		eos_guide.cxx
Version:        1.0
Description:	Class EOS_GUIDE - HTML documents, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#define EOS_GUIDE_MAX_TOKEN_LENGTH 4096

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include "isearch.hxx"
#include "eos_guide.hxx"

EOS_GUIDE::EOS_GUIDE(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
	// Read doctype options
	STRLIST StrList;
	Db->GetDocTypeOptions(&StrList);
	StrList.GetValue("SOURCE", &DocSource);
}

// Reads NewRecord's bytes off disk one character at a time (fgetc()),
// tracking whether we're inside <HEAD>...</HEAD> and, within it,
// accumulating each HTML tag into `token`. Adds a "TITLE" field
// spanning the text between <TITLE> and </TITLE>, and one field per
// <META NAME="..." CONTENT="...">, named after its NAME attribute
// value and spanning its CONTENT attribute value. Stops as soon as
// </HEAD> or EOF is seen.
void
EOS_GUIDE::ParseFields(PRECORD NewRecord) {

  // open the file
  STRING filename;
  NewRecord->GetFullFileName(&filename);
  CHR* fn = filename.NewCString();
  FILE* fp = fopen(fn, "rb");
  if (!fp) {
    cout << "EOS_GUIDE::ParseFields(): Failed to open file\n\t";
    perror(fn);
    delete [] fn;
    return;
  }

  int inHead = 0;  // 1 if we are in the <HEAD> ... </HEAD> section
  //  int done = 0;  // 1 if it is time to stop parsing
  GDT_BOOLEAN done = GDT_FALSE;  // 1 if it is time to stop parsing
  char token[EOS_GUIDE_MAX_TOKEN_LENGTH + 1];
  int tokenLength;  // maintained while still building the string
  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeeos_guidecxx): ch must stay
  // an int. fgetc() returns either a real byte value (0-255) or the
  // sentinel EOF (typically -1); the original `char ch;` immediately
  // narrowed that result, and on a platform where char is signed
  // (the common case), the real byte 0xFF narrows to -1 too -- the
  // exact same value as EOF. Every file containing that byte anywhere
  // in its <HEAD> section (a real possibility for Latin-1/ISO-8859-1
  // content, which this class's own XML header elsewhere claims to
  // support) had parsing silently stop early right there.
  int ch;
  int foundTag = 0;  // 1 if we hit a tag and are building a token
  long position = 0;  // offset position within input file
  long tokenPosition;  // offset position of the beginning of the token
  int tokenReady;  // 1 if the token string is ready to be processed
  // BUGFIX #2: was reset to 0 unconditionally at the top of every
  // "process token" pass below (see the removed `titlePosition = 0;`
  // that used to sit right after this loop begins) -- since <TITLE>
  // and its value are read across several loop iterations (each
  // iteration captures one tag; the title text between them is
  // skipped character-by-character, not accumulated), that reset
  // destroyed the position <TITLE> had just recorded before the
  // matching </TITLE> iteration ever got a chance to read it. The
  // `if (titlePosition == 0) break;` guard a few lines below was
  // therefore *always* true -- and since that `break` isn't inside a
  // switch, it exits the *entire* main parsing loop, not just the
  // <TITLE> handling. So the bug was worse than "title never
  // extracted": parsing stopped dead the moment any </TITLE> was
  // seen, silently discarding every <META> (or anything else) that
  // came after it too. Confirmed via a real before/after test run:
  // reverting this fix made 3 of this file's 4 regression tests fail,
  // including the META-extraction one, not just the TITLE one. One-
  // time initialization here instead.
  long titlePosition = 0;  // offset position of start of title (after <TITLE>)
  DFD dfd;
  DFT dft;
  FC fc;
  DF df;

  // main parsing loop

  while ( ! done ) {

    // get next token (i.e. the next HTML tag)
    tokenLength = 0;
    foundTag = 0;
    tokenReady = 0;
    do {
      ch = fgetc(fp);
      if (ch == EOF) {
	token[tokenLength] = '\0';
      } else {
	switch (ch) {
	case '<':
	  if ( ! foundTag ) {
	    foundTag = 1;
	    tokenPosition = position;
	  }
	  token[tokenLength++] = '<';
	  break;
	case '>':
	  if ( foundTag ) {
	    token[tokenLength++] = '>';
	    token[tokenLength] = '\0';
	    tokenReady = 1;
	  }
	  break;
	default:
	  if ( foundTag ) {
	    // BUGFIX #1 (continued): toupper() is undefined behavior
	    // for a value not representable as unsigned char (or EOF);
	    // ch is guaranteed in [0,255] here (the EOF case already
	    // branched away above), but cast explicitly since ch is a
	    // plain int, not implicitly unsigned-char-range like a
	    // char would (mis)appear to be.
	    token[tokenLength++] = (char)toupper((unsigned char)ch);
	  }
	  break;
	}
	position++;
      }
    } while ( ( ! tokenReady ) && ( ch != EOF ) && (tokenLength < EOS_GUIDE_MAX_TOKEN_LENGTH) );

    if (ch == EOF) {
      done = GDT_TRUE;
      break;
    }
    
    // process token
    if (inHead) {
      // we are in the <HEAD> section, so we do want to process this
      if (TagMatch(token, "/HEAD")) {
	done = GDT_TRUE;
	break;
      }

      if (TagMatch(token, "TITLE")) {
	titlePosition = tokenPosition + 7;
      }

      if (TagMatch(token, "/TITLE")) {
	// We know that titlePosition cannot be 0, since we have to move
	// into the file past the <HEAD> tag, so if it is still 0, we never
	// found a title tag.
	if (titlePosition == 0)
	  break;

	if ( (tokenPosition - 1 - titlePosition) > 0 ) {
	  STRING fieldName;
	  fieldName = "TITLE";
	  dfd.SetFieldName(fieldName);
	  Db->DfdtAddEntry(dfd);
	  fc.SetFieldStart(titlePosition);
	  fc.SetFieldEnd(tokenPosition - 1);
	  FCT fct;
	  fct.AddEntry(fc);
	  df.SetFct(fct);
	  df.SetFieldName(fieldName);
	  dft.AddEntry(df);
	}
      }
      if (TagMatch(token, "META")) {
	char* name = strstr(token + 6, "NAME=\"");
	char* content = strstr(token + 6, "CONTENT=\"");
	if (name && content) {
	  char* contentEndQuote = strchr(content + 9, '\"');
	  name = name + 6;
	  if (contentEndQuote) {
	    // extract NAME value
	    int x = 0;
	    /* while ( (name[x] != '\"') && (name[x] != '\0') ) { */
	    //	    while ( isalnum(name[x]) && (name[x] != '\0') ) {
	    while ( IsAlnum(name[x]) && (name[x] != '\0') ) {
	      x++;
	    }
	    if (x>0) {
	      char *nameText;
	      nameText = new char[x+1];
	      strncpy(nameText, name, x);
	      nameText[x] = '\0';
#ifdef DEBUG
	      fprintf(stderr,"Name=%s,Content=%s\n",nameText,content);
#endif
	      STRING fieldName;
	      fieldName = nameText;
	      // now build the position data
	      long contentStart = tokenPosition + (content - token) + 9;
	      long contentEnd = tokenPosition + (contentEndQuote - token) - 1;
	      dfd.SetFieldName(fieldName);
	      Db->DfdtAddEntry(dfd);
	      fc.SetFieldStart(contentStart);
	      fc.SetFieldEnd(contentEnd);
	      FCT fct;
	      fct.AddEntry(fc);
	      df.SetFct(fct);
	      df.SetFieldName(fieldName);
	      dft.AddEntry(df);
	      delete [] nameText;
	    }
	  }
	}
      }
    } else {
      if (TagMatch(token, "HEAD")) {
	inHead = 1;
      }
    }
  }

  NewRecord->SetDft(dft);
  
  fclose(fp);
  delete [] fn;
  
}


void 
EOS_GUIDE::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		   const STRING& RecordSyntax, STRING* StringBufferPtr) {

  STRING Title,Path,File,Hold,Key;
  STRINGINDEX n;
  INT ndb;
  CHR ndb_string[8];

  *StringBufferPtr = "";
  if (ElementSet.Equals("F")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;

  } else if (ElementSet.Equals("R")) {
    STRLIST Strlist;
    GDT_BOOLEAN Status;
    STRING FieldName = "TITLE";

    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "No title";

    ResultRecord.GetPathName(&Path);
    ResultRecord.GetFileName(&File);
    ResultRecord.GetKey(&Key);
    ndb = ResultRecord.GetDbNum();
    snprintf(ndb_string,sizeof(ndb_string),"%d",ndb);

    n = Path.Search("http/");
    if (n > 0) {
      n = Path.Search("/");
      Path.EraseBefore(n);
      //      Path.Replace("http/","http://");
      // Now look for the next /
      n = Path.Search("/");
      STRING HoldPath;
      HoldPath = Path;
      Path.EraseBefore(n);
      HoldPath.EraseAfter(n);
    }

    if (RecordSyntax.CaseEquals("XML")) {
      Path.Cat(File);
      *StringBufferPtr = "\t\t\t<isearch:result docid=\"";
      if (ndb > 0) {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypeeos_guidecxx): this
	// passed the raw INT ndb straight to Cat(), which has no
	// INT overload -- it silently resolved to Cat(const UCHR
	// Character) instead (an implicit INT->UCHR narrowing
	// conversion), appending one garbled byte in place of the
	// intended decimal docid number. The "B"/ISEARCH_XML branch
	// below does this correctly, via the ndb_string buffer built
	// above.
	StringBufferPtr->Cat(ndb_string);
	StringBufferPtr->Cat(':');
      }
      //      Key.XmlCleanup();
      //      DocSource.XmlCleanup();
      Title.XmlCleanup();
      //      Path.XmlCleanup();

      StringBufferPtr->Cat(Key);
      StringBufferPtr->Cat("\" status=\"OK\" source=\"");
      StringBufferPtr->Cat(DocSource);
      StringBufferPtr->Cat("\">\n");
      StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"title\">");
      StringBufferPtr->Cat(Title);
      StringBufferPtr->Cat("</isearch:field>\n");
      StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"documenturl\">\n");
      StringBufferPtr->Cat("\t\t\t\t\t<xlink type=\"link\" ref=\"");
      StringBufferPtr->Cat(Path);
      StringBufferPtr->Cat("\" />\n");
      StringBufferPtr->Cat("\t\t\t\t</isearch:field>\n");
      StringBufferPtr->Cat("\t\t\t</isearch:result>\n");

    } else {

      Path.Cat(File);

      *StringBufferPtr = "TITLE=";
      StringBufferPtr->Cat(Title);
      StringBufferPtr->Cat("\n");
      StringBufferPtr->Cat("LINK=");
      StringBufferPtr->Cat(Path);
      StringBufferPtr->Cat("\nSOURCE=");
      StringBufferPtr->Cat(DocSource);
    }

  } else if (ElementSet.Equals("B")) {
    STRLIST Strlist;
    STRING Title;
    GDT_BOOLEAN Status;
    STRING FieldName = "TITLE";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "(title not found)";
    //    *StringBufferPtr = Title;

    Title.TrimLeading();

    if (RecordSyntax.CaseEquals("ISEARCH_XML")) {
      STRING HoldPath;
      STRINGINDEX colon,dot;

      ResultRecord.GetPathName(&Path);
      ResultRecord.GetFileName(&File);
      ResultRecord.GetKey(&Key);
      ndb = ResultRecord.GetDbNum();
      snprintf(ndb_string,sizeof(ndb_string),"%d",ndb);

      n = Path.Search("http/");
      if (n > 0) {
	Path.EraseBefore(n);
	//	Path.Replace("http/","http://");
	// Now look for the next / 
	n = Path.Search("/"); // This is on http/ so erase it, too
	Path.EraseBefore(n+1);
	HoldPath = Path;
	n = Path.Search("/");
	Path.EraseAfter(n-1);
	HoldPath.EraseBefore(n);
	colon = Path.SearchReverse('_');
	dot = Path.SearchReverse('.');
	if (dot < colon) {
	  Path.SetChr(colon,':');
	}
	Path.Cat(HoldPath);
	HoldPath = Path;
	Path = "http://";
	Path.Cat(HoldPath);
      }

      Path.Cat(File);
      Title.XmlCleanup();

      *StringBufferPtr = "\t\t\t<isearch:result docid=\"";
      if (ndb > 0) {
	StringBufferPtr->Cat(ndb_string);
	StringBufferPtr->Cat(':');
      }
      //      Key.XmlCleanup();
      //      DocSource.XmlCleanup();
      //      Path.XmlCleanup();

      StringBufferPtr->Cat(Key);
      StringBufferPtr->Cat("\" status=\"OK\" source=\"");
      StringBufferPtr->Cat(DocSource);
      StringBufferPtr->Cat("\">\n");
      StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"title\">");
      StringBufferPtr->Cat(Title);
      StringBufferPtr->Cat("</isearch:field>\n");
      StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"documenturl\">\n");
      StringBufferPtr->Cat("\t\t\t\t\t<xlink type=\"link\" ref=\"");
      StringBufferPtr->Cat(Path);
      StringBufferPtr->Cat("\" />\n");
      StringBufferPtr->Cat("\t\t\t\t</isearch:field>\n");
      StringBufferPtr->Cat("\t\t\t</isearch:result>\n");

    } else if (RecordSyntax.CaseEquals("XML")) {
      STRING XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n";
      // Clean the title buffer to make sure it is legal XML
      //      Title.XmlCleanup();

      *StringBufferPtr = XML_Header;
      StringBufferPtr->Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
      StringBufferPtr->Cat("<title>");
      StringBufferPtr->Cat(Title);
      StringBufferPtr->Cat("</title>\n");
      StringBufferPtr->Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");

    } else {
      *StringBufferPtr = "";
      StringBufferPtr->Cat(DocSource);
      StringBufferPtr->Cat(": ");
      StringBufferPtr->Cat(Title);
    }
  } else if (ElementSet.Equals("S")) {
    STRLIST Strlist;
    STRING Title;
    GDT_BOOLEAN Status;
    STRING FieldName = "TITLE";
    Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "(title not found)";
    //    *StringBufferPtr = Title;

    Title.TrimLeading();
    if (RecordSyntax.CaseEquals("XML")) {
      STRING XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n";
      // Clean the title buffer to make sure it is legal XML
      //      Title.XmlCleanup();

      *StringBufferPtr = XML_Header;
      StringBufferPtr->Cat("<metadata>\n<idinfo>\n<citation>\n<citeinfo>\n");
      StringBufferPtr->Cat("<title>");
      StringBufferPtr->Cat(Title);
      StringBufferPtr->Cat("</title>\n");
      StringBufferPtr->Cat("</citeinfo>\n</citation>\n</idinfo>\n</metadata>\n");
    }

  } else {
    DOCTYPE::Present (ResultRecord, ElementSet, StringBufferPtr);
  }
}


EOS_GUIDE::~EOS_GUIDE() {
}


// returns 1 if tag is of type tagType.
// e.g. if tag[] == "<META NAME=\"AUTHOR\" CONTENT=\"Nassar\">"
//     and tagType[] == "META"
// then TagMatch will return 1
INT 
EOS_GUIDE::TagMatch(char* tag, const char* tagType) const {
  // check first character
  if (*tag != '<') {
    return 0;
  }
  // iterate tagType[] and compare (case-insensitive) with tag
  int x;
  int y = strlen(tagType);
  for (x = 0; x < y; x++) {
    if (toupper(tag[x + 1]) != toupper(tagType[x])) {
      return 0;
    }
  }
  // now just make sure that was really the end of the tag
  //  return ( ! isalnum(tag[y + 1]) );
  return ( ! IsAlnum(tag[y + 1]) );
}
