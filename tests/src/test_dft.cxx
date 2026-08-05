// Tests for src/dft.hxx / src/dft.cxx (class DFT - Data Field Table).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "dft.hxx"

#include <cstdio>

// Fills Out in place rather than returning DF by value: DF (like FCT, which
// it embeds via its Fct member) declares operator= but no copy constructor,
// so returning one by value would need the compiler's deprecated implicit
// copy constructor. See BUG_CATALOG.md's "found but out of scope" note
// under src/dft.hxx.
//
// DF::SetFieldName upper-cases whatever it's given, so tests pass
// already-uppercase names to keep round-trip comparisons exact.
static void MakeDf(DF* Out, const CHR* Name) {
	Out->SetFieldName(STRING(Name));
}

static STRING NameOf(const DFT& Dft, INT Index) {
	DF df;
	Dft.GetEntry(Index, &df);
	STRING Name;
	df.GetFieldName(&Name);
	return Name;
}

TEST_CASE("DFT starts empty", "[dft]") {
	DFT dft;
	REQUIRE(dft.GetTotalEntries() == 0);
}

TEST_CASE("DFT AddEntry grows the count and preserves insertion order", "[dft]") {
	DFT dft;
	DF title, author;
	MakeDf(&title, "TITLE");
	MakeDf(&author, "AUTHOR");
	dft.AddEntry(title);
	dft.AddEntry(author);
	REQUIRE(dft.GetTotalEntries() == 2);
	REQUIRE(NameOf(dft, 1) == "TITLE");
	REQUIRE(NameOf(dft, 2) == "AUTHOR");
}

TEST_CASE("DFT AddEntry expands past the initial 100-entry capacity", "[dft]") {
	DFT dft;
	DF field;
	MakeDf(&field, "FIELD");
	for (INT i = 0; i < 150; i++) {
		dft.AddEntry(field);
	}
	REQUIRE(dft.GetTotalEntries() == 150);
}

TEST_CASE("DFT GetEntry leaves its output untouched for an out-of-range index", "[dft]") {
	DFT dft;
	DF only;
	MakeDf(&only, "ONLY");
	dft.AddEntry(only);

	DF out;
	MakeDf(&out, "UNTOUCHED");
	dft.GetEntry(0, &out);
	STRING Name;
	out.GetFieldName(&Name);
	REQUIRE(Name == "UNTOUCHED");

	dft.GetEntry(99, &out);
	out.GetFieldName(&Name);
	REQUIRE(Name == "UNTOUCHED");
}

TEST_CASE("DFT copy constructor deep-copies entries independently of the source", "[dft]") {
	// BUGFIX #2 coverage: before the fix, this triggered a
	// compiler-generated shallow copy of Table and a double-free/
	// use-after-free in ~DFT() once both objects went out of scope --
	// exactly what this test constructs. Passing under ASan is the point.
	DFT original;
	DF one, two, three;
	MakeDf(&one, "ONE");
	MakeDf(&two, "TWO");
	MakeDf(&three, "THREE");
	original.AddEntry(one);
	original.AddEntry(two);

	DFT copy = original;
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(NameOf(copy, 1) == "ONE");

	copy.AddEntry(three);
	REQUIRE(copy.GetTotalEntries() == 3);
	REQUIRE(original.GetTotalEntries() == 2);
}

TEST_CASE("DFT operator= deep-copies entries independently of the source", "[dft]") {
	DFT original;
	DF one, placeholder;
	MakeDf(&one, "ONE");
	MakeDf(&placeholder, "PLACEHOLDER");
	original.AddEntry(one);

	DFT copy;
	copy.AddEntry(placeholder);
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 1);
	REQUIRE(NameOf(copy, 1) == "ONE");
}

TEST_CASE("DFT CleanUp shrinks capacity without losing entries", "[dft]") {
	DFT dft;
	DF a, b;
	MakeDf(&a, "A");
	MakeDf(&b, "B");
	dft.AddEntry(a);
	dft.AddEntry(b);
	dft.CleanUp();
	REQUIRE(dft.GetTotalEntries() == 2);
	REQUIRE(NameOf(dft, 1) == "A");
	REQUIRE(NameOf(dft, 2) == "B");
}

TEST_CASE("DFT Write/Read round-trips every entry through a file", "[dft]") {
	DFT original;
	DF alpha, beta;
	MakeDf(&alpha, "ALPHA");
	MakeDf(&beta, "BETA");
	original.AddEntry(alpha);
	original.AddEntry(beta);

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	DFT restored;
	restored.Read(fp);
	fclose(fp);

	REQUIRE(restored.GetTotalEntries() == 2);
	REQUIRE(NameOf(restored, 1) == "ALPHA");
	REQUIRE(NameOf(restored, 2) == "BETA");
}
