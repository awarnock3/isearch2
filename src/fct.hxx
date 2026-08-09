/*@@@
File:		fct.hxx
Version:	1.00
Description:	Class FCT - Field Coordinate Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/**
 * @file fct.hxx
 * @brief Field Coordinate Table (FCT) — ordered list of field coordinates for indexed document terms.
 *
 * Defines the FCT class, an ordered circular list (inheriting from VLIST) that stores
 * field-coordinate pairs (FC objects) for term occurrences within indexed documents.
 * FCT manages a collection of field positions and provides operations to add, retrieve,
 * sort, serialize, and offset field coordinates. Used by the indexing and search engines
 * to track and manipulate field boundaries during document processing.
 */

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
	/// @brief Default constructor; initializes an empty field coordinate table.
	FCT();

	/// @brief Assignment operator; copies all entries from another FCT.
	FCT& operator=(const FCT& OtherFct);

	/// @brief Appends a copy of FcRecord as a new node.
	void AddEntry(const FC& FcRecord);

	/// @brief Retrieves the Index'th entry (1-based) into the provided FC pointer.
	void GetEntry(const INT Index, FC* FcRecord) const;

	/// @brief Sorts entries in place by field start position.
	void SortByFc();

	/// @brief Serializes the table as text to a file pointer.
	void Write(PFILE fp) const;

	/// @brief Reads the table back from text previously written by Write().
	void Read(PFILE fp);

	/// @brief Prints all entries to an output stream.
	void Print(std::ostream& Os) const;

	/// @brief Shifts every entry's field boundaries left by a given offset.
	void SubtractOffset(const GPTYPE GpOffset);

	// BUGFIX #1: qualified std::ostream so this header doesn't depend on
	// some other translation unit's "using namespace std;" being in effect
	// first.
	/// @brief Stream insertion operator; outputs the FCT via Print().
	friend std::ostream& operator<<(std::ostream& os, const FCT& Fct);
private:
	FC Fc;
};

typedef FCT* PFCT;

#endif
