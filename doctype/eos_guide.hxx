/*@@@
File:		eos_guide.hxx
Version:	1.0
Description:	Class EOS_GUIDE - HTML documents with special markup, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef EOS_GUIDE_HXX
#define EOS_GUIDE_HXX

#include "defs.hxx"
#include "doctype.hxx"

// An HTML DOCTYPE that only ever looks inside <HEAD>...</HEAD>: reads
// the <TITLE> text as a "TITLE" field, and every <META NAME="..."
// CONTENT="..."> as a field named after its NAME attribute. See
// ParseFields() in eos_guide.cxx for the character-at-a-time tokenizer
// this is built on.
class EOS_GUIDE
  : public DOCTYPE {

public:
  EOS_GUIDE(PIDBOBJ DbParent);
  void ParseFields(PRECORD NewRecord);
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING* StringBufferPtr);
  ~EOS_GUIDE();

private:
  INT TagMatch(char* tag, const char* tagType) const;
  STRING DocSource;
};

typedef EOS_GUIDE* PEOS_GUIDE;

#endif
