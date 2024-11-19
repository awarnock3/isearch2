/*@@@
File:		htmltag.cxx
Version:        1.0
Description:	Class HTMLTAG - HTML documents, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

#define HTMLTAG_MAX_TOKEN_LENGTH 4096

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include "isearch.hxx"
#include "htmltag.hxx"

HTMLTAG::HTMLTAG(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}


void 
HTMLTAG::ParseFields(PRECORD NewRecord) {

  // open the file
  STRING filename;
  NewRecord->GetFullFileName(&filename);
  CHR* fn = filename.NewCString();
  FILE* fp = fopen(fn, "rb");
  if (!fp) {
    cout << "HTMLTAG::ParseFields(): Failed to open file\n\t";
    perror(fn);
    delete [] fn;
    return;
  }

  int inHead = 0;  // 1 if we are in the <HEAD> ... </HEAD> section
  //  int done = 0;  // 1 if it is time to stop parsing
  GDT_BOOLEAN done = GDT_FALSE;  // 1 if it is time to stop parsing
  char token[HTMLTAG_MAX_TOKEN_LENGTH + 1];
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
	    token[tokenLength++] = ch;
	  }
	  break;
	}
	position++;
      }
    } while ( ( ! tokenReady ) && ( ch != EOF ) && (tokenLength < HTMLTAG_MAX_TOKEN_LENGTH) );

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
	    while ( isalnum(name[x]) && (name[x] != '\0') ) {
	      x++;
	    }
	    if (x>0) {
	      char *nameText;
	      nameText = new char[x+1];
	      strncpy(nameText, name, x);
	      nameText[x] = '\0';
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
HTMLTAG::Present(const RESULT& ResultRecord, const STRING& ElementSet,
               STRING* StringBufferPtr) {

  *StringBufferPtr = "";
  if (ElementSet.Equals("F")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;
  }
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
    *StringBufferPtr = Title;
}


HTMLTAG::~HTMLTAG() {
}

// returns 1 if tag is of type tagType.
// e.g. if tag[] == "<META NAME=\"AUTHOR\" CONTENT=\"Nassar\">"
//     and tagType[] == "META"
// then TagMatch will return 1
int HTMLTAG::TagMatch(char* tag, char* tagType) const {
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
	return ( ! isalnum(tag[y + 1]) );
}
