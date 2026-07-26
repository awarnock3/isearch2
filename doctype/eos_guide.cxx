/*@@@
File:		eos_guide.cxx
Version:        1.0
Description:	Class EOS_GUIDE - HTML documents, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

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
  char ch;
  int foundTag = 0;  // 1 if we hit a tag and are building a token
  long position = 0;  // offset position within input file
  long tokenPosition;  // offset position of the beginning of the token
  int tokenReady;  // 1 if the token string is ready to be processed
  long titlePosition;  // offset position of start of title (after <TITLE>)
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
      ch = (char)fgetc(fp);
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
	    token[tokenLength++] = toupper(ch);
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
    titlePosition = 0;
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
    sprintf(ndb_string,"%d",ndb);

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
	StringBufferPtr->Cat(ndb);
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
      sprintf(ndb_string,"%d",ndb);

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
