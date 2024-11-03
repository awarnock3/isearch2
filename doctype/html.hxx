/*-@@@
File:		html.hxx
Version:	1.03
Description:	Class HTML - WWW HTML Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef HTML_HXX
#define HTML_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmlnorm.hxx"

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#ifndef STRICT_HTML
# define STRICT_HTML	0 /* 0==> Accept most tags 1==>Accept only certain tags */
#endif

class HTML:public SGMLNORM
{
public:
	HTML (PIDBOBJ DbParent);
	void ParseRecords (const RECORD & FileRecord);
	void ParseFields (PRECORD NewRecord);
	void GetMetadata(const RECORD& record, const STRING& mdType,
			   STRING* buffer);
	void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		      PSTRING StringBuffer);
	~HTML ();
};
typedef HTML *PHTML;

#endif

