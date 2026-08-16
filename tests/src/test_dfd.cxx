// Tests for src/dfd.hxx / src/dfd.cxx (class DFD - Data Field
// Definition: a file number plus an ATTRLIST of attributes).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "dfd.hxx"

TEST_CASE("DFD default-constructs with file number 0", "[dfd]") {
	DFD dfd;
	REQUIRE(dfd.GetFileNumber() == 0);
}

TEST_CASE("DFD SetFileNumber/GetFileNumber round-trip", "[dfd]") {
	DFD dfd;
	dfd.SetFileNumber(42);
	REQUIRE(dfd.GetFileNumber() == 42);
}

TEST_CASE("DFD SetFieldName/GetFieldName round-trip, stored uppercased", "[dfd]") {
	DFD dfd;
	dfd.SetFieldName(STRING("title"));
	STRING s;
	dfd.GetFieldName(&s);
	REQUIRE(s == "TITLE");
}

TEST_CASE("DFD SetFieldType/GetFieldType round-trip, stored uppercased", "[dfd]") {
	DFD dfd;
	dfd.SetFieldType(STRING("text"));
	STRING s;
	dfd.GetFieldType(&s);
	REQUIRE(s == "TEXT");
}

TEST_CASE("DFD SetAttributes/GetAttributes round-trip", "[dfd]") {
	DFD dfd;
	ATTRLIST attrs;
	ATTR attr;
	attr.SetSetId(STRING("set1"));
	attr.SetAttrType(1);
	attrs.AddEntry(attr);
	dfd.SetAttributes(attrs);

	ATTRLIST out;
	dfd.GetAttributes(&out);
	REQUIRE(out.GetTotalEntries() == 1);
}

TEST_CASE("DFD operator= copies fields independently of the source", "[dfd]") {
	DFD a;
	a.SetFileNumber(7);
	a.SetFieldName(STRING("abstract"));

	DFD b;
	b = a;

	a.SetFileNumber(99);
	a.SetFieldName(STRING("mutated"));

	REQUIRE(b.GetFileNumber() == 7);
	STRING s;
	b.GetFieldName(&s);
	REQUIRE(s == "ABSTRACT");
}

TEST_CASE("DFD operator= self-assignment leaves fields intact", "[dfd]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdfdcxx): without a
	// self-assignment guard, `dfd = dfd;` reached ATTRLIST::operator=()
	// with source and target being the same ATTRLIST, which
	// deletes/reinitializes itself before reading its own (now-empty)
	// entry count -- silently wiping Attributes, including the field
	// name/type stored inside it.
	DFD dfd;
	dfd.SetFileNumber(5);
	dfd.SetFieldName(STRING("keep"));
	dfd.SetFieldType(STRING("text"));

	dfd = dfd;

	REQUIRE(dfd.GetFileNumber() == 5);
	STRING s;
	dfd.GetFieldName(&s);
	REQUIRE(s == "KEEP");
	dfd.GetFieldType(&s);
	REQUIRE(s == "TEXT");
}
