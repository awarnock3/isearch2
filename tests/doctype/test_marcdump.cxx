// Tests for doctype/marcdump.hxx / doctype/marcdump.cxx (class
// MARCDUMP). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not reimplemented
// here.
//
// MARCDUMP's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_colondoc.cxx
// works here too, extended with a DocTypeAddRecord() override so
// ParseRecords()'s record-splitting can be observed. ParseFields()
// reads the record's bytes back off disk, so these tests use a temp
// file the same way test_colondoc.cxx does.

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
#include "marcdump.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_marcdump_XXXXXX";
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
				    fc.GetFieldEnd() - fc.GetFieldStart());
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

TEST_CASE("MARCDUMP::UnifiedName passes tag names through unchanged", "[marcdump]") {
	TESTIDBOBJ db;
	MARCDUMP dt(&db);
	REQUIRE(STRING(dt.UnifiedName("245")) == "245");
}

TEST_CASE("MARCDUMP::ParseRecords indexes a file containing only a single record", "[marcdump]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): the loop only
	// ever closed off a record when it found the *next* record's "001"
	// tag, so a file with exactly one record -- the common case for a
	// standalone marcdump extract -- produced zero calls to
	// DocTypeAddRecord() before this fix.
	std::string content = "001 REC1\n245 A Title\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARCDUMP dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
}

TEST_CASE("MARCDUMP::ParseRecords indexes the last of several records, not just the earlier ones", "[marcdump]") {
	// Same BUGFIX #1 regression, but for a multi-record file: only the
	// final record lacks a following "001" to trigger its own flush, so
	// this specifically exercises the fix's post-loop flush rather than
	// the in-loop path already exercised for every earlier record.
	std::string content = "001 REC1\n245 Title One\n001 REC2\n245 Title Two\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MARCDUMP dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
}

TEST_CASE("MARCDUMP::ParseFields extracts tag/value pairs, including the last byte of the file", "[marcdump]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypemarcdumpcxx): the "whole file
	// as one record" fallback (RecordEnd left at 0) used to compute
	// RecEnd = ftell(fp) - 1, dropping the file's last byte. This
	// content deliberately has no trailing newline, so the last field's
	// last character ('e' of "Doe") IS the last byte of the file -- if
	// BUGFIX #2 regressed, this field would come back as "Jane Do".
	std::string content = "001 X\n700 Jane Doe";
	TempFile file(content);
	TESTIDBOBJ db;
	MARCDUMP dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF authorDf;
	REQUIRE(FindField(dft, "700", &authorDf));

	FCT fct;
	authorDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane Doe");
}

TEST_CASE("MARCDUMP::ParseFields also adds a human-readable alias field via marcdumpFieldNumToName", "[marcdump]") {
	std::string content = "001 X\n245 My Title";
	TempFile file(content);
	TESTIDBOBJ db;
	MARCDUMP dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF numericDf, aliasDf;
	REQUIRE(FindField(dft, "245", &numericDf));
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(FindField(dft, "TITLE", &aliasDf));
}

TEST_CASE("MARCDUMP::ParseFields does not crash on an empty file", "[marcdump]") {
	// BUGFIX #2's underflow specifically: GPTYPE is unsigned (UINT4);
	// for an empty file, ftell(fp) is 0, and the old `- 1` wrapped to
	// UINT_MAX, which then wrapped `RecLength + 1` back to 0 too,
	// leading to a 0-byte allocation and a 1-byte write past its end
	// (RecBuffer[ActualLength] = '\0') -- undefined behavior, though a
	// standalone repro found this build's allocator/ASan combination
	// doesn't actually flag it. This test just confirms the fixed code
	// still doesn't crash (it shouldn't have before either, empirically
	// -- see docs/BUG_CATALOG.md#doctypemarcdumpcxx for the full story).
	TempFile file("");
	TESTIDBOBJ db;
	MARCDUMP dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}
