// Tests for doctype/colondoc.hxx / doctype/colondoc.cxx (class COLONDOC).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// COLONDOC's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too. ParseFields() reads the record's bytes back off disk, so
// these tests use a temp file the same way test_sgmlnorm.cxx/
// test_sgmltag.cxx do -- including going through
// RECORD::SetPathName()/SetFileName() (not a single combined path)
// since SetFileName() strips any directory component via RemovePath().

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
#include "colondoc.hxx"

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

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_colondoc_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
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

std::string ExtractField(const char* FileContents, FC& fc) {
	return std::string(FileContents + fc.GetFieldStart(),
			    fc.GetFieldEnd() - fc.GetFieldStart() + 1);
}

}  // namespace

TEST_CASE("COLONDOC::UnifiedName passes tag names through unchanged", "[colondoc]") {
	TESTIDBOBJ db;
	COLONDOC dt(&db);
	REQUIRE(STRING(dt.UnifiedName("Title")) == "Title");
}

TEST_CASE("COLONDOC::ParseFields extracts full field values, including the last byte of the file", "[colondoc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypecolondoccxx): the file used
	// to be read as `ftell(fp) - 1` bytes, silently dropping the last
	// byte. This content deliberately has no trailing newline, so the
	// last field's last character ('e' of "Doe") IS the last byte of
	// the file -- if BUGFIX #1 regressed, this field would come back as
	// "Jane Do" instead of "Jane Doe".
	//
	// BUGFIX #2/#4 (docs/BUG_CATALOG.md#doctypecolondoccxx): the
	// trailing-whitespace trim used to check one byte past the value's
	// real end (chopping "World" to "Worl"), and SetFieldEnd() used to
	// store one byte too many (masking BUGFIX #2 in the common case --
	// see the BUG_CATALOG entry for why fixing only one of the two
	// would have made things worse, not better). This test's extracted
	// substrings, read back via FC::GetFieldStart()/GetFieldEnd()
	// exactly like the real engine does for retrieval, catch either one
	// regressing on its own.
	const char* content = "Title: Hello World\nAuthor: Jane Doe";
	TempFile file(content);
	TESTIDBOBJ db;
	COLONDOC dt(&db);

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
	REQUIRE(name1 == "TITLE");
	REQUIRE(name2 == "AUTHOR");

	FCT fct1, fct2;
	df1.GetFct(&fct1);
	df2.GetFct(&fct2);
	REQUIRE(fct1.GetTotalEntries() == 1);
	REQUIRE(fct2.GetTotalEntries() == 1);
	FC fc1, fc2;
	fct1.GetEntry(1, &fc1);
	fct2.GetEntry(1, &fc2);

	REQUIRE(ExtractField(content, fc1) == "Hello World");
	REQUIRE(ExtractField(content, fc2) == "Jane Doe");
}

TEST_CASE("COLONDOC::ParseFields trims leading whitespace after the colon", "[colondoc]") {
	const char* content = "Title:    Padded Value\n";
	TempFile file(content);
	TESTIDBOBJ db;
	COLONDOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 1);
	DF df1;
	dft.GetEntry(1, &df1);
	FCT fct1;
	df1.GetFct(&fct1);
	FC fc1;
	fct1.GetEntry(1, &fc1);
	REQUIRE(ExtractField(content, fc1) == "Padded Value");
}
