/*-@@@
File:		colondoc.hxx
Version:	1.00
Description:	Class COLONDOC - Colon Tagged (IAFA-like) Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef COLONDOC_HXX
#define COLONDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A "colon-tagged" (IAFA-like) DOCTYPE: records are lines of the form
// "Tag: value", where a field's value continues across following
// lines until the next "Tag:" line. See the "What"/format comment
// above the file-local parse_tags() in colondoc.cxx for the exact
// grammar (no whitespace between field name and ':', field names
// cannot contain whitespace).
class COLONDOC :  public DOCTYPE {
public:
	COLONDOC(PIDBOBJ DbParent);
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(PRECORD NewRecord);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~COLONDOC();
// hooks into the guts of the field parser
	virtual const CHR *UnifiedName (const CHR *tag) const; // for children to play with
};
typedef COLONDOC* PCOLONDOC;

#endif
