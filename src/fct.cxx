/*@@@
File:		fct.cxx
Version:	1.00
Description:	Class FCT - Field Coordinate Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/**
 * @file fct.cxx
 * @brief Implementation of the Field Coordinate Table (FCT) class.
 *
 * Provides the implementation for FCT, an ordered circular list that manages field
 * coordinate pairs. The FCT class inherits from VLIST to maintain a doubly-linked
 * list structure. Supports operations such as adding entries, retrieving coordinates,
 * sorting by field start position, and serialization to/from text format. Field
 * coordinates track the byte positions of indexed term occurrences within document
 * fields, enabling precise field-based searching and result highlighting.
 */

#include <stdlib.h>
#include <iostream>

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "fc.hxx"
#include "fct.hxx"

//#include <iostream.h>

/**
 * @brief Constructs an empty FCT node; initializes the VLIST base.
 */
FCT::FCT() : VLIST() {
}


/**
 * @brief Assigns the contents of another FCT to this instance.
 *
 * Clears the current table and creates a deep copy of all entries from the source FCT.
 * Each entry is copied as a new node in the list.
 *
 * @param OtherFct The source FCT to copy from.
 * @return A reference to this FCT instance.
 */
FCT& FCT::operator=(const FCT& OtherFct) {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcfctcxx): no self-assignment
	// guard -- Clear() ran before OtherFct.GetTotalEntries() was read,
	// so `fct = fct;` cleared itself and then "copied" its own
	// now-empty contents back, silently losing every entry. Same shape
	// as DF's/ATTRLIST's/DFDT's/DFT's own fixes for this pattern
	// elsewhere in this tree; already flagged here as deferred, not
	// fixed, at DF's own turn (docs/BUG_CATALOG.md#srcdfcxx).
	if (this == &OtherFct) {
		return *this;
	}
	Clear();
	SIZE_T x;
	SIZE_T y = OtherFct.GetTotalEntries();
	FCT* NodePtr;
	FCT* NewNodePtr;
	for (x=1; x<=y; x++) {
		NodePtr = (FCT*)(OtherFct.GetNodePtr(x));
		NewNodePtr = new FCT();
		NewNodePtr->Fc = NodePtr->Fc;
		VLIST::AddNode(NewNodePtr);
	}
	return *this;
}

/**
 * @brief Appends a new entry to the end of the field coordinate table.
 *
 * Creates a new FCT node, copies the provided FC record into it, and adds it
 * to the circular list.
 *
 * @param FcRecord The field coordinate record to add.
 */
void FCT::AddEntry(const FC& FcRecord) {
	FCT* NodePtr = new FCT();
	NodePtr->Fc = FcRecord;
	VLIST::AddNode(NodePtr);
}

/**
 * @brief Retrieves a field coordinate entry at the specified 1-based index.
 *
 * Looks up the entry at the given index (1-based indexing per VLIST convention)
 * and copies it into the provided FC pointer. If the index is out of range,
 * the output pointer is left unchanged.
 *
 * @param Index The 1-based index of the entry to retrieve.
 * @param FcRecord Pointer to an FC object to receive the copied entry.
 */
void FCT::GetEntry(const INT Index, FC* FcRecord) const {
	FCT* NodePtr = (FCT*)(VLIST::GetNodePtr(Index));
	if (NodePtr) {
		*FcRecord = NodePtr->Fc;
	}
}

/**
 * @brief Comparison function for qsort; compares two FC objects by field start position.
 *
 * @param x Pointer to first FC object.
 * @param y Pointer to second FC object.
 * @return Difference of field start positions (for qsort ordering).
 */
int FctFcCompare(const void* x, const void* y) {
	// BUGFIX #2 (docs/BUG_CATALOG.md#srcfctcxx): subtracting two GPTYPE
	// (unsigned) values and narrowing the result to the int qsort
	// expects silently misorders once two field offsets in the same
	// table differ by more than ~2GB -- confirmed with a standalone
	// repro (0u - 3000000000u narrowed to int came out positive, the
	// wrong sign for 0 < 3000000000). Comparing directly instead of
	// subtracting avoids the overflow entirely, regardless of offset
	// magnitude.
	GPTYPE xStart = ((FC*)x)->GetFieldStart();
	GPTYPE yStart = ((FC*)y)->GetFieldStart();
	if (xStart < yStart) return -1;
	if (xStart > yStart) return 1;
	return 0;
}

