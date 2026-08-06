// Tests for src/marcdefs.hxx. Pure C structs/macros, no implementation
// file to link against -- these tests exercise the declarations
// directly.

#include "catch_amalgamated.hpp"

#include "marcdefs.hxx"

TEST_CASE("marcdefs.hxx is self-contained", "[marcdefs]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcmarcdefshxx): this header
	// used INT4 without including its definition (src/gdt.h). Merely
	// compiling this translation unit -- which includes only
	// marcdefs.hxx among the project's own headers -- is the
	// regression test for that: it wouldn't have compiled beforehand.
	MARC_REC rec{};
	REQUIRE(rec.length == 0);
}

TEST_CASE("special char delimiters have their documented ASCII values", "[marcdefs]") {
	REQUIRE(SUBFDELIM == '\037');
	REQUIRE(FIELDTERM == '\036');
	REQUIRE(RECTERM == '\035');
}

TEST_CASE("overlay structs match the real MARC on-disk widths", "[marcdefs]") {
	// Confirmed while processing this file: both overlays are entirely
	// char/char[] fields (alignof 1), so no compiler padding can sneak
	// in between members -- these sizes are load-bearing for reading
	// raw MARC bytes straight into the structs.
	REQUIRE(sizeof(MARC_LEADER_OVER) == 24);
	REQUIRE(sizeof(MARC_DIRENTRY_OVER) == 12);
}

TEST_CASE("MARC_FIELD/MARC_SUBFIELD link into a small list", "[marcdefs]") {
	MARC_SUBFIELD sub1{};
	sub1.code = 'a';
	sub1.next = nullptr;

	MARC_FIELD field{};
	field.subfield = &sub1;
	field.lastsub = &sub1;
	field.next = nullptr;
	field.indicator1 = ' ';
	field.indicator2 = ' ';
	field.length = 0;

	REQUIRE(field.subfield == field.lastsub);
	REQUIRE(field.subfield->code == 'a');
	REQUIRE(field.subfield->next == nullptr);
	REQUIRE(field.next == nullptr);
}
