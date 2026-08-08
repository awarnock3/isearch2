// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		listdigest.hxx
Version:	1.00
Description:	Class LISTDIGEST - Listserver Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef LISTDIGEST_HXX
#define LISTDIGEST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

// A Listserv-style mail digest DOCTYPE, splitting a digest file into
// one record per message at each long "====...====" magic separator
// line (unlike its sibling doctype/irlist.cxx, this one does not also
// split on mbox "From " lines). Field parsing (ParseFields()) is
// inherited unchanged from MAILFOLDER.
class LISTDIGEST :  public MAILFOLDER {
public:
	LISTDIGEST(PIDBOBJ DbParent);
	void ParseRecords(const RECORD& FileRecord);
	~LISTDIGEST();
};

typedef LISTDIGEST* PLISTDIGEST;

#endif
