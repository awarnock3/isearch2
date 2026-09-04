// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

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

// A WWW HTML DOCTYPE built on SGMLNORM's tag-based parsing.
// ParseFields() walks HTML <tag>value</tag> pairs (delegating the
// actual scanning to SGMLNORM::parse_tags()/find_end_tag()), skipping
// tags in IgnoreHTMLTag()'s list (or, if STRICT_HTML is defined,
// accepting only tags in IsHTMLFieldTag()'s allowlist instead) and
// applying a hand-rolled fallback for the common "minimized tag" HTML
// idioms (<DD>/<DT>/<LI>/<TL> used without a matching close). Present()
// with BRIEF_MAGIC ("B") returns the "title" field, falling back to
// the filename if there is none.
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

