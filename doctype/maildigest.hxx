// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		maildigest.hxx
Version:	1.00
Description:	Class MAILDIGEST - Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MAILDIGEST_HXX
#define MAILDIGEST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

// An Internet mail digest DOCTYPE, splitting a digest file into one
// record per message at each "----...----" (30 dashes) magic separator
// line (unlike its sibling doctype/irlist.cxx, this one does not also
// split on mbox "From " lines -- see doctype/listdigest.cxx, its other
// sibling, for the near-identical "====...====" variant). Field parsing
// (ParseFields()) is inherited unchanged from MAILFOLDER.
class MAILDIGEST :  public MAILFOLDER {
public:
	MAILDIGEST(PIDBOBJ DbParent);
	void ParseRecords(const RECORD& FileRecord);
	~MAILDIGEST();
};

typedef MAILDIGEST* PMAILDIGEST;

#endif
