/*@@@
File:		fct.hxx
Version:	1.00
Description:	Class FCT - Field Coordinate Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef FCT_HXX
#define FCT_HXX

#include <iostream>
#include "defs.hxx"  // BUGFIX #1: was commented out; INT/PFILE/GPTYPE below need it.
#include "fc.hxx"    // BUGFIX #1: was commented out; FC is used below.
#include "vlist.hxx" // BUGFIX #1: was commented out; base class.

// Field Coordinate Table: an ordered, circular list (via the VLIST base
// class) of FC field-coordinate pairs, one per field occurrence of a term
// within an indexed document.
class FCT : public VLIST {
public:
	FCT();
	FCT& operator=(const FCT& OtherFct);
	// Appends a copy of FcRecord as a new node.
	void AddEntry(const FC& FcRecord);
	// Copies the Index'th entry (1-based, matching VLIST::GetNodePtr) into
	// *FcRecord; leaves *FcRecord untouched if Index is out of range.
	void GetEntry(const INT Index, FC* FcRecord) const;
	// Sorts entries in place by FC::GetFieldStart().
	void SortByFc();
	// Serializes the table as text to fp.
	void Write(PFILE fp) const;
	// Reads the table back from text previously written by Write().
	void Read(PFILE fp);
	void Print(std::ostream& Os) const;
	// Shifts every entry's FieldStart/FieldEnd left by GpOffset.
	void SubtractOffset(const GPTYPE GpOffset);
	// BUGFIX #1: qualified std::ostream so this header doesn't depend on
	// some other translation unit's "using namespace std;" being in effect
	// first.
	friend std::ostream& operator<<(std::ostream& os, const FCT& Fct);
private:
	FC Fc;
};

typedef FCT* PFCT;

#endif
