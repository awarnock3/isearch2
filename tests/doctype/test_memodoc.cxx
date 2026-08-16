// Tests for doctype/memodoc.hxx / doctype/memodoc.cxx (class MEMODOC).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// MEMODOC's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_colondoc.cxx
// works here too. ParseFields() reads the record's bytes back off
// disk, so these tests use a temp file the same way test_colondoc.cxx
// does.

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
#include "memodoc.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_memodoc_XXXXXX";
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

std::string ExtractField(const std::string& FileContents, FC& fc) {
	return FileContents.substr(fc.GetFieldStart(),
				    fc.GetFieldEnd() - fc.GetFieldStart() + 1);
}

bool FindField(DFT& dft, const char* Name, DF* Out) {
	INT total = dft.GetTotalEntries();
	for (INT i = 1; i <= total; i++) {
		DF df;
		dft.GetEntry(i, &df);
		STRING fieldName;
		df.GetFieldName(&fieldName);
		if (fieldName == Name) {
			*Out = df;
			return true;
		}
	}
	return false;
}

}  // namespace

TEST_CASE("MEMODOC::ParseFields extracts a tagged field, including the last byte of the file", "[memodoc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypememodoccxx): the "whole file
	// as one record" fallback (RecordEnd left at 0) used to compute
	// RecEnd = ftell(fp) - 1, dropping the file's last byte. This
	// content deliberately has no trailing newline, so the last field's
	// last character ('e' of "Doe") IS the last byte of the file -- if
	// BUGFIX #1 regressed, this field would come back as "Jane Do".
	std::string content = "Author: Jane Doe";
	TempFile file(content);
	TESTIDBOBJ db;
	MEMODOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF authorDf;
	REQUIRE(FindField(dft, "AUTHOR", &authorDf));

	FCT fct;
	authorDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane Doe");
}

TEST_CASE("MEMODOC::ParseFields does not crash on an empty file", "[memodoc]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypememodoccxx): parse_tags()'s
	// scan loop used to be `for (GPTYPE i = 0; i < len - 4; i++)`; len
	// is unsigned (GPTYPE/UINT4), so for a record under 4 bytes -- an
	// empty file, reached via BUGFIX #1's fallback above once it stops
	// underflowing RecEnd first -- `len - 4` wrapped to just under
	// UINT_MAX, and the loop body's b[i+1]/b[i+2]/b[i+3] reads ran
	// straight past RecBuffer's real (1-byte) allocation on its very
	// first iteration: a real heap-buffer-overflow, confirmed via a
	// before/after test-revert under ASan.
	TempFile file("");
	TESTIDBOBJ db;
	MEMODOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("MEMODOC::ParseFields does not crash on a record under 4 bytes", "[memodoc]") {
	// Same BUGFIX #2 underflow, but via a short *non-empty* record
	// rather than a literally empty file -- a second, independent way
	// to reach len < 4 without going through the RecordEnd==0 fallback
	// at all (RecordStart/RecordEnd set directly, as ParseRecords()
	// would for a real short record).
	std::string content = "Hi";
	TempFile file(content);
	TESTIDBOBJ db;
	MEMODOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd((GPTYPE)(content.size() - 1));

	dt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("MEMODOC::ParseFields does not treat a run ending in a non-separator char as a section break", "[memodoc]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypememodoccxx): the 4-char
	// section-break scan's last disjunct used to check `b[i+2] == '-'`
	// instead of `b[i+3] == '-'`, a copy-paste typo. Whenever the 3rd
	// character of a would-be run was specifically '-', the whole
	// disjunct was satisfied regardless of the 4th character's real
	// value, so "__-X" (X being anything) wrongly matched as a section
	// break -- ending tag-hunting right there and silently dropping
	// every tag after it. This field appears right after such a run;
	// with the bug present, tag-hunting would have already stopped
	// (State == DONE) before ever reaching it. The trailing space after
	// "X" (rather than going straight to '\n') is deliberate: it lets
	// the non-matching run cleanly fall back to an ordinary (never
	// colon-terminated, so never confirmed) tag-name scan that resolves
	// via STARTED -> CONTINUING -> HUNTING before the next line, so
	// this test isolates BUGFIX #3 alone rather than also depending on
	// unrelated tag-across-newline behavior.
	std::string content = "Title: Memo\n__-X \nAuthor: Jane Doe";
	TempFile file(content);
	TESTIDBOBJ db;
	MEMODOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF authorDf;
	REQUIRE(FindField(dft, "AUTHOR", &authorDf));
}

TEST_CASE("MEMODOC::ParseFields treats a genuine 4-char run as a section break, starting Memo-Body", "[memodoc]") {
	std::string content = "Title: Memo\n----\nFree text body here";
	TempFile file(content);
	TESTIDBOBJ db;
	MEMODOC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf, bodyDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));
	REQUIRE(FindField(dft, "MEMO-BODY", &bodyDf));
}
