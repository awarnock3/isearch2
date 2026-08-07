// Tests for src/dfdt.hxx / src/dfdt.cxx (class DFDT - Data Field
// Definitions Table). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "dfdt.hxx"
#include "dfd.hxx"
#include "string.hxx"

TEST_CASE("DFDT default-constructs empty and unchanged", "[dfdt]") {
	DFDT dt;
	REQUIRE(dt.GetTotalEntries() == 0);
	REQUIRE(dt.GetChanged() == 0);
}

TEST_CASE("DFDT AddEntry/GetEntry round-trips (1-based)", "[dfdt]") {
	DFDT dt;
	DFD d;
	d.SetFieldName(STRING("title"));
	dt.AddEntry(d);
	REQUIRE(dt.GetTotalEntries() == 1);
	REQUIRE(dt.GetChanged() == 1);

	DFD out;
	dt.GetEntry(1, &out);
	STRING s;
	out.GetFieldName(&s);
	REQUIRE(s == "TITLE");
}

TEST_CASE("DFDT AddEntry updates an existing entry by field name instead of duplicating", "[dfdt]") {
	DFDT dt;
	DFD d;
	d.SetFieldName(STRING("title"));
	dt.AddEntry(d);
	REQUIRE(dt.GetTotalEntries() == 1);

	DFD d2;
	d2.SetFieldName(STRING("TITLE"));  // same field, different case in the call
	dt.AddEntry(d2);
	REQUIRE(dt.GetTotalEntries() == 1);  // still just one entry
}

TEST_CASE("DFDT GetDfdRecord finds by field name case-insensitively", "[dfdt]") {
	DFDT dt;
	DFD d;
	d.SetFieldName(STRING("title"));
	dt.AddEntry(d);

	DFD out;
	dt.GetDfdRecord(STRING("Title"), &out);
	STRING s;
	out.GetFieldName(&s);
	REQUIRE(s == "TITLE");
}

TEST_CASE("DFDT GetDfdRecord leaves *DfdRecord untouched if not found", "[dfdt]") {
	// BUGFIX #3 regression: this used to try (ineffectively) to null the
	// by-value pointer parameter itself. Confirm the documented
	// leave-untouched contract instead.
	DFDT dt;
	DFD out;
	out.SetFieldName(STRING("sentinel"));
	dt.GetDfdRecord(STRING("does-not-exist"), &out);
	STRING s;
	out.GetFieldName(&s);
	REQUIRE(s == "SENTINEL");
}

TEST_CASE("DFDT grows past its initial capacity via Expand", "[dfdt]") {
	DFDT dt;
	INT i;
	STRING name;
	for (i = 0; i < 501; i++) {
		DFD d;
		name = "field";
		name.Cat(STRING(i));
		d.SetFieldName(name);
		dt.AddEntry(d);
	}
	REQUIRE(dt.GetTotalEntries() == 501);
}

TEST_CASE("DFDT copy constructor deep-copies independently of the source", "[dfdt]") {
	DFDT original;
	DFD d;
	d.SetFieldName(STRING("original-field"));
	original.AddEntry(d);

	DFDT copy(original);
	DFD another;
	another.SetFieldName(STRING("second-field"));
	copy.AddEntry(another);

	// If Table were shared (the pre-fix shallow copy), this AddEntry
	// on the copy would also grow the original.
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);
}

TEST_CASE("DFDT operator= deep-copies and survives self-assignment", "[dfdt]") {
	DFDT original;
	DFD d;
	d.SetFieldName(STRING("original-field"));
	original.AddEntry(d);

	DFDT copy;
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 1);

	DFD another;
	another.SetFieldName(STRING("second-field"));
	copy.AddEntry(another);
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);

	// BUGFIX #2 regression: self-assignment used to silently empty the
	// table (delete+Initialize ran before the source's entry count was
	// read).
	original = original;
	REQUIRE(original.GetTotalEntries() == 1);
	STRING s;
	DFD out;
	original.GetEntry(1, &out);
	out.GetFieldName(&s);
	REQUIRE(s == "ORIGINAL-FIELD");
}

TEST_CASE("DFDT copies survive independent destruction without a double-free", "[dfdt]") {
	// Same shape as the standalone repro that originally confirmed this
	// bug (see docs/AUTOPILOT_LOG.md#srcdfdthxx): copy-construct, then
	// destroy both.
	DFDT* a = new DFDT();
	DFD entry;
	entry.SetFieldName(STRING("a-field"));
	a->AddEntry(entry);

	DFDT* b = new DFDT(*a);
	delete b;
	delete a;
	SUCCEED("no ASan/UBSan failure on independent destruction");
}
