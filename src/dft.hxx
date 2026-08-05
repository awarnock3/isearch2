/*@@@
File:		dft.hxx
Version:	1.00
Description:	Class DFT - Data Field Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef DFT_HXX
#define DFT_HXX

#include "defs.hxx"  // BUGFIX #1: was commented out; INT/PFILE below need it.
#include "df.hxx"    // BUGFIX #1: was commented out; DF/PDF below need it.

// Data Field Table: a dynamically resizing array of DF (Data Field) entries,
// one per named field found while parsing a document, 1-indexed to match
// GetEntry/AddEntry's own convention below.
class DFT {
public:
	DFT();
	// BUGFIX #2: added -- previously absent, so copy-constructing a DFT
	// (e.g. `DFT b = a;`, pass/return by value) used the compiler-generated
	// copy constructor, which shallow-copies the Table pointer instead of
	// the entries it points to. Both copies' destructors then delete the
	// same heap array, corrupting memory on the first destruction and
	// double-freeing on the second. Deep-copies entries the same way
	// operator= does, below.
	DFT(const DFT& OtherDft);
	// Allocates a fresh, empty 100-entry Table. Used internally by the
	// constructors; calling it directly on a DFT that already owns entries
	// overwrites Table without freeing it first and leaks the old array,
	// so prefer assignment (operator=) or a fresh DFT instead.
	void Init();
	DFT& operator=(const DFT& OtherDft);
	// Appends a copy of DfRecord, expanding Table first if it's full.
	void AddEntry(const DF& DfRecord);
	// Copies the Index'th entry (1-based) into *DfRecord; leaves *DfRecord
	// untouched if Index is out of [1, GetTotalEntries()] range.
	void GetEntry(const INT Index, PDF DfRecord) const;
	// Grows Table's capacity by 100 entries.
	void Expand();
	// Shrinks Table's capacity down to exactly GetTotalEntries().
	void CleanUp();
	// Reallocates Table to hold exactly Entries entries, copying over
	// min(Entries, GetTotalEntries()) existing ones.
	void Resize(const INT Entries);
	INT GetTotalEntries() const;
	// Serializes the table as text to fp.
	void Write(PFILE fp) const;
	// Reads the table back from text previously written by Write().
	void Read(PFILE fp);
	~DFT();
private:
	PDF Table;
	INT TotalEntries;
	INT MaxEntries;
};

typedef DFT* PDFT;

#endif
