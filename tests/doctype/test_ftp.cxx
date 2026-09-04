// Tests for doctype/ftp.hxx / doctype/ftp.cxx (class FTP). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// FTP's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too. Present() calls RESULT::GetRecordData(), which only crashes
// when fopen() fails on an empty/invalid path (see
// tests/doctype/test_filename.cxx's notes on that) -- pointing a real
// SetPathName()/SetFileName()/SetRecordStart()/SetRecordEnd()-populated
// RESULT at a real file avoids that path entirely.

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
#include "ftp.hxx"

#include <cstdio>
#include <cstring>
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
	STRING Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_ftp_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

RESULT MakeResult(const TempFile& file, const char* Contents) {
	RESULT r;
	r.SetPathName(file.Dir);
	r.SetFileName(file.Base);
	r.SetRecordStart(0);
	r.SetRecordEnd((GPTYPE)strlen(Contents));
	return r;
}

}  // namespace

TEST_CASE("FTP::Present element set B returns exactly the first line, excluding the newline", "[ftp]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeftpcxx): EraseAfter(firstNL)
	// kept the '\n' itself (STRING's Search()/EraseAfter() are 1-based
	// and EraseAfter(N) keeps N characters inclusive), so the "headline"
	// always included a trailing newline. Confirmed here via an
	// exact-string assertion (not just "doesn't crash").
	const char* content = "Headline text\nBody line one\nBody line two\n";
	TempFile file(content);
	TESTIDBOBJ db;
	FTP ftp(&db);

	RESULT r = MakeResult(file, content);
	STRING out;
	ftp.Present(r, "B", &out);
	REQUIRE(out == "Headline text");
}

TEST_CASE("FTP::Present element set F returns everything after the first line", "[ftp]") {
	const char* content = "Headline text\nBody line one\nBody line two\n";
	TempFile file(content);
	TESTIDBOBJ db;
	FTP ftp(&db);

	RESULT r = MakeResult(file, content);
	STRING out;
	ftp.Present(r, "F", &out);
	REQUIRE(out == "Body line one\nBody line two\n");
}

TEST_CASE("FTP::Present does not crash when there is no newline in the file", "[ftp]") {
	const char* content = "No newline anywhere in this file";
	TempFile file(content);
	TESTIDBOBJ db;
	FTP ftp(&db);

	RESULT r = MakeResult(file, content);
	STRING out;
	ftp.Present(r, "B", &out);  // Must not crash.
	SUCCEED();
}
