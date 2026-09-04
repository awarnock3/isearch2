// Tests for src/nfield.hxx / src/nfield.cxx (class NUMERICFLD - one
// numeric-field entry: a byte offset paired with a numeric value).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "nfield.hxx"

TEST_CASE("NUMERICFLD default-constructs to zero", "[nfield]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcnfieldcxx): the constructor
	// used to leave GlobalStart/NumericValue indeterminate.
	NUMERICFLD f;
	REQUIRE(f.GetGlobalStart() == 0);
	REQUIRE(f.GetNumericValue() == 0.0);
}

TEST_CASE("NUMERICFLD SetGlobalStart/GetGlobalStart round-trip", "[nfield]") {
	NUMERICFLD f;
	f.SetGlobalStart(12345);
	REQUIRE(f.GetGlobalStart() == 12345);
}

TEST_CASE("NUMERICFLD SetNumericValue/GetNumericValue round-trip", "[nfield]") {
	NUMERICFLD f;
	f.SetNumericValue(3.14159);
	REQUIRE(f.GetNumericValue() == 3.14159);
}

TEST_CASE("NUMERICFLD default-constructed array elements are all zeroed", "[nfield]") {
	// Matches the real usage in src/nlist.cxx: `new NUMERICFLD[N]`
	// leaves every slot default-constructed until filled in.
	NUMERICFLD table[8];
	for (int i = 0; i < 8; i++) {
		REQUIRE(table[i].GetGlobalStart() == 0);
		REQUIRE(table[i].GetNumericValue() == 0.0);
	}
}
