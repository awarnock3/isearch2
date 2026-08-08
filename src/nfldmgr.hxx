#ifndef NUMERICFLDMGR_HXX
#define NUMERICFLDMGR_HXX
/*
#include "numericlist.hxx"
#include "irset.hxx"
*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// Loads and looks up per-attribute NUMERICLIST field tables for a
// database, from "<dbName>.fdf". See src/nfldmgr.cxx for the format
// and for a note on this class's current callers (there are none).
class NUMERICFLDMGR{
private:
	PNUMERICLIST fields;
	INT NumFields;
	INT MaxEntries;
public:
	NUMERICFLDMGR();
	~NUMERICFLDMGR();
	INT LoadFields(PCHR dbName);
	void GetResult(INT use, PIRSET s,PIDBOBJ p);
	INT LocateFieldByAttribute(INT Attribute);  // return index for field
				// list for this attribute - a search will
	INT Find(INT Attribute, INT4 Relation, FLOAT Key);

};
typedef NUMERICFLDMGR* PNUMERICFLDMGR;
	
#endif
