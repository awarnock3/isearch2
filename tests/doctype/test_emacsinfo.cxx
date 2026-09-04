// Tests for doctype/emacsinfo.hxx / doctype/emacsinfo.cxx (class
// EMACSINFO). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// EMACSINFO's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests
// works here too, extended with a DocTypeAddRecord() override
// (IDBOBJ's default is a no-op) so ParseRecords()'s record-splitting
// can actually be observed, matching test_bibtex.cxx's pattern.

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
#include "emacsinfo.hxx"

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

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_emacsinfo_XXXXXX";
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

TEST_CASE("EMACSINFO::ParseRecords splits a file into one record per File: marker", "[emacsinfo]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeemacsinfocxx) regression
	// test: ParseRecords() used to leak the whole file's buffer on
	// every successful call; make tests-asan's LeakSanitizer is what
	// actually verifies the fix.
	const std::string content = "File:one.txt,\nbody one\nFile:two.txt,\nbody two\n";
	TempFile file(content.c_str());
	TESTIDBOBJ db;
	EMACSINFO ei(&db);

	RECORD FileRecord;
	FileRecord.SetPathName(file.Dir);
	FileRecord.SetFileName(file.Base);

	ei.ParseRecords(FileRecord);

	REQUIRE(db.Records.size() == 2);
	GPTYPE secondFile = (GPTYPE)content.find("File:two.txt");
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == secondFile - 1);
	REQUIRE(db.Records[1].first == secondFile);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("EMACSINFO::ParseFields extracts File: and Node: field values", "[emacsinfo]") {
	// BUGFIX #3 regression test: val_end used to point at the comma
	// delimiter itself (SetFieldEnd() is inclusive), so the stored
	// field included a trailing comma.
	const char* content = "File:one.txt,  Node:Top,\nSome body text.\n";
	TempFile file(content);
	TESTIDBOBJ db;
	EMACSINFO ei(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	ei.ParseFields(&record);

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
	REQUIRE(name1 == "FILE");
	REQUIRE(name2 == "NODE");

	FCT fct1, fct2;
	df1.GetFct(&fct1);
	df2.GetFct(&fct2);
	FC fc1, fc2;
	fct1.GetEntry(1, &fc1);
	fct2.GetEntry(1, &fc2);

	REQUIRE(ExtractField(content, fc1) == "one.txt");
	REQUIRE(ExtractField(content, fc2) == "Top");
}

TEST_CASE("EMACSINFO::ParseFields adds no fields when File:/Node: are absent", "[emacsinfo]") {
	const char* content = "Just some plain text with no markers.\n";
	TempFile file(content);
	TESTIDBOBJ db;
	EMACSINFO ei(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	ei.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 0);
}

// Present() is not exercised directly: it calls
// ResultRecord.GetRecordData() unconditionally, before even looking at
// ElementSet, and that (RESULT, not EMACSINFO, code) crashes on a
// default-constructed RESULT for reasons unrelated to any
// EMACSINFO-specific logic -- the same issue noted for
// doctype/bibtex.cxx's Present(). Building a real, exercisable RESULT
// was judged out of scope for this file's own turn.
