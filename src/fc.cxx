/*@@@
File:		fc.cxx
Version:	1.00
Description:	Class FC - Field Coordinates
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "fc.hxx"

/**
 * @brief Constructs an FC with FieldStart/FieldEnd zero-initialized.
 */
FC::FC() {
  // BUGFIX #1 (docs/BUG_CATALOG.md#srcfchxx): FieldStart/FieldEnd were
  // left indeterminate here -- the same "indeterminate primitive
  // member" category already fixed in RESULT's/NUMERICFLD's/
  // NUMERICLIST's constructors.
  FieldStart = 0;
  FieldEnd = 0;
}

void FC::SetFieldStart(const GPTYPE NewFieldStart) {
	FieldStart = NewFieldStart;
}

GPTYPE FC::GetFieldStart() {
	return FieldStart;
}

void FC::SetFieldEnd(const GPTYPE NewFieldEnd) {
	FieldEnd = NewFieldEnd;
}

GPTYPE FC::GetFieldEnd() {
	return FieldEnd;
}

/**
 * @brief Serializes FieldStart/FieldEnd as text to fp.
 * @param fp Destination file, open for writing.
 */
void FC::Write(PFILE fp) const {
	// BUGFIX #2 (docs/BUG_CATALOG.md#srcfchxx): FieldStart/FieldEnd are
	// GPTYPE (UINT4, unsigned); %d is signed, a format/argument-type
	// mismatch that's undefined behavior per the C standard regardless
	// of whether it happens to round-trip correctly on any given
	// platform (it does on this one -- see the Read() comment below).
	// %u matches the actual type.
	fprintf(fp, "%u\n%u\n", FieldStart, FieldEnd);
}

/**
 * @brief Reads FieldStart/FieldEnd back from text previously written
 * by Write().
 * @param fp Source file, open for reading.
 */
void FC::Read(PFILE fp) {
	// BUGFIX #2 (cont'd): GetInt() returns a signed 32-bit INT, which
	// can't represent GPTYPE values above INT_MAX. On this platform the
	// original %d/GetInt() pairing happened to round-trip anyway (both
	// printf's signed reinterpretation of the unsigned bit pattern and
	// atoi()'s long-to-int truncation wrap modulo 2^32 the same way),
	// so this isn't a demonstrated failure here -- but it's unspecified/
	// implementation-defined behavior, not a guarantee, the same
	// "works today only by platform coincidence" category as this
	// file's own header-self-containment fix. GetLong() (LONG = 64-bit
	// long here) parses the full unsigned 32-bit range as a valid LONG
	// with no narrowing, which is what actually makes the round-trip
	// portable rather than accidental.
	STRING s;
	s.FGet(fp, 16);
	FieldStart = s.GetLong();
	s.FGet(fp, 16);
	FieldEnd = s.GetLong();
}

void FC::FlipBytes() {
	GpSwab(&FieldStart);
	GpSwab(&FieldEnd);
}

ostream& operator<<(ostream& Os, const FC& Fc) {
	Os << Fc.FieldStart << ' ' << Fc.FieldEnd << endl;
	return Os;
}

FC::~FC() {
}
