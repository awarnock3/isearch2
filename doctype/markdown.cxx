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

void
MARKDOWN::Present(const RESULT& ResultRecord, const STRING& ElementSet,
                  PSTRING StringBuffer) {
  if (ElementSet.Equals("B")) {
    STRING RecordText;
    ResultRecord.GetRecordData(&RecordText);
    ExtractMarkdownBrief(RecordText, StringBuffer);
    return;
  }

  DOCTYPE::Present(ResultRecord, ElementSet, StringBuffer);
}

MARKDOWN::~MARKDOWN() {
}
