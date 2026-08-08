// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		filmline.hxx
Version:	1.00
Description:	Class FILMLINE - FILMLINE v1.x Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef FILMLINE_HXX
#define FILMLINE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "medline.hxx"

// Filmline v 1.x Interchange format. Record splitting and field
// parsing are entirely inherited from MEDLINE; FILMLINE only
// customizes UnifiedName() (mapping Filmline's own two-letter field
// codes, e.g. "DI" for director, onto the shared Medline field-parser
// hooks) and Present() (a brief-headline composer for BRIEF_MAGIC).
class FILMLINE :  public MEDLINE {
public:
	FILMLINE(PIDBOBJ DbParent);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~FILMLINE();
// hooks into the guts of the Medline field parser
	const CHR *UnifiedName (const CHR *tag) const; 
};
typedef FILMLINE* PFILMLINE;


#endif