/**
 * @brief Sorts all entries in the field coordinate table by field start position.
 *
 * Extracts all field coordinates into a temporary array, sorts them using qsort,
 * and then re-inserts them into the circular list in sorted order.
 */
void FCT::SortByFc() {
	SIZE_T TotalEntries = GetTotalEntries();
	FC* TablePtr = new FC[TotalEntries];
	SIZE_T x = 0;
	FCT* p = (FCT*)(this->GetNextNodePtr());
	while (p != this) {
		TablePtr[x++] = p->Fc;
		p = (FCT*)(p->GetNextNodePtr());
	}
	qsort(TablePtr, TotalEntries, sizeof(FC), FctFcCompare);
	p = (FCT*)(p->GetNextNodePtr());
	x = 0;
	while (p != this) {
		p->Fc = TablePtr[x++];
		p = (FCT*)(p->GetNextNodePtr());
	}
	delete [] TablePtr;
}

/**
 * @brief Serializes the field coordinate table to text format.
 *
 * Writes the total entry count followed by each FC entry in text form
 * (delegating to FC::Write() for each entry's format).
 *
 * @param fp File pointer to write to.
 */
void FCT::Write(PFILE fp) const {
	SIZE_T TotalEntries = GetTotalEntries();
	fprintf(fp, "%zu\n", TotalEntries);
	SIZE_T x;
	for (x=1; x<=TotalEntries; x++) {
		((FCT*)(VLIST::GetNodePtr(x)))->Fc.Write(fp);
	}
}

/**
 * @brief Deserializes the field coordinate table from text format.
 *
 * Clears the current table and reads in a previously serialized set of entries.
 * Expects a format matching that written by Write(): entry count on first line,
 * followed by each entry's text representation.
 *
 * @param fp File pointer to read from.
 */
void FCT::Read(PFILE fp) {
	Clear();
	STRING s;
	FC Fc;
	s.FGet(fp, 16);
	INT n, x;
	n = s.GetInt();
	for (x=0; x<n; x++) {
		Fc.Read(fp);
		AddEntry(Fc);
	}
}

/**
 * @brief Outputs all entries to an output stream.
 *
 * Walks the circular list and outputs each FC entry using its stream insertion operator.
 *
 * @param Os The output stream to write to.
 */
void FCT::Print(ostream& Os) const {
	FCT* p = (FCT*)(this->GetNextNodePtr());
	while (p != this) {
		Os << p->Fc;
		p = (FCT*)(p->GetNextNodePtr());
	}
}

/**
 * @brief Shifts all field coordinates left by a given offset.
 *
 * Subtracts the specified offset from both FieldStart and FieldEnd of every
 * entry in the table. Used during document processing when field boundaries
 * need to be adjusted relative to a new base position.
 *
 * @param GpOffset The offset value to subtract from all field coordinates.
 */
void FCT::SubtractOffset(const GPTYPE GpOffset) {
	FCT* p = (FCT*)(this->GetNextNodePtr());
	while (p != this) {
		p->Fc.SetFieldStart(p->Fc.GetFieldStart() - GpOffset);
		p->Fc.SetFieldEnd(p->Fc.GetFieldEnd() - GpOffset);
		p = (FCT*)(p->GetNextNodePtr());
	}
}

/**
 * @brief Stream insertion operator; outputs an FCT to an output stream.
 *
 * Delegates to the Print() method to output all entries in the field coordinate table.
 *
 * @param Os The output stream to write to.
 * @param Fct The FCT instance to output.
 * @return A reference to the output stream.
 */
ostream& operator<<(ostream& Os, const FCT& Fct) {
	Fct.Print(Os);
	return Os;
}
