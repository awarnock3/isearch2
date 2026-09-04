// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		irlist.hxx
Version:	1.00
Description:	Class IRLIST - IRList Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef IRLIST_HXX
#define IRLIST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

// A mail-digest DOCTYPE, splitting a digest file into one record per
// message at each "*********" magic separator line or blank-line-
// preceded mbox "From " line -- like the inherited
// MAILFOLDER::ParseRecords(), which this overrides to also recognize
// the magic separator, but otherwise mirrors its blank-line-guarded
// splitting logic. Field parsing (ParseFields()) is inherited
// unchanged from MAILFOLDER.
class IRLIST :  public MAILFOLDER {
public:
	IRLIST(PIDBOBJ DbParent);
	void ParseRecords(const RECORD& FileRecord);
	~IRLIST();
};

typedef IRLIST* PIRLIST;

#endif
