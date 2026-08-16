// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		markdown.cxx
Version:	1.00
Description:	Class MARKDOWN - Markdown document type
Author:		Copilot
@@@*/

#include <ctype.h>

#include "isearch.hxx"
#include "markdown.hxx"

MARKDOWN::MARKDOWN(PIDBOBJ DbParent) : DOCTYPE(DbParent) {
}

// Fills BriefBuffer with RecordText's first ATX heading ("#"/"##"/...
// prefixed line, stripped of the leading '#' run and surrounding
// whitespace), or, if no heading is found anywhere in the text, its
// first non-empty line. Leaves BriefBuffer empty for a blank record.
static void
ExtractMarkdownBrief(const STRING& RecordText, PSTRING BriefBuffer) {
  *BriefBuffer = "";

  CHR* Text = RecordText.NewCString();
  const CHR* p = Text;
  STRING FirstNonEmptyLine;

  while (*p) {
    const CHR* LineStart = p;
    while (*p && *p != '\n' && *p != '\r') {
      p++;
    }
    const CHR* LineEnd = p;

    while (*p == '\r' || *p == '\n') {
      p++;
    }

    const CHR* s = LineStart;
    while (s < LineEnd && isspace((unsigned char)*s)) {
      s++;
    }
    const CHR* e = LineEnd;
    while (e > s && isspace((unsigned char)*(e - 1))) {
      e--;
    }

    if (e > s && FirstNonEmptyLine.GetLength() == 0) {
      FirstNonEmptyLine.Set((const UCHR*)s, (STRINGINDEX)(e - s));
    }

    if (s < e && *s == '#') {
      const CHR* h = s;
      while (h < e && *h == '#') {
        h++;
      }
      while (h < e && isspace((unsigned char)*h)) {
        h++;
      }
      const CHR* t = e;
      while (t > h && *(t - 1) == '#') {
        t--;
      }
      while (t > h && isspace((unsigned char)*(t - 1))) {
        t--;
      }
      if (t > h) {
        BriefBuffer->Set((const UCHR*)h, (STRINGINDEX)(t - h));
        delete [] Text;
        return;
      }
    }
  }

  if (FirstNonEmptyLine.GetLength() > 0) {
    *BriefBuffer = FirstNonEmptyLine;
  }

  delete [] Text;
}

// Element set "B" returns ExtractMarkdownBrief()'s heading/first-line
// summary; "S" returns ExtractMarkdownHeaders()'s "#"-prefixed heading
// lines; every other element set (including "F") returns the raw
// record text unchanged.
static void
ExtractMarkdownHeaders(const STRING& RecordText, PSTRING HeaderBuffer) {
  *HeaderBuffer = "";

  CHR* Text = RecordText.NewCString();
  const CHR* p = Text;
  bool first_header = true;

  while (*p) {
    const CHR* LineStart = p;
    while (*p && *p != '\n' && *p != '\r') {
      p++;
    }
    const CHR* LineEnd = p;

    while (*p == '\r' || *p == '\n') {
      p++;
    }

    const CHR* s = LineStart;
    while (s < LineEnd && isspace((unsigned char)*s)) {
      s++;
    }
    const CHR* e = LineEnd;
    while (e > s && isspace((unsigned char)*(e - 1))) {
      e--;
    }

    if (s < e && *s == '#') {
      if (!first_header) {
        HeaderBuffer->Cat("\n");
      }
      HeaderBuffer->Cat((const CHR*)s, (STRINGINDEX)(e - s));
      first_header = false;
    }
  }

  delete [] Text;
}

void
MARKDOWN::Present(const RESULT& ResultRecord, const STRING& ElementSet,
                  PSTRING StringBuffer) {
  Present(ResultRecord, ElementSet, SutrsRecordSyntax, StringBuffer);
}

void
MARKDOWN::Present(const RESULT& ResultRecord, const STRING& ElementSet,
                  const STRING& RecordSyntax, PSTRING StringBuffer) {
  (void)RecordSyntax;

  STRING RecordText;
  ResultRecord.GetRecordData(&RecordText);

  if (ElementSet.Equals("F")) {
    *StringBuffer = RecordText;
    return;
  }

  if (ElementSet.Equals("S")) {
    ExtractMarkdownHeaders(RecordText, StringBuffer);
    return;
  }

  if (ElementSet.Equals("B")) {
    ExtractMarkdownBrief(RecordText, StringBuffer);
    return;
  }

  *StringBuffer = RecordText;
}

MARKDOWN::~MARKDOWN() {
}
