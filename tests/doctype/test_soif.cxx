// Tests for doctype/soif.hxx / doctype/soif.cxx (class SOIF). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// SOIF's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_colondoc.cxx
// works here too. Unlike most other doctypes, ParseRecords() does not
// split a file into multiple records -- it always adds exactly one
// record spanning the whole file -- so its own test doesn't need a
// DocTypeAddRecord() override to observe splitting, just confirmation
// that a single, correctly-bounded record was added.

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
#include "soif.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_soif_XXXXXX";
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

TEST_CASE("SOIF::ParseRecords adds the whole file as a single record", "[soif]") {
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHello\n}\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == content.size());
}

TEST_CASE("SOIF::ParseFields extracts the @FILE url field", "[soif]") {
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHello\n}\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF urlDf;
	REQUIRE(FindField(dft, "URL", &urlDf));

	FCT fct;
	urlDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "http://example.com/page");
}

TEST_CASE("SOIF::ParseFields extracts a name{len}:<TAB>value attribute field", "[soif]") {
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHello\n}\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));

	FCT fct;
	titleDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Hello");
}

TEST_CASE("SOIF::ParseFields does not crash on a badly started @FILE record", "[soif]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypesoifcxx): this error path
	// (no '\n' anywhere after "@FILE { ") used to leak the `pdft`
	// (new DFT()) allocated just above the parsing loop -- freed
	// RecBuffer but never pdft. Exercising it here means a regression
	// would show up as a real leak under `make tests-asan`.
	std::string content = "@FILE { http://example.com/no-newline-here";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash or leak.
	SUCCEED();
}

TEST_CASE("SOIF::ParseFields does not crash on a badly ended record", "[soif]") {
	// Same BUGFIX #2 leak, second site: content that matches neither
	// the "@FILE { " prefix, the "name{len}:<TAB>" attribute pattern,
	// nor the closing "}\n" exactly.
	std::string content = "@FILE { http://example.com/page\ngarbage line with no tag\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash or leak.
	SUCCEED();
}

TEST_CASE("SOIF::ParseFields does not crash on an attribute value missing its trailing newline", "[soif]") {
	// Same BUGFIX #2 leak, third site: a well-formed "name{len}:<TAB>"
	// header whose declared length doesn't land on a following '\n'.
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHelloX";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash or leak.
	SUCCEED();
}

TEST_CASE("SOIF::ParseFields does not crash or leak when the file can't be opened", "[soif]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypesoifcxx): `file` (a
	// NewCString() copy of the path, used only for perror()) used to be
	// allocated unconditionally at the top of the function but never
	// freed anywhere -- leaking on every single call, not just this
	// error path. This is simply the easiest path to exercise directly.
	TESTIDBOBJ db;
	SOIF dt(&db);

	RECORD record;
	record.SetPathName("/tmp/");
	record.SetFileName("isearch2_test_soif_does_not_exist");
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash or leak.
	SUCCEED();
}

TEST_CASE("SOIF::Present \"F\" returns the whole record", "[soif]") {
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHello\n}\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RESULT r;
	r.SetPathName(file.Dir);
	r.SetFileName(file.Base);
	r.SetRecordStart(0);
	r.SetRecordEnd((GPTYPE)(content.size() - 1));

	STRING out;
	dt.Present(r, "F", &out);
	REQUIRE(STRING(content.c_str()) == out);
}

TEST_CASE("SOIF::Present a non-\"F\" element set with no fields returns empty", "[soif]") {
	std::string content = "@FILE { http://example.com/page\ntitle{5}:\tHello\n}\n";
	TempFile file(content);
	TESTIDBOBJ db;
	SOIF dt(&db);

	RESULT r;
	r.SetPathName(file.Dir);
	r.SetFileName(file.Base);
	r.SetRecordStart(0);
	r.SetRecordEnd((GPTYPE)(content.size() - 1));

	STRING out;
	dt.Present(r, "B", &out);  // Db (TESTIDBOBJ) reports 0 DfdtGetTotalEntries().
	REQUIRE(STRING("") == out);
}
