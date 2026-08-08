// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		memodoc.hxx
Version:	1.00
Description:	Class MEMODOC - Colon Tagged Memo Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MEMODOC_HXX
#define MEMODOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A DOCTYPE for "TAG: value" memo documents (see the file-local
// parse_tags() in the source for the exact grammar, including the
// "____"/"----"/"++++"/"====" 4-char run that marks the end of the
// tagged header and the start of the free-text "Memo-Body"). Record
// splitting is inherited unchanged from DOCTYPE; only ParseFields()
// and Present() are overridden.
class MEMODOC :  public DOCTYPE {
public:
	MEMODOC(PIDBOBJ DbParent);
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(PRECORD NewRecord);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~MEMODOC();
};
typedef MEMODOC* PMEMODOC;

#endif
