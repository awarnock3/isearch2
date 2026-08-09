// Tests for src/rset.hxx / src/rset.cxx (class RSET - Search Result Set).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// result.hxx isn't self-contained yet (its own turn hasn't come up), so
// this file pre-includes its dependencies the same way rset.cxx itself
// does, in the same order.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"

#include <cstdio>
#include <unistd.h>

// Fills Out in place rather than returning RESULT by value: RESULT (like
// DF, FCT, ATTRLIST -- see BUG_CATALOG.md) declares operator= but no
// copy constructor, so returning one by value would need the compiler's
// deprecated implicit copy constructor.
static void MakeResult(RESULT* Out, const CHR* Key, DOUBLE Score) {
	Out->SetKey(STRING(Key));
	Out->SetScore(Score);
}

TEST_CASE("RSET starts empty", "[rset]") {
	RSET rset;
	REQUIRE(rset.GetTotalEntries() == 0);
}

TEST_CASE("RSET AddEntry/GetEntry round-trips in insertion order", "[rset]") {
	RSET rset;
	RESULT r1, r2;
	MakeResult(&r1, "one", 1.0);
	MakeResult(&r2, "two", 2.0);
	rset.AddEntry(r1);
	rset.AddEntry(r2);
	REQUIRE(rset.GetTotalEntries() == 2);

	RESULT out;
	STRING s;
	rset.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "one");
	rset.GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "two");
}

TEST_CASE("RSET GetEntry leaves its output untouched for an out-of-range index", "[rset]") {
	RSET rset;
	RESULT only;
	MakeResult(&only, "only", 1.0);
	rset.AddEntry(only);

	RESULT out;
	MakeResult(&out, "untouched", 0.0);
	rset.GetEntry(0, &out);
	STRING s;
	out.GetKey(&s);
	REQUIRE(s == "untouched");

	rset.GetEntry(99, &out);
	out.GetKey(&s);
	REQUIRE(s == "untouched");
}

TEST_CASE("RSET SetEntry overwrites an existing entry in place", "[rset]") {
	RSET rset;
	RESULT r1, r2;
	MakeResult(&r1, "one", 1.0);
	MakeResult(&r2, "two", 2.0);
	rset.AddEntry(r1);
	rset.AddEntry(r2);

	RESULT replacement;
	MakeResult(&replacement, "replaced", 5.0);
	rset.SetEntry(1, replacement);

	RESULT out;
	STRING s;
	rset.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "replaced");
	rset.GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "two");
}

TEST_CASE("RSET SetEntry with an out-of-range index is a no-op, not an out-of-bounds write", "[rset]") {
	// BUGFIX #7 coverage: SetEntry() had no bounds check at all, unlike
	// GetEntry() -- confirmed via a standalone repro that an
	// out-of-range index produced a wild-pointer heap-buffer-overflow.
	RSET rset;
	RESULT only;
	MakeResult(&only, "only", 1.0);
	rset.AddEntry(only);

	RESULT replacement;
	MakeResult(&replacement, "should-not-appear", 9.0);
	rset.SetEntry(0, replacement);
	rset.SetEntry(500, replacement);
	REQUIRE(rset.GetTotalEntries() == 1);

	RESULT out;
	STRING s;
	rset.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "only");
}

TEST_CASE("RSET AddEntry expands past the initial 100-entry capacity", "[rset]") {
	RSET rset;
	RESULT r;
	MakeResult(&r, "k", 0.0);
	for (int i = 0; i < 150; i++) {
		rset.AddEntry(r);
	}
	REQUIRE(rset.GetTotalEntries() == 150);
}

TEST_CASE("RSET copy constructor deep-copies entries independently of the source", "[rset]") {
	// BUGFIX #2 coverage: before the fix, RSET declared neither a copy
	// constructor nor operator=, so `RSET b = a;` used the compiler-
	// generated shallow copy of Table, and both copies' destructors then
	// deleted the same heap array -- confirmed double-free/use-after-free
	// under ASan. Passing under ASan (make tests-asan) is the point.
	RSET original;
	RESULT r1, r2, r3;
	MakeResult(&r1, "one", 1.0);
	MakeResult(&r2, "two", 2.0);
	MakeResult(&r3, "three", 3.0);
	original.AddEntry(r1);
	original.AddEntry(r2);

	RSET copy = original;
	REQUIRE(copy.GetTotalEntries() == 2);

	copy.AddEntry(r3);
	REQUIRE(copy.GetTotalEntries() == 3);
	REQUIRE(original.GetTotalEntries() == 2);
}

TEST_CASE("RSET operator= deep-copies entries independently of the source", "[rset]") {
	// BUGFIX #2 coverage, the assignment half: `b = a;` previously used
	// the same buggy compiler-generated shallow copy.
	RSET original;
	RESULT r1, placeholder;
	MakeResult(&r1, "one", 1.0);
	MakeResult(&placeholder, "placeholder", 0.0);
	original.AddEntry(r1);

	RSET copy;
	copy.AddEntry(placeholder);
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 1);

	RESULT out;
	STRING s;
	copy.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "one");
}

