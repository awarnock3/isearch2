// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

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


// Scans the file a character at a time up through </HEAD>, indexing
// the <TITLE> text as a "TITLE" field and every <META NAME="..."
// CONTENT="..."> as a field named after its NAME value.
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
  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypehtmltagcxx): ch must stay an
  // int. fgetc() returns either a real byte value (0-255) or the
  // sentinel EOF (typically -1); the original `char ch;` immediately
  // narrowed that result, and on a platform where char is signed (the
  // common case), the real byte 0xFF narrows to -1 too -- the exact
  // same value as EOF. Every file containing that byte anywhere in its
  // <HEAD> section had parsing silently stop early right there. Same
  // bug as, and fixed the same way as, doctype/eos_guide.cxx's
  // BUGFIX #1 (a structurally similar <HEAD>-only HTML tokenizer).
  int ch;
  int foundTag = 0;  // 1 if we hit a tag and are building a token
  long position = 0;  // offset position within input file
  long tokenPosition;  // offset position of the beginning of the token
  int tokenReady;  // 1 if the token string is ready to be processed
  long titlePosition;  // offset position of start of title (after <TITLE>)
  int sawTitleOpen = 0;  // 1 once a <TITLE> has actually been seen
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
	sawTitleOpen = 1;
      }
      // BUGFIX #3 (docs/BUG_CATALOG.md#doctypehtmltagcxx): titlePosition
      // was read here even if a matching <TITLE> had never actually
      // been seen (e.g. a stray/malformed </TITLE> with no opener),
      // reading the uninitialized local -- guarded with sawTitleOpen
      // instead of trusting whatever garbage happened to be on the
      // stack.
      if (sawTitleOpen && TagMatch(token, "/TITLE")) {
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
	    // BUGFIX #2 (docs/BUG_CATALOG.md#doctypehtmltagcxx): isalnum()
	    // is undefined behavior for an argument not representable as
	    // unsigned char (or EOF) -- name[x] is a plain (possibly
	    // signed) char read from file content, so a high-bit-set byte
	    // (e.g. Latin-1/UTF-8 in a META NAME value) would pass a
	    // negative value. Same UB class as src/nlatlon.cxx's earlier
	    // fix; cast explicitly.
	    while ( isalnum((unsigned char)name[x]) && (name[x] != '\0') ) {
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


// ElementSet "F" returns the raw record data; anything else returns
// the indexed "TITLE" field, falling back to a placeholder if none
// was found.
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
int HTMLTAG::TagMatch(char* tag, const char* tagType) const {
	// check first character
	if (*tag != '<') {
		return 0;
	}
	// iterate tagType[] and compare (case-insensitive) with tag
	//
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypehtmltagcxx): toupper()/
	// isalnum() are undefined behavior for an argument not
	// representable as unsigned char (or EOF); tag[] holds raw file
	// content, so tag[x + 1] and tag[y + 1] could be negative on a
	// signed-char platform. tagType[] is always one of this file's own
	// ASCII string-literal calls ("HEAD", "TITLE", etc.), so casting
	// it is defensive rather than fixing an observed bug, but keeps
	// both sides of the comparison consistently safe.
	int x;
	int y = strlen(tagType);
	for (x = 0; x < y; x++) {
		if (toupper((unsigned char)tag[x + 1]) != toupper((unsigned char)tagType[x])) {
			return 0;
		}
	}
	// now just make sure that was really the end of the tag
	return ( ! isalnum((unsigned char)tag[y + 1]) );
}
