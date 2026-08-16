// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		iafadoc.hxx
Version:	1.00
Description:	Class IAFADOC - Colon Tagged (IAFA-like) Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef IAFADOC_HXX
#define IAFADOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "colondoc.hxx"

// A "colon:value"-tagged IAFA (Internet Anonymous FTP Archive) file
// announcement DOCTYPE. Record splitting and field parsing are
// entirely inherited from COLONDOC; IAFADOC only customizes Present(),
// which composes a one-line headline for BRIEF_MAGIC ("B") by trying
// several fallback tag names in turn (Title; else Package-/Service-/
// Preferred-/Mailinglist-/Newsgroup-Name; else a truncated
// Description), appending an Author when one is present.
class IAFADOC :  public COLONDOC {
public:
	IAFADOC(PIDBOBJ DbParent);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~IAFADOC();
};
typedef IAFADOC* PIAFADOC;

#endif
