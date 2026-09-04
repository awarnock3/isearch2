// Tests for doctype/simple.hxx / doctype/simple.cxx (class SIMPLE).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// SIMPLE's constructor reads a "LINES" doctype option via
// Db->GetDocTypeOptions(), so TESTIDBOBJ here takes the desired LINES
// value at construction time and returns it as a "LINES=<n>" STRLIST
// entry, matching the "KEY=VALUE" format STRLIST::GetValue() parses
// (see src/strlist.cxx). Present()'s "B" element set calls
// RESULT::GetRecordData(), which (unlike RECORD::ParseFields()) opens
// PathName+FileName off disk unconditionally, so -- same technique as
// tests/doctype/test_markdown.cxx -- these tests build a real,
// file-backed RESULT rather than leaving Present() untested.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
#include "idbobj.hxx"
#include "doctype.hxx"
#include "simple.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	explicit TESTIDBOBJ(INT Lines = 1) : LinesOption(Lines) {}

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }

	void GetDocTypeOptions(STRLIST* StringListBuffer) const override {
		STRING entry = "LINES=";
		STRING n(LinesOption);
		entry.Cat(n);
		StringListBuffer->AddEntry(entry);
	}

private:
	INT LinesOption;
};

struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_simple_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents.data(), Contents.size());
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
	}

	~TempFile() {
		STRING Path(Dir);
		Path.Cat(Base);
		remove(Path);
	}
};

RESULT MakeResult(const TempFile& File, size_t ContentLength) {
	RESULT r;
	r.SetPathName(File.Dir);
	r.SetFileName(File.Base);
	r.SetRecordStart(0);
	r.SetRecordEnd((GPTYPE)(ContentLength - 1));  // inclusive end
	return r;
}

}  // namespace

TEST_CASE("SIMPLE::Present \"B\" returns the first line when LINES defaults to 1", "[simple]") {
	std::string content = "First line here\nSecond line here\n";
	TempFile file(content);
	TESTIDBOBJ db(1);
	SIMPLE dt(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	dt.Present(r, "B", &out);
	REQUIRE(STRING("First line here") == out);
}

TEST_CASE("SIMPLE::Present \"B\" skips leading whitespace before the first line", "[simple]") {
	std::string content = "   Indented headline\nBody text\n";
	TempFile file(content);
	TESTIDBOBJ db(1);
	SIMPLE dt(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	dt.Present(r, "B", &out);
	REQUIRE(STRING("Indented headline") == out);
}

TEST_CASE("SIMPLE::Present \"B\" includes the second line when LINES=2", "[simple]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypesimplecxx): the inner
	// while loop scanning each line stops with its position still on
	// the '\n' it just found, and without skipping past it before the
	// next outer-loop iteration, every line after the first was
	// silently dropped -- for LINES=2 this would have come back as
	// just "First line here" instead of both lines concatenated.
	std::string content = "First line here\nSecond line here\nThird line here\n";
	TempFile file(content);
	TESTIDBOBJ db(2);
	SIMPLE dt(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	dt.Present(r, "B", &out);
	REQUIRE(STRING("First line hereSecond line here") == out);
}

TEST_CASE("SIMPLE::Present \"B\" does not crash when LINES exceeds the number of real lines", "[simple]") {
	std::string content = "Only one line\n";
	TempFile file(content);
	TESTIDBOBJ db(5);
	SIMPLE dt(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	dt.Present(r, "B", &out);  // Must not crash.
	REQUIRE(STRING("Only one line") == out);
}

TEST_CASE("SIMPLE::Present defers non-\"B\" element sets to DOCTYPE::Present", "[simple]") {
	std::string content = "Some text\n";
	TempFile file(content);
	TESTIDBOBJ db(1);
	SIMPLE dt(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	dt.Present(r, "X", &out);  // Must not crash.
	SUCCEED();
}
