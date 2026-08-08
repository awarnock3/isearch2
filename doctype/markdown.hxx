// ISEARCH2-CLEANUP: processed 2026-08-08
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
// ParseFields() override); only the "B" (brief) element set is
// customized, via Present() -> ExtractMarkdownBrief() in the source,
// to prefer the document's first ATX heading ("# Heading") and fall
// back to its first non-empty line.
class MARKDOWN : public DOCTYPE {
public:
  MARKDOWN(PIDBOBJ DbParent);
  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		       PSTRING StringBuffer);
  virtual ~MARKDOWN();
};

typedef MARKDOWN* PMARKDOWN;

#endif
