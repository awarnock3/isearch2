/*
 * Based on:
 */

/*-@@@
File:		html.hxx
Version:	1.03
Description:	Class LITMED - WWW LITMED Document Type
Author:		Edward C. Zimmermann, edz@bsn.com/Roy Smith, roy@nyu.edu
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef LITMED_HXX
#define LITMED_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmlnorm.hxx"

/*
 * 0==> Accept most tags
 * 1==> Accept only certain tags
 */
#define STRICT_LITMED	1

class LITMED:public SGMLNORM
{
  public:
  LITMED (PIDBOBJ DbParent);
  void ParseRecords (const RECORD & FileRecord);
  void ParseFields (PRECORD NewRecord);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		PSTRING StringBuffer);
   ~LITMED ();
};
typedef LITMED *PLITMED;

#endif
