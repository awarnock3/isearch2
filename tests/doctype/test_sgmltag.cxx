// Tests for doctype/sgmltag.hxx / doctype/sgmltag.cxx (class SGMLTAG).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// SGMLTAG's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_doctype.cxx and
// tests/doctype/test_sgmlnorm.cxx works here too. sgml_parse_tags()/
// find_end_tag() are plain buffer parsers with no IDBOBJ dependency;
// ParseFields() additionally reads the record's bytes back off disk, so
// that test uses a temp file the same way test_sgmlnorm.cxx does --
// including going through RECORD::SetPathName()/SetFileName() (not a
// single combined path) since SetFileName() strips any directory
// component via RemovePath().

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
#include "sgmltag.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_sgmltag_XXXXXX";
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

}  // namespace

TEST_CASE("SGMLTAG::sgml_parse_tags extracts tag names and counts useful fields", "[sgmltag]") {
	TESTIDBOBJ db;
	SGMLTAG dt(&db);
	char buf[] = "<a>x</a>";
	int numtags = -1;
	char** tags = dt.sgml_parse_tags(buf, (int)strlen(buf), &numtags);
	REQUIRE(tags != nullptr);
	REQUIRE(STRING(tags[0]) == "a");
	REQUIRE(STRING(tags[1]) == "/a");
	REQUIRE(tags[2] == nullptr);
	// Default DOCTYPE::UsefulSearchField() always returns GDT_TRUE, so
	// both the "a" and "/a" tags count.
	REQUIRE(numtags == 2);
	delete [] tags;
}

TEST_CASE("SGMLTAG::find_end_tag finds a matching close tag, case-insensitively", "[sgmltag]") {
	TESTIDBOBJ db;
	SGMLTAG dt(&db);
	char buf[] = "<TITLE>x</title>";
	int numtags = 0;
	char** tags = dt.sgml_parse_tags(buf, (int)strlen(buf), &numtags);
	REQUIRE(tags != nullptr);
	char* end = dt.find_end_tag(tags, tags[0]);
	REQUIRE(end != nullptr);
	REQUIRE(STRING(end) == "/title");
	delete [] tags;
}

TEST_CASE("SGMLTAG::find_end_tag requires an exact tag-text match, unlike SGMLNORM", "[sgmltag]") {
	// SGMLTAG's contract (see the class doc comment in sgmltag.hxx) is
	// stricter than SGMLNORM's: only tags whose open/close text matches
	// exactly (besides the leading '/') count as a pair. "b rel=x" has
	// no matching "/b rel=x" here, only a plain "/b".
	TESTIDBOBJ db;
	SGMLTAG dt(&db);
	char buf[] = "<b rel=x>y</b>";
	int numtags = 0;
	char** tags = dt.sgml_parse_tags(buf, (int)strlen(buf), &numtags);
	REQUIRE(tags != nullptr);
	char* end = dt.find_end_tag(tags, tags[0]);
	REQUIRE(end == nullptr);
	delete [] tags;
}

TEST_CASE("SGMLTAG::ParseFields builds one DF per exact tag pair", "[sgmltag]") {
	TempFile file("<title>Hello World</title><author>Jane Doe</author>");
	TESTIDBOBJ db;
	SGMLTAG dt(&db);

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

	FCT fct1;
	df1.GetFct(&fct1);
	REQUIRE(fct1.GetTotalEntries() == 1);
	FC fc1;
	fct1.GetEntry(1, &fc1);
	const char wholeFile[] = "<title>Hello World</title><author>Jane Doe</author>";
	std::string value1(wholeFile + fc1.GetFieldStart(),
			    fc1.GetFieldEnd() - fc1.GetFieldStart() + 1);
	REQUIRE(value1 == "Hello World");
}

TEST_CASE("SGMLTAG::ParseFields does not leak the file-name buffer on a missing file", "[sgmltag]") {
	// Regression test for BUGFIX #1 (docs/BUG_CATALOG.md#doctypesgmltagcxx):
	// `file` used to leak on every return path, including this one (file
	// open failure). Run under `make tests-asan` (LeakSanitizer, part of
	// ASan) is what actually catches a regression here -- this test just
	// needs to hit that path.
	TESTIDBOBJ db;
	SGMLTAG dt(&db);
	RECORD record;
	record.SetPathName(STRING("/tmp/"));
	record.SetFileName(STRING("isearch2_test_sgmltag_does_not_exist"));
	record.SetRecordStart(0);
	record.SetRecordEnd(0);
	dt.ParseFields(&record);  // Should return cleanly, not leak or crash.
	SUCCEED();
}
