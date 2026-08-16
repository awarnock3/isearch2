// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		markdown.hxx
Version:	1.00
Description:	Class MARKDOWN - Markdown document type
Author:		Copilot
@@@*/

#ifndef MARKDOWN_HXX
#define MARKDOWN_HXX

#include "defs.hxx"
#include "doctype.hxx"

// A DOCTYPE for Markdown documents. Record splitting and field parsing
// are entirely inherited from DOCTYPE's own defaults (no ParseRecords()/
// ParseFields() override); two element sets are customized via
// Present() in the source: "B" (brief) prefers the document's first
// ATX heading ("# Heading"), falling back to its first non-empty line
// (ExtractMarkdownBrief()); "S" returns every "#"-prefixed heading line
// in the record, newline-joined and otherwise unmodified
// (ExtractMarkdownHeaders()). Every other element set, including "F",
// returns the raw record text unchanged.
class MARKDOWN : public DOCTYPE {
public:
  MARKDOWN(PIDBOBJ DbParent);
  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		       PSTRING StringBuffer);
  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		       const STRING& RecordSyntax, PSTRING StringBuffer);
  virtual ~MARKDOWN();
};

typedef MARKDOWN* PMARKDOWN;

#endif
