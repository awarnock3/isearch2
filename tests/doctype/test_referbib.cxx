// Tests for doctype/referbib.hxx / doctype/referbib.cxx (class
// REFERBIB). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not reimplemented
// here.
//
// REFERBIB's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_colondoc.cxx
// works here too, extended with a DocTypeAddRecord() override
// (IDBOBJ's default is a no-op) so ParseRecords()'s record-splitting
// can be observed.

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
#include "referbib.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	std::vector<std::pair<GPTYPE, GPTYPE>> Records;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }

	void DocTypeAddRecord(const RECORD& NewRecord) override {
		Records.push_back({NewRecord.GetRecordStart(), NewRecord.GetRecordEnd()});
	}
};

struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_referbib_XXXXXX";
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

TEST_CASE("REFERBIB::UnifiedName maps a single-letter tag to its full field name", "[referbib]") {
	TESTIDBOBJ db;
	REFERBIB dt(&db);
	REQUIRE(STRING(dt.UnifiedName("%A")) == "author");
	REQUIRE(STRING(dt.UnifiedName("%T")) == "title");
}

TEST_CASE("REFERBIB::UnifiedName returns nullptr for a reserved-but-ignored tag", "[referbib]") {
	TESTIDBOBJ db;
	REFERBIB dt(&db);
	REQUIRE(dt.UnifiedName("%F") == nullptr);
}

TEST_CASE("REFERBIB::ParseRecords indexes a single reference with no internal blank line", "[referbib]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypereferbibcxx): the post-loop
	// flush checked `SavePosition == 0`, not `Position == 0`, even
	// though Position is what's actually decremented right below.
	// SavePosition is only assigned inside the loop when a blank-line
	// boundary is found -- for a file with exactly one reference (no
	// internal blank line at all), SavePosition never gets touched, so
	// this wrongly took the RecordEnd = 0 branch and the record failed
	// the RecordEnd > Start guard, silently skipping it entirely.
	std::string content = "%A Jane Doe\n%T A Sample Title\n";
	TempFile file(content);
	TESTIDBOBJ db;
	REFERBIB dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == content.size() - 1);
}

TEST_CASE("REFERBIB::ParseRecords splits two references at a blank line", "[referbib]") {
	std::string ref1 = "%A Jane Doe\n%T First Title\n";
	std::string sep = "\n";
	std::string ref2 = "%A John Smith\n%T Second Title\n";
	std::string content = ref1 + sep + ref2;
	TempFile file(content);
	TESTIDBOBJ db;
	REFERBIB dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("REFERBIB::ParseFields extracts a tagged field, including the last byte of the file", "[referbib]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypereferbibcxx): the "whole
	// file as one record" fallback used to compute RecEnd = ftell(fp) -
	// 1, dropping the file's last byte. This content deliberately has
	// no trailing newline, so the last field's last character ('e' of
	// "Doe") IS the last byte of the file.
	std::string content = "%A Jane Doe";
	TempFile file(content);
	TESTIDBOBJ db;
	REFERBIB dt(&db);

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

TEST_CASE("REFERBIB::ParseFields does not crash on an empty file", "[referbib]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypereferbibcxx): parse_tags()'s
	// scan loop used to be `for (GPTYPE i = 0; i < len - 4; i++)`; len
	// is unsigned (GPTYPE/UINT4), so for a record under 4 bytes -- an
	// empty file, reached via BUGFIX #2's fallback once it stops
	// truncating first -- `len - 4` wrapped to just under UINT_MAX, and
	// the loop body's b[i+1]/b[i+2] reads ran past RecBuffer's real
	// allocation on its very first iteration.
	TempFile file("");
	TESTIDBOBJ db;
	REFERBIB dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("REFERBIB::ParseFields extracts a recognized tag under its unified field name", "[referbib]") {
	std::string content = "%Q Some Corp\n";
	TempFile file(content);
	TESTIDBOBJ db;
	REFERBIB dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF df;
	REQUIRE(FindField(dft, "CORP_AUTHOR", &df));
}

TEST_CASE("REFERBIB::ParseFields throws an unrecognized (lowercase) tag into Misc", "[referbib]") {
	// UnifiedName()'s table is indexed by uppercase letter only; a
	// lowercase tag like "%q" is well-formed (parse_tags() itself
	// doesn't care about case) but falls outside 'A'..'Z', so
	// UnifiedName() returns nullptr and WANT_MISC's fallback applies.
	std::string content = "%q Some Value\n";
	TempFile file(content);
	TESTIDBOBJ db;
	REFERBIB dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF df;
	REQUIRE(FindField(dft, "MISC", &df));
}
