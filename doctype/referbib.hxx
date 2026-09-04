// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		referbib.hxx
Version:	1.00
Description:	Class REFERBIB - Refer Bibliographic records Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef REFERBIB_HXX
#define REFERBIB_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A DOCTYPE for Unix `refer`-style bibliography records: one record
// per blank-line-separated entry, each made of "%X value" tag lines
// (see the file-local parse_tags()'s own header comment for the
// grammar). UnifiedName() maps single-letter refer tags (%A, %T, ...)
// to full field names via a fixed table.
class REFERBIB :  public DOCTYPE {
public:
	REFERBIB(PIDBOBJ DbParent);
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(PRECORD NewRecord);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~REFERBIB();
// hooks into the guts of the field parser
	virtual const CHR *UnifiedName (const CHR *tag) const; // for children to play with
};
typedef REFERBIB* PREFERBIB;

#endif
