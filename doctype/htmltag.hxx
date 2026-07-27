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
