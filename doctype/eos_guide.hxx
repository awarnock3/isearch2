/*@@@
File:		eos_guide.hxx
Version:	1.0
Description:	Class EOS_GUIDE - HTML documents with special markup, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

#ifndef EOS_GUIDE_HXX
#define EOS_GUIDE_HXX

#include "defs.hxx"
#include "doctype.hxx"

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
