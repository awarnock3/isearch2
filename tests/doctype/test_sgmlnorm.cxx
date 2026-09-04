// Tests for doctype/sgmlnorm.hxx / doctype/sgmlnorm.cxx (class SGMLNORM).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_OBJS in
// the top-level Makefile, not reimplemented here.
//
// SGMLNORM's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_doctype.cxx works
// here too. parse_tags()/find_end_tag()/UnifiedName() are plain buffer
// parsers with no IDBOBJ dependency, so most of this file exercises them
// directly; ParseFields() additionally needs a real file on disk (it
// reads the record's bytes back via fopen/fread), so that one test uses
// a temp file the same way tests/src/test_filemap.cxx uses a temp MDT.

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
#include "sgmlnorm.hxx"

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

// Owns a temp file holding Contents and cleans it up. SGMLNORM::
// ParseFields() reads the record's bytes straight off disk via
// NewRecord->GetFullFileName(), so a real file is unavoidable here.
// Dir/Base are kept separate (not just a combined Path) because
// RECORD::SetFileName() runs RemovePath() on whatever it's given --
// passing a full "/tmp/xxx" path as the file name would silently strip
// the directory back off, so the directory has to go through
// SetPathName() instead.
struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_sgmlnorm_XXXXXX";
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

TEST_CASE("SGMLNORM::UnifiedName passes tag names through unchanged", "[sgmlnorm]") {
	TESTIDBOBJ db;
	SGMLNORM dt(&db);
	REQUIRE(STRING(dt.UnifiedName("title")) == "title");
	REQUIRE(STRING(dt.UnifiedName("")) == "");
}

TEST_CASE("SGMLNORM::parse_tags extracts tag names from a simple buffer", "[sgmlnorm]") {
	TESTIDBOBJ db;
	SGMLNORM dt(&db);
	char buf[] = "<a>x</a>";
	PCHR* tags = dt.parse_tags(buf, (GPTYPE)strlen(buf));
	REQUIRE(tags != nullptr);
	REQUIRE(STRING(tags[0]) == "a");
	REQUIRE(STRING(tags[1]) == "/a");
	REQUIRE(tags[2] == nullptr);
	delete [] tags;
}

TEST_CASE("SGMLNORM::parse_tags returns nullptr on an unterminated tag", "[sgmlnorm]") {
	// No closing '>' -- parse_tags should report a parse error, not read
	// past the buffer looking for one.
	TESTIDBOBJ db;
	SGMLNORM dt(&db);
	char buf[] = "<title";
	PCHR* tags = dt.parse_tags(buf, (GPTYPE)strlen(buf));
	REQUIRE(tags == nullptr);
}

TEST_CASE("SGMLNORM::find_end_tag finds a matching end tag, case-insensitively", "[sgmlnorm]") {
	TESTIDBOBJ db;
	SGMLNORM dt(&db);
	char buf[] = "<TITLE>x</title>";
	PCHR* tags = dt.parse_tags(buf, (GPTYPE)strlen(buf));
	REQUIRE(tags != nullptr);
	const CHR* end = dt.find_end_tag(tags, tags[0]);
	REQUIRE(end != nullptr);
	REQUIRE(STRING(end) == "/title");
	delete [] tags;
}

TEST_CASE("SGMLNORM::find_end_tag returns nullptr when no end tag matches", "[sgmlnorm]") {
	TESTIDBOBJ db;
	SGMLNORM dt(&db);
	char buf[] = "<a>x<b>y</b>";
	PCHR* tags = dt.parse_tags(buf, (GPTYPE)strlen(buf));
	REQUIRE(tags != nullptr);
	// tags[0] is "a"; there's no "/a" anywhere in the list, only "/b".
	const CHR* end = dt.find_end_tag(tags, tags[0]);
	REQUIRE(end == nullptr);
	delete [] tags;
}

TEST_CASE("SGMLNORM::ParseFields builds one DF per tag pair, trimming whitespace", "[sgmlnorm]") {
	TempFile file("<title> Hello World </title><author>Jane Doe</author>");
	TESTIDBOBJ db;
	SGMLNORM dt(&db);

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
	// DF::SetFieldName() uppercases internally (src/df.cxx) regardless
	// of what SGMLNORM passes in.
	REQUIRE(name1 == "TITLE");
	REQUIRE(name2 == "AUTHOR");

	// Leading/trailing whitespace inside <title> ... </title> should be
	// trimmed from the stored field coordinates.
	FCT fct1;
	df1.GetFct(&fct1);
	REQUIRE(fct1.GetTotalEntries() == 1);
	FC fc1;
	fct1.GetEntry(1, &fc1);
	const char wholeFile[] = "<title> Hello World </title><author>Jane Doe</author>";
	std::string value1(wholeFile + fc1.GetFieldStart(),
			    fc1.GetFieldEnd() - fc1.GetFieldStart() + 1);
	REQUIRE(value1 == "Hello World");
}
