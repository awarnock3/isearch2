// Tests for doctype/medline.hxx / doctype/medline.cxx (class MEDLINE).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// MEDLINE's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too. ParseFields() reads the record's bytes back off disk, so those
// tests use a temp file the same way test_colondoc.cxx/
// test_mailfolder.cxx do.

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
#include "medline.hxx"

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

// See tests/doctype/test_sgmlnorm.cxx's TempFile for why Dir/Base are
// kept separate rather than a single combined path.
struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const char* Contents, size_t Len) {
		char tmpl[] = "/tmp/isearch2_test_medline_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, Len);
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
	}
	explicit TempFile(const char* Contents) : TempFile(Contents, strlen(Contents)) {}

	~TempFile() {
		STRING Path(Dir);
		Path.Cat(Base);
		remove(Path);
	}
};

std::string ExtractField(const char* FileContents, FC& fc) {
	return std::string(FileContents + fc.GetFieldStart(),
			    fc.GetFieldEnd() - fc.GetFieldStart() + 1);
}

}  // namespace

TEST_CASE("MEDLINE::UnifiedName passes tag names through unchanged", "[medline]") {
	TESTIDBOBJ db;
	MEDLINE dt(&db);
	REQUIRE(STRING(dt.UnifiedName("AB")) == "AB");
}

TEST_CASE("MEDLINE::ParseFields extracts full field values, including the last byte of the file", "[medline]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypemedlinecxx): the file used to
	// be read as `ftell(fp) - 1` bytes, silently dropping the last byte.
	// This content deliberately has no trailing newline, so the last
	// field's last character is the last byte of the file.
	//
	// BUGFIX #1b/#2/#4: the trailing-newline exclusion, trailing-
	// whitespace trim, and SetFieldEnd() off-by-ones (same shape as
	// doctype/colondoc.cxx's bugs, see that entry). This test's
	// extracted substrings, read back via FC::GetFieldStart()/
	// GetFieldEnd() exactly like the real engine does for retrieval,
	// catch any of them regressing on its own.
	const char* content = "AB  -Hello World\nTI  -The Title";
	TempFile file(content);
	TESTIDBOBJ db;
	MEDLINE dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 2);

	DF df1, df2;
	dft.GetEntry(1, &df1);
	dft.GetEntry(2, &df2);
	STRING name1, name2;
	df1.GetFieldName(&name1);
	df2.GetFieldName(&name2);
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(name1 == "AB");
	REQUIRE(name2 == "TI");

	FCT fct1, fct2;
	df1.GetFct(&fct1);
	df2.GetFct(&fct2);
	FC fc1, fc2;
	fct1.GetEntry(1, &fc1);
	fct2.GetEntry(1, &fc2);

	REQUIRE(ExtractField(content, fc1) == "Hello World");
	REQUIRE(ExtractField(content, fc2) == "The Title");
}

TEST_CASE("MEDLINE::ParseFields does not overflow on a record too short to hold a tag", "[medline]") {
	// BUGFIX #7 (docs/BUG_CATALOG.md#doctypemedlinecxx): parse_tags()'s
	// `for (i = 0; i < len - 4; i++)` underflowed (len is unsigned) for
	// any record under 4 bytes, turning the scan into a read far past
	// the tiny allocated buffer. Confirmed a real heap-buffer-overflow
	// with a standalone repro (a 2-byte record) before fixing; make
	// tests-asan is what actually re-verifies this, same as the repro.
	TempFile file("ab", 2);
	TESTIDBOBJ db;
	MEDLINE dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 0);
}

TEST_CASE("MEDLINE::Present with the brief element set does not crash", "[medline]") {
	// Exercises UnifiedName("TI")/("SO")/("AU") (see BUGFIX #6: an
	// unconditional debug printf() used to fire on every one of these
	// calls, which would have corrupted a CGI frontend's HTTP response
	// -- see docs/BUG_CATALOG.md#doctypemedlinecxx). TESTIDBOBJ doesn't
	// override GetFieldData(), so the resulting headline is empty
	// rather than populated; this test is about Present() completing
	// cleanly, not the brief-headline content itself.
	TESTIDBOBJ db;
	MEDLINE dt(&db);
	RESULT result;
	STRING out;
	dt.Present(result, STRING("B"), &out);
	SUCCEED();
}
