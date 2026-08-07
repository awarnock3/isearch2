// Tests for src/intfield.hxx / src/intfield.cxx (class INTERVALFLD -
// one numeric-interval entry: a byte offset paired with a
// [StartValue, EndValue] range).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "intfield.hxx"

TEST_CASE("INTERVALFLD default-constructs to -1/0/0", "[intfield]") {
	INTERVALFLD f;
	REQUIRE(f.GetGlobalStart() == -1);
	REQUIRE(f.GetStartValue() == 0.0);
	REQUIRE(f.GetEndValue() == 0.0);
}

TEST_CASE("INTERVALFLD SetGlobalStart/GetGlobalStart round-trip", "[intfield]") {
	INTERVALFLD f;
	f.SetGlobalStart(42);
	REQUIRE(f.GetGlobalStart() == 42);
}

TEST_CASE("INTERVALFLD SetStartValue/SetEndValue round-trip", "[intfield]") {
	INTERVALFLD f;
	f.SetStartValue(1.5);
	f.SetEndValue(9.5);
	REQUIRE(f.GetStartValue() == 1.5);
	REQUIRE(f.GetEndValue() == 9.5);
}

TEST_CASE("INTERVALFLD copy constructor copies GlobalStart/StartValue/EndValue", "[intfield]") {
	INTERVALFLD a;
	a.SetGlobalStart(7);
	a.SetStartValue(2.0);
	a.SetEndValue(8.0);

	INTERVALFLD b(a);
	REQUIRE(b.GetGlobalStart() == 7);
	REQUIRE(b.GetStartValue() == 2.0);
	REQUIRE(b.GetEndValue() == 8.0);
}

TEST_CASE("INTERVALFLD operator= copies GlobalStart/StartValue/EndValue", "[intfield]") {
	// operator='s signature is non-standard (returns by value, takes a
	// non-const reference -- see docs/BUG_CATALOG.md#srcintfieldcxx for
	// why this wasn't changed) but its body is a plain field-by-field
	// copy, exercised here the only way its signature allows: assigning
	// from a named lvalue.
	INTERVALFLD a;
	a.SetGlobalStart(11);
	a.SetStartValue(3.0);
	a.SetEndValue(13.0);

	INTERVALFLD b;
	b = a;
	REQUIRE(b.GetGlobalStart() == 11);
	REQUIRE(b.GetStartValue() == 3.0);
	REQUIRE(b.GetEndValue() == 13.0);
}

TEST_CASE("INTERVALFLD default-constructed array elements are all -1/0/0", "[intfield]") {
	// Matches the real usage in src/intlist.cxx:
	// `table = new INTERVALFLD[50*Ncoords];`.
	INTERVALFLD table[8];
	for (int i = 0; i < 8; i++) {
		REQUIRE(table[i].GetGlobalStart() == -1);
		REQUIRE(table[i].GetStartValue() == 0.0);
		REQUIRE(table[i].GetEndValue() == 0.0);
	}
}

TEST_CASE("INTERVALFLD inherited NumericValue accessors are independent of GlobalStart", "[intfield]") {
	// Documents the shadowing found this turn (see
	// docs/BUG_CATALOG.md#srcintfieldcxx): INTERVALFLD's own
	// GlobalStart/GetGlobalStart()/SetGlobalStart() hide NUMERICFLD's,
	// but GetNumericValue()/SetNumericValue() are inherited unshadowed
	// and operate on a separate NUMERICFLD-subobject field that no
	// INTERVALFLD-specific code reads or writes.
	INTERVALFLD f;
	f.SetGlobalStart(5);
	f.SetNumericValue(99.0);
	REQUIRE(f.GetGlobalStart() == 5);
	REQUIRE(f.GetNumericValue() == 99.0);
}
