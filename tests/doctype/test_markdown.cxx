// Tests for doctype/markdown.hxx / doctype/markdown.cxx (class
// MARKDOWN). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not reimplemented
// here.
//
// MARKDOWN only overrides Present() -- ParseRecords()/ParseFields()
// are inherited unchanged from DOCTYPE. Its "B" (brief) element set
// calls RESULT::GetRecordData(), which (unlike RECORD::ParseFields())
// opens PathName+FileName off disk unconditionally and has no "unset ->
// read nothing" fallback, so (unlike tests/doctype/test_emacsinfo.cxx's
// and test_bibtex.cxx's Present(), which were left untested for this
// exact reason) this test builds a real, file-backed RESULT via
// SetPathName()/SetFileName()/SetRecordStart()/SetRecordEnd() -- the
// only way to exercise this file's actual logic at all, since it all
// lives behind Present().

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
#include "markdown.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_markdown_XXXXXX";
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

TEST_CASE("MARKDOWN::Present \"B\" returns the first ATX heading, stripped of '#' and whitespace", "[markdown]") {
	std::string content = "  ##   My Title  \nSome body text\nMore text\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARKDOWN md(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	md.Present(r, "B", &out);
	REQUIRE(STRING("My Title") == out);
}

TEST_CASE("MARKDOWN::Present \"B\" falls back to the first non-empty line when there is no heading", "[markdown]") {
	std::string content = "\n\n   Hello World   \nMore text\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARKDOWN md(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	md.Present(r, "B", &out);
	REQUIRE(STRING("Hello World") == out);
}

TEST_CASE("MARKDOWN::Present \"B\" prefers a later heading over an earlier plain line", "[markdown]") {
	std::string content = "Intro line\n# Real Title\nBody\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARKDOWN md(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	md.Present(r, "B", &out);
	REQUIRE(STRING("Real Title") == out);
}

TEST_CASE("MARKDOWN::Present leaves the brief empty for a blank record", "[markdown]") {
	std::string content = "\n\n\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARKDOWN md(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	md.Present(r, "B", &out);
	REQUIRE(STRING("") == out);
}

TEST_CASE("MARKDOWN::Present defers non-\"B\" element sets to DOCTYPE::Present", "[markdown]") {
	std::string content = "# Title\nBody\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARKDOWN md(&db);
	RESULT r = MakeResult(file, content.size());

	STRING out;
	md.Present(r, "X", &out);  // Must not crash.
	SUCCEED();
}
