/*-@@@
File:		medline.hxx
Version:	1.00
Description:	Class MEDLINE - MEDLINE-like Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef MEDLINE_HXX
#define MEDLINE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A MEDLINE-format DOCTYPE: tag-prefixed lines ("AB  - value", 2-4
// letter tag + mandatory ' '/'-' separator), value continues across
// following lines until the next tag. See the "What"/format comment
// above the file-local parse_tags() in medline.cxx for the exact
// grammar.
class MEDLINE :  public DOCTYPE {
public:
	MEDLINE(PIDBOBJ DbParent);
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(PRECORD NewRecord);
	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		PSTRING StringBuffer);
	~MEDLINE();
// hooks into the guts of the field parser
	virtual const CHR *UnifiedName (const CHR *tag) const; // for children to play with
};
typedef MEDLINE* PMEDLINE;

#endif
