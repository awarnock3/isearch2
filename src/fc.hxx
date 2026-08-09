/*@@@
File:		fc.hxx
Version:	1.00
Description:	Class FC - Field Coordinates
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef FC_HXX
#define FC_HXX

#include <iostream>
#include "defs.hxx"  // BUGFIX #1: was commented out; GPTYPE/PFILE below need it.

// Field Coordinates: a [FieldStart, FieldEnd) byte-offset pair delimiting
// one field occurrence within an indexed document.
class FC {
public:
	/// Constructs an FC; FieldStart/FieldEnd are left unset until SetFieldStart()/SetFieldEnd() or Read() are called.
	FC();
	/// Sets the field's start byte offset.
	void SetFieldStart(const GPTYPE NewFieldStart);
	/// Returns the field's start byte offset.
	GPTYPE GetFieldStart();
	/// Sets the field's end byte offset.
	void SetFieldEnd(const GPTYPE NewFieldEnd);
	/// Returns the field's end byte offset.
	GPTYPE GetFieldEnd();
	/// Serializes the pair as text to fp.
	void Write(PFILE fp) const;
	/// Reads the pair back from text previously written by Write().
	void Read(PFILE fp);
	/// Swaps byte order of both fields in place, for cross-endian file I/O.
	void FlipBytes();
	// BUGFIX #1: qualified std::ostream so this header doesn't depend on
	// some other translation unit's "using namespace std;" being in
	// effect first (e.g. string.hxx's, seen only transitively today).
	/// Writes "FieldStart FieldEnd" (space-separated, newline-terminated) to os.
	friend std::ostream& operator<<(std::ostream& os, const FC& Fc);
	/// Destroys the FC (no owned resources to release).
	~FC();
private:
	GPTYPE FieldStart;
	GPTYPE FieldEnd;
};

typedef FC* PFC;

#endif
