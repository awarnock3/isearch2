// Tests for src/fprec.hxx / src/fprec.cxx (class FPREC - one entry in
// FPT's open-file table).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "fprec.hxx"

TEST_CASE("FPREC default-constructs with a null FilePointer, priority 0, not closed", "[fprec]") {
	FPREC f;
	REQUIRE(f.GetFilePointer() == nullptr);
	REQUIRE(f.GetPriority() == 0);
	REQUIRE(f.GetClosed() == GDT_FALSE);
}

TEST_CASE("FPREC SetFileName/GetFileName round-trip", "[fprec]") {
	FPREC f;
	f.SetFileName(STRING("/tmp/somefile.txt"));
	STRING s;
	f.GetFileName(&s);
	REQUIRE(s == "/tmp/somefile.txt");
}

TEST_CASE("FPREC SetFilePointer/GetFilePointer round-trip", "[fprec]") {
	FPREC f;
	FILE* marker = reinterpret_cast<FILE*>(0x1234);
	f.SetFilePointer(marker);
	REQUIRE(f.GetFilePointer() == marker);
}

TEST_CASE("FPREC SetPriority/GetPriority round-trip", "[fprec]") {
	FPREC f;
	f.SetPriority(5);
	REQUIRE(f.GetPriority() == 5);
}

TEST_CASE("FPREC SetClosed/GetClosed round-trip", "[fprec]") {
	FPREC f;
	f.SetClosed(GDT_TRUE);
	REQUIRE(f.GetClosed() == GDT_TRUE);
	f.SetClosed(GDT_FALSE);
	REQUIRE(f.GetClosed() == GDT_FALSE);
}

TEST_CASE("FPREC SetOpenMode/GetOpenMode round-trip", "[fprec]") {
	FPREC f;
	f.SetOpenMode(STRING("rb"));
	STRING s;
	f.GetOpenMode(&s);
	REQUIRE(s == "rb");
}

TEST_CASE("FPREC operator= copies every field, including Priority and Closed", "[fprec]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcfpreccxx): Priority and
	// Closed were never copied here, confirmed live via src/fpt.cxx's
	// `Fprec = Table[z-1];` followed by reading Fprec.GetClosed().
	FPREC a;
	a.SetFileName(STRING("/tmp/a.txt"));
	a.SetPriority(7);
	a.SetClosed(GDT_TRUE);
	FILE* marker = reinterpret_cast<FILE*>(0x5678);
	a.SetFilePointer(marker);
	a.SetOpenMode(STRING("r+b"));

	FPREC b;
	b = a;

	STRING s;
	b.GetFileName(&s);
	REQUIRE(s == "/tmp/a.txt");
	REQUIRE(b.GetPriority() == 7);
	REQUIRE(b.GetClosed() == GDT_TRUE);
	REQUIRE(b.GetFilePointer() == marker);
	b.GetOpenMode(&s);
	REQUIRE(s == "r+b");
}

TEST_CASE("FPREC operator= self-assignment leaves fields intact", "[fprec]") {
	FPREC f;
	f.SetPriority(3);
	f.SetClosed(GDT_TRUE);
	f = f;
	REQUIRE(f.GetPriority() == 3);
	REQUIRE(f.GetClosed() == GDT_TRUE);
}
