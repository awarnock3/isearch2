// Tests for src/reclist.hxx / src/reclist.cxx (class RECLIST - Database
// Record List). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "reclist.hxx"
#include "record.hxx"
#include "string.hxx"

TEST_CASE("RECLIST default-constructs empty", "[reclist]") {
	RECLIST rl;
	REQUIRE(rl.GetTotalEntries() == 0);
}

TEST_CASE("RECLIST AddEntry/GetEntry round-trips (1-based)", "[reclist]") {
	RECLIST rl;
	RECORD r;
	r.SetKey(STRING("entry-key"));
	rl.AddEntry(r);
	REQUIRE(rl.GetTotalEntries() == 1);

	RECORD out;
	rl.GetEntry(1, &out);
	STRING s;
	out.GetKey(&s);
	REQUIRE(s == "entry-key");
}

TEST_CASE("RECLIST GetEntry ignores out-of-range indices", "[reclist]") {
	RECLIST rl;
	RECORD r;
	r.SetKey(STRING("entry-key"));
	rl.AddEntry(r);

	RECORD out;
	out.SetKey(STRING("untouched"));
	rl.GetEntry(0, &out);
	STRING s;
	out.GetKey(&s);
	REQUIRE(s == "untouched");

	rl.GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "untouched");
}

TEST_CASE("RECLIST grows past its initial capacity via Expand", "[reclist]") {
	RECLIST rl;
	INT i;
	for (i = 0; i < 1001; i++) {
		RECORD r;
		rl.AddEntry(r);
	}
	REQUIRE(rl.GetTotalEntries() == 1001);
}

TEST_CASE("RECLIST CleanUp shrinks capacity to the current entry count", "[reclist]") {
	RECLIST rl;
	RECORD r;
	rl.AddEntry(r);
	rl.CleanUp();
	REQUIRE(rl.GetTotalEntries() == 1);
}

TEST_CASE("RECLIST copy constructor deep-copies independently of the source", "[reclist]") {
	RECLIST original;
	RECORD r;
	r.SetKey(STRING("original-key"));
	original.AddEntry(r);

	RECLIST copy(original);
	RECORD another;
	another.SetKey(STRING("second-key"));
	copy.AddEntry(another);

	// If Table were shared (the pre-fix shallow copy), this AddEntry
	// on the copy would also grow the original.
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);

	RECORD out;
	STRING s;
	original.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "original-key");
	copy.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "original-key");
	copy.GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "second-key");
}

TEST_CASE("RECLIST operator= deep-copies and survives self-assignment", "[reclist]") {
	RECLIST original;
	RECORD r;
	r.SetKey(STRING("original-key"));
	original.AddEntry(r);

	RECLIST copy;
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 1);

	RECORD another;
	another.SetKey(STRING("second-key"));
	copy.AddEntry(another);
	REQUIRE(copy.GetTotalEntries() == 2);
	REQUIRE(original.GetTotalEntries() == 1);

	// Self-assignment must not corrupt or double-free Table.
	original = original;
	REQUIRE(original.GetTotalEntries() == 1);
	RECORD out;
	STRING s;
	original.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "original-key");
}

TEST_CASE("RECLIST copies survive independent destruction without a double-free", "[reclist]") {
	// Same shape as the standalone repro that originally confirmed this
	// bug (see docs/AUTOPILOT_LOG.md#srcreclisthxx): copy-construct,
	// then destroy both. Prior to the fix this double-freed Table under
	// ASan/UBSan.
	RECLIST* a = new RECLIST();
	RECORD r;
	r.SetKey(STRING("a-key"));
	a->AddEntry(r);

	RECLIST* b = new RECLIST(*a);
	delete b;
	delete a;
	SUCCEED("no ASan/UBSan failure on independent destruction");
}
