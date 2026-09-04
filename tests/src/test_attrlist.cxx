// Tests for src/attrlist.hxx / src/attrlist.cxx (class ATTRLIST -
// Attribute List). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "attrlist.hxx"
#include "string.hxx"

TEST_CASE("ATTRLIST default-constructs empty", "[attrlist]") {
	ATTRLIST al;
	REQUIRE(al.GetTotalEntries() == 0);
}

TEST_CASE("ATTRLIST AddEntry/GetEntry round-trips (1-based)", "[attrlist]") {
	ATTRLIST al;
	ATTR a;
	a.SetSetId(STRING("Bib-1"));
	a.SetAttrType(1);
	al.AddEntry(a);
	REQUIRE(al.GetTotalEntries() == 1);

	ATTR out;
	al.GetEntry(1, &out);
	STRING s;
	out.GetSetId(&s);
	REQUIRE(s == "Bib-1");
	REQUIRE(out.GetAttrType() == 1);
}

TEST_CASE("ATTRLIST SetEntry overwrites an existing entry", "[attrlist]") {
	ATTRLIST al;
	ATTR a;
	a.SetSetId(STRING("original"));
	al.AddEntry(a);

	ATTR replacement;
	replacement.SetSetId(STRING("replaced"));
	al.SetEntry(1, replacement);

	ATTR out;
	al.GetEntry(1, &out);
	STRING s;
	out.GetSetId(&s);
	REQUIRE(s == "replaced");
}

TEST_CASE("ATTRLIST DeleteEntry removes and shifts down", "[attrlist]") {
	ATTRLIST al;
	ATTR a, b, c;
	a.SetSetId(STRING("a"));
	b.SetSetId(STRING("b"));
	c.SetSetId(STRING("c"));
	al.AddEntry(a);
	al.AddEntry(b);
	al.AddEntry(c);

	al.DeleteEntry(2);  // remove "b"
	REQUIRE(al.GetTotalEntries() == 2);

	STRING s;
	ATTR out;
	al.GetEntry(1, &out);
	out.GetSetId(&s);
	REQUIRE(s == "a");
	al.GetEntry(2, &out);
	out.GetSetId(&s);
	REQUIRE(s == "c");
}

TEST_CASE("ATTRLIST SetValue/GetValue round-trips by SetId+AttrType", "[attrlist]") {
	ATTRLIST al;
	al.SetValue(STRING("Bib-1"), 1, STRING("some-value"));

	STRING out;
	REQUIRE(al.GetValue(STRING("Bib-1"), 1, &out) == GDT_TRUE);
	REQUIRE(out == "some-value");

	// Unknown SetId/AttrType combination is not found.
	STRING missing;
	REQUIRE(al.GetValue(STRING("Bib-1"), 99, &missing) == GDT_FALSE);
	REQUIRE(missing.GetLength() == 0);
}

TEST_CASE("ATTRLIST grows past its initial capacity via Expand", "[attrlist]") {
	ATTRLIST al;
	INT i;
	for (i = 0; i < 20; i++) {
		ATTR a;
		al.AddEntry(a);
	}
	REQUIRE(al.GetTotalEntries() == 20);
}

TEST_CASE("ATTRLIST copy constructor deep-copies independently of the source", "[attrlist]") {
	ATTRLIST original;
	ATTR a;
	a.SetSetId(STRING("original-id"));
	original.AddEntry(a);

	ATTRLIST copy(original);
	ATTR another;
	another.SetSetId(STRING("second-id"));
	copy.AddEntry(another);

	// If Table were shared (the pre-fix shallow copy), this AddEntry
	// on the copy would also grow the original.
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);
}

TEST_CASE("ATTRLIST operator= deep-copies and survives self-assignment", "[attrlist]") {
	ATTRLIST original;
	ATTR a;
	a.SetSetId(STRING("original-id"));
	original.AddEntry(a);

	ATTRLIST copy;
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 1);

	ATTR another;
	another.SetSetId(STRING("second-id"));
	copy.AddEntry(another);
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);

	// BUGFIX #2 regression: self-assignment used to silently empty the
	// list (delete+Init ran before the source's entry count was read).
	original = original;
	REQUIRE(original.GetTotalEntries() == 1);
	STRING s;
	ATTR out;
	original.GetEntry(1, &out);
	out.GetSetId(&s);
	REQUIRE(s == "original-id");
}

TEST_CASE("ATTRLIST copies survive independent destruction without a double-free", "[attrlist]") {
	// Same shape as the standalone repro that originally confirmed this
	// bug (see docs/AUTOPILOT_LOG.md#srcattrlisthxx): copy-construct,
	// then destroy both.
	ATTRLIST* a = new ATTRLIST();
	ATTR entry;
	entry.SetSetId(STRING("a-entry"));
	a->AddEntry(entry);

	ATTRLIST* b = new ATTRLIST(*a);
	delete b;
	delete a;
	SUCCEED("no ASan/UBSan failure on independent destruction");
}
