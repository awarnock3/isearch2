// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		htmltag.hxx
Version:	1.0
Description:	Class HTMLTAG - HTML documents, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

#ifndef HTMLTAG_HXX
#define HTMLTAG_HXX

#include "defs.hxx"
#include "doctype.hxx"

// An HTML DOCTYPE that only ever looks inside <HEAD>...</HEAD>,
// indexing the <TITLE> text and every <META NAME="..." CONTENT="...">
// as its own field, via a character-at-a-time fgetc() tokenizer --
// structurally similar to doctype/eos_guide.cxx, though not derived
// from it.
class HTMLTAG
  : public DOCTYPE {

public:
  HTMLTAG(PIDBOBJ DbParent);
  void ParseFields(PRECORD NewRecord);
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	       STRING* StringBufferPtr);
  ~HTMLTAG();

private:
    int TagMatch(char* tag, const char* tagType) const;
};

typedef HTMLTAG* PHTMLTAG;

#endif