TEST_CASE("RSET operator= self-assignment is a no-op, not a self-destruct", "[rset]") {
	RSET rset;
	RESULT r;
	MakeResult(&r, "one", 1.0);
	rset.AddEntry(r);
	rset = rset;
	REQUIRE(rset.GetTotalEntries() == 1);
}

TEST_CASE("RSET SortByKey orders entries ascending by key", "[rset]") {
	// BUGFIX #6 coverage: before the fix, the comparator returned
	// STRING::operator==()'s boolean result (never negative), which
	// qsort can't sort with -- confirmed unordered output pre-fix.
	RSET rset;
	RESULT r1, r2, r3;
	MakeResult(&r1, "charlie", 0.0);
	MakeResult(&r2, "alpha", 0.0);
	MakeResult(&r3, "bravo", 0.0);
	rset.AddEntry(r1);
	rset.AddEntry(r2);
	rset.AddEntry(r3);
	rset.SortByKey();
	REQUIRE(rset.GetTotalEntries() == 3);

	RESULT out;
	STRING s;
	rset.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "alpha");
	rset.GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "bravo");
	rset.GetEntry(3, &out);
	out.GetKey(&s);
	REQUIRE(s == "charlie");
}

TEST_CASE("RSET SortByScore orders entries by score descending", "[rset]") {
	// BUGFIX #5 coverage: before the fix, the comparator sorted ascending
	// (lowest score first), the opposite of IRSET::SortByScore's own
	// convention for the same conceptual operation.
	RSET rset;
	RESULT r1, r2, r3;
	MakeResult(&r1, "low", 1.0);
	MakeResult(&r2, "high", 9.0);
	MakeResult(&r3, "mid", 5.0);
	rset.AddEntry(r1);
	rset.AddEntry(r2);
	rset.AddEntry(r3);
	rset.SortByScore();

	RESULT out;
	STRING s;
	rset.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "high");
	rset.GetEntry(3, &out);
	out.GetKey(&s);
	REQUIRE(s == "low");
}

TEST_CASE("RSET CleanUp shrinks capacity without losing entries", "[rset]") {
	RSET rset;
	RESULT r1, r2;
	MakeResult(&r1, "a", 0.0);
	MakeResult(&r2, "b", 0.0);
	rset.AddEntry(r1);
	rset.AddEntry(r2);
	rset.CleanUp();
	REQUIRE(rset.GetTotalEntries() == 2);
}

TEST_CASE("RSET GetScaledScore scales linearly between the score range", "[rset]") {
	RSET rset;
	rset.SetScoreRange(10.0, 0.0);
	REQUIRE(rset.GetScaledScore(5.0, 100) == 50);
	REQUIRE(rset.GetScaledScore(0.0, 100) == 0);
	REQUIRE(rset.GetScaledScore(10.0, 100) == 100);
}

TEST_CASE("RSET SaveTable/LoadTable round-trips every field through a file", "[rset]") {
	// BUGFIX #4 coverage: before the fix, SaveTable/LoadTable raw-
	// fwrite/fread'd Table's in-memory bytes, including each RESULT's
	// STRING fields' internal Buffer pointers -- restoring stale pointer
	// values on load. Confirmed real with a standalone repro: save a
	// populated RSET, load it into a fresh RSET (forcing other heap
	// activity in between), then read an entry's Key -- aborted under
	// ASan with heap-use-after-free in STRING::Copy()'s memcpy. This test
	// exercises the same save/load/read sequence; passing under
	// make tests-asan is the point.
	char tmpl[] = "/tmp/isearch2_test_rset_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd >= 0);
	close(fd);
	STRING path(tmpl);

	{
		RSET original;
		RESULT r1;
		r1.SetKey(STRING("key1"));
		r1.SetDocumentType(STRING("html"));
		r1.SetPathName(STRING("/some/path/"));
		r1.SetFileName(STRING("file1.txt"));
		r1.SetRecordStart(100u);
		r1.SetRecordEnd(200u);
		r1.SetScore(0.75);
		r1.SetDbNum(3);
		original.AddEntry(r1);
		original.SaveTable(path);
	}

	RSET restored;
	restored.LoadTable(path);
	REQUIRE(restored.GetTotalEntries() == 1);

	RESULT out;
	STRING s;
	restored.GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "key1");
	out.GetDocumentType(&s);
	REQUIRE(s == "html");
	out.GetPathName(&s);
	REQUIRE(s == "/some/path/");
	out.GetFileName(&s);
	REQUIRE(s == "file1.txt");
	REQUIRE(out.GetRecordStart() == 100u);
	REQUIRE(out.GetRecordEnd() == 200u);
	REQUIRE(out.GetScore() == Catch::Approx(0.75));
	REQUIRE(out.GetDbNum() == 3);

	remove(tmpl);
}
