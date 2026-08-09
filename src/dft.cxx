/*@@@
File:		dft.cxx
Version:	1.00
Description:	Class DFT - Data Field Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"

DFT::DFT() {
	Init();
}

void DFT::Init() {
	Table = new DF[100];
	TotalEntries = 0;
	MaxEntries = 100;
}

// BUGFIX #2: see the declaration in dft.hxx for why this is needed.
DFT::DFT(const DFT& OtherDft) {
	Init();
	INT y = OtherDft.GetTotalEntries();
	INT x;
	DF df;
	for (x=1; x<=y; x++) {
		OtherDft.GetEntry(x, &df);
		AddEntry(df);
	}
}

/**
 * @brief Deep-copies OtherDft's entries into this DFT, replacing
 * whatever this table already held.
 * @param OtherDft The source table to copy from.
 * @return A reference to this DFT.
 */
DFT& DFT::operator=(const DFT& OtherDft) {
	// BUGFIX #3 (docs/BUG_CATALOG.md#srcdfthxx): no self-assignment
	// guard -- delete [] Table; Init(); ran before OtherDft.GetTotalEntries()
	// was read, so `x = x;` saw an already-emptied table (the same
	// object) and copied nothing back, silently wiping it. Same shape
	// of bug already found and fixed in ATTRLIST's/DFDT's/STRLIST's
	// operator='s.
	if (this == &OtherDft) {
		return *this;
	}
	if (Table) {
		delete [] Table;
	}
	Init();
	INT y = OtherDft.GetTotalEntries();
	INT x;
	DF df;
	for (x=1; x<=y; x++) {
		OtherDft.GetEntry(x, &df);
		AddEntry(df);
	}
	return *this;
}

void DFT::AddEntry(const DF& DfRecord) {
	if (TotalEntries == MaxEntries) {
		Expand();
	}
	Table[TotalEntries] = DfRecord;
	TotalEntries++;
}

void DFT::GetEntry(const INT Index, PDF DfRecord) const {
	if ( (Index > 0) && (Index <= TotalEntries) ) {
		*DfRecord = Table[Index-1];
	}
}

void DFT::Expand() {
	Resize(TotalEntries+100);
}

void DFT::CleanUp() {
	Resize(TotalEntries);
}

void DFT::Resize(const INT Entries) {
	PDF Temp = new DF[Entries];
	INT RecsToCopy;
	INT x;
	if (Entries >= TotalEntries) {
		RecsToCopy = TotalEntries;
	} else {
		RecsToCopy = Entries;
		TotalEntries = Entries;
	}
	for (x=0; x<RecsToCopy; x++) {
		Temp[x] = Table[x];
	}
	if (Table)
		delete [] Table;
	Table = Temp;
	MaxEntries = Entries;
}

INT DFT::GetTotalEntries() const {
	return TotalEntries;
}

void DFT::Write(PFILE fp) const {
	fprintf(fp, "%d\n", TotalEntries);
	INT x;
	for (x=0; x<TotalEntries; x++) {
		Table[x].Write(fp);
	}
}

void DFT::Read(PFILE fp) {
	STRING s;
	INT n, x;
	DF Df;
	DFT Dft;
	s.FGet(fp, 16);
	n = s.GetInt();
	for (x=0; x<n; x++) {
		Df.Read(fp);
		Dft.AddEntry(Df);
	}
	*this = Dft;
}

DFT::~DFT() {
	if (Table)
		delete [] Table;
}
