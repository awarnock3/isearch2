// Tests for src/result.hxx / src/result.cxx (class RESULT - one search
// hit).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// result.hxx isn't self-contained yet (matches the note in
// tests/src/test_rset.cxx), so this file pre-includes its dependencies
// the same way rset.cxx itself does, in the same order.

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
#include "result.hxx"

#include <cstdio>
#include <unistd.h>

namespace {

// Owns a real, on-disk MDT (mirrors tests/src/test_filemap.cxx's
// TempMdt) purely so SetMdt()/GetMdt() have a real object to point at.
struct TempMdt {
	STRING Stem;
	MDT* Mdt;

	TempMdt() {
		char tmpl[] = "/tmp/isearch2_test_result_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		remove(tmpl);
		Stem = tmpl;
		Mdt = new MDT(Stem, GDT_FALSE);
	}

	~TempMdt() {
		delete Mdt;
		STRING Fn;
		Fn = Stem; Fn.Cat(".mdt"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdg"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdk"); remove(Fn);
	}
};

}  // namespace

TEST_CASE("RESULT default-constructs with zeroed fields", "[result]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcresultcxx): DbNum,
	// RecordStart, RecordEnd, Score, and MyMdt were all left
	// indeterminate by the constructor.
	RESULT r;
	REQUIRE(r.GetDbNum() == 0);
	REQUIRE(r.GetRecordStart() == 0u);
	REQUIRE(r.GetRecordEnd() == 0u);
	REQUIRE(r.GetScore() == 0.0);
	REQUIRE(r.GetMdt() == nullptr);
	STRING s;
	r.GetKey(&s);
	REQUIRE(s == "");
}

TEST_CASE("RESULT SetKey/GetKey round-trip", "[result]") {
	RESULT r;
	r.SetKey(STRING("doc42"));
	STRING s;
	r.GetKey(&s);
	REQUIRE(s == "doc42");
}

TEST_CASE("RESULT GetVKey has no prefix when DbNum is 0", "[result]") {
	RESULT r;
	r.SetKey(STRING("doc42"));
	STRING s;
	r.GetVKey(&s);
	REQUIRE(s == "doc42");
}

TEST_CASE("RESULT GetVKey prefixes with \"DbNum:\" when DbNum is set", "[result]") {
	RESULT r;
	r.SetKey(STRING("doc42"));
	r.SetDbNum(3);
	STRING s;
	r.GetVKey(&s);
	REQUIRE(s == "3:doc42");
}

TEST_CASE("RESULT SetDocumentType/GetDocumentType round-trip", "[result]") {
	RESULT r;
	r.SetDocumentType(STRING("TEXT"));
	STRING s;
	r.GetDocumentType(&s);
	REQUIRE(s == "TEXT");
}

TEST_CASE("RESULT GetFullFileName concatenates PathName and FileName", "[result]") {
	RESULT r;
	r.SetPathName(STRING("/data/docs/"));
	r.SetFileName(STRING("report.txt"));
	STRING s;
	r.GetFullFileName(&s);
	REQUIRE(s == "/data/docs/report.txt");
}

TEST_CASE("RESULT RecordStart/RecordEnd and GetRecordSize", "[result]") {
	RESULT r;
	r.SetRecordStart(100u);
	r.SetRecordEnd(149u);
	REQUIRE(r.GetRecordStart() == 100u);
	REQUIRE(r.GetRecordEnd() == 149u);
	REQUIRE(r.GetRecordSize() == 50);
}

TEST_CASE("RESULT SetScore/GetScore round-trip", "[result]") {
	RESULT r;
	r.SetScore(0.875);
	REQUIRE(r.GetScore() == 0.875);
}

TEST_CASE("RESULT SetMdt/GetMdt round-trip", "[result]") {
	TempMdt tmp;
	RESULT r;
	r.SetMdt(*tmp.Mdt);
	REQUIRE(r.GetMdt() == tmp.Mdt);
}

TEST_CASE("RESULT operator= copies every field including MyMdt", "[result]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcresultcxx): MyMdt was never
	// copied by operator=, so a copy's GetMdt() kept whatever it already
	// had instead of the source's.
	TempMdt tmp;
	RESULT a;
	a.SetKey(STRING("a-key"));
	a.SetScore(0.5);
	a.SetMdt(*tmp.Mdt);

	RESULT b;
	b = a;

	STRING s;
	b.GetKey(&s);
	REQUIRE(s == "a-key");
	REQUIRE(b.GetScore() == 0.5);
	REQUIRE(b.GetMdt() == tmp.Mdt);
}

TEST_CASE("RESULT operator= self-assignment leaves fields intact", "[result]") {
	TempMdt tmp;
	RESULT r;
	r.SetKey(STRING("keep"));
	r.SetMdt(*tmp.Mdt);
	r = r;
	STRING s;
	r.GetKey(&s);
	REQUIRE(s == "keep");
	REQUIRE(r.GetMdt() == tmp.Mdt);
}

TEST_CASE("RESULT GetRecordData reads the byte range out of the real file", "[result]") {
	// Also exercises BUGFIX #3 (see docs/BUG_CATALOG.md#srcresultcxx):
	// GetRecordData used to narrow GPTYPE record offsets to INT locally
	// instead of reusing GetRecordSize()'s already-correct LONG.
	char tmpl[] = "/tmp/isearch2_test_result_data_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	FILE* fp = fdopen(fd, "w");
	REQUIRE(fp != nullptr);
	fprintf(fp, "0123456789ABCDEF");
	fclose(fp);

	RESULT r;
	r.SetPathName(STRING(""));
	r.SetFileName(STRING(tmpl));
	r.SetRecordStart(3u);
	r.SetRecordEnd(7u);

	STRING out;
	r.GetRecordData(&out);
	remove(tmpl);

	REQUIRE(out == "34567");
}

TEST_CASE("RESULT GetHighlightedRecord is a no-op without DO_HIGHLIGHTING", "[result]") {
	// DO_HIGHLIGHTING isn't defined anywhere in this build; confirms the
	// #else branch (added to silence -Wunused-parameter) doesn't crash
	// and leaves the buffer untouched.
	RESULT r;
	STRING out("untouched");
	r.GetHighlightedRecord(STRING("<b>"), STRING("</b>"), &out);
	REQUIRE(out == "untouched");
}
