// Tests for src/df.hxx / src/df.cxx (class DF - Data Field: a field
// name paired with its FCT of byte-offset occurrences).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "df.hxx"

#include <cstdio>

TEST_CASE("DF SetFieldName stores the name uppercased", "[df]") {
	DF df;
	df.SetFieldName(STRING("title"));
	STRING s;
	df.GetFieldName(&s);
	REQUIRE(s == "TITLE");
}

TEST_CASE("DF SetFct/GetFct round-trip", "[df]") {
	DF df;
	FCT fct;
	FC fc;
	fc.SetFieldStart(10u);
	fc.SetFieldEnd(20u);
	fct.AddEntry(fc);
	df.SetFct(fct);

	FCT out;
	df.GetFct(&out);
	REQUIRE(out.GetTotalEntries() == 1);
	FC outFc;
	out.GetEntry(1, &outFc);
	REQUIRE(outFc.GetFieldStart() == 10u);
	REQUIRE(outFc.GetFieldEnd() == 20u);
}

TEST_CASE("DF operator= deep-copies, independent of the source's later mutation", "[df]") {
	DF a;
	a.SetFieldName(STRING("abstract"));
	FCT fct;
	FC fc;
	fc.SetFieldStart(1u);
	fc.SetFieldEnd(2u);
	fct.AddEntry(fc);
	a.SetFct(fct);

	DF b;
	b = a;

	a.SetFieldName(STRING("mutated"));
	FCT fct2;
	fc.SetFieldStart(99u);
	fc.SetFieldEnd(100u);
	fct2.AddEntry(fc);
	a.SetFct(fct2);

	STRING s;
	b.GetFieldName(&s);
	REQUIRE(s == "ABSTRACT");
	FCT out;
	b.GetFct(&out);
	REQUIRE(out.GetTotalEntries() == 1);
	FC outFc;
	out.GetEntry(1, &outFc);
	REQUIRE(outFc.GetFieldStart() == 1u);
	REQUIRE(outFc.GetFieldEnd() == 2u);
}

TEST_CASE("DF operator= self-assignment leaves FieldName and Fct intact", "[df]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdfcxx): without a
	// self-assignment guard, `df = df;` reached FCT::operator=() with
	// source and target being the same FCT, which Clear()s itself
	// before reading its own (now-empty) entry count -- silently
	// wiping Fct's entries.
	DF df;
	df.SetFieldName(STRING("keep"));
	FCT fct;
	FC fc;
	fc.SetFieldStart(5u);
	fc.SetFieldEnd(6u);
	fct.AddEntry(fc);
	df.SetFct(fct);

	df = df;

	STRING s;
	df.GetFieldName(&s);
	REQUIRE(s == "KEEP");
	FCT out;
	df.GetFct(&out);
	REQUIRE(out.GetTotalEntries() == 1);
	FC outFc;
	out.GetEntry(1, &outFc);
	REQUIRE(outFc.GetFieldStart() == 5u);
	REQUIRE(outFc.GetFieldEnd() == 6u);
}

TEST_CASE("DF Write/Read round-trips through a file", "[df]") {
	DF original;
	original.SetFieldName(STRING("body"));
	FCT fct;
	FC fc;
	fc.SetFieldStart(3u);
	fc.SetFieldEnd(7u);
	fct.AddEntry(fc);
	original.SetFct(fct);

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	DF restored;
	restored.Read(fp);
	fclose(fp);

	STRING s;
	restored.GetFieldName(&s);
	REQUIRE(s == "BODY");
	FCT out;
	restored.GetFct(&out);
	REQUIRE(out.GetTotalEntries() == 1);
	FC outFc;
	out.GetEntry(1, &outFc);
	REQUIRE(outFc.GetFieldStart() == 3u);
	REQUIRE(outFc.GetFieldEnd() == 7u);
}
