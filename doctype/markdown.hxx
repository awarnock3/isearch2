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
