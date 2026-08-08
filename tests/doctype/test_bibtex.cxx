// Tests for doctype/bibtex.hxx / doctype/bibtex.cxx (class BIBTEX).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// BIBTEX's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too, extended with a DocTypeAddRecord() override (IDBOBJ's default is
// a no-op) so ParseRecords()'s record-splitting can actually be
// observed. ParseFields()/ParseRecords() both read bytes back off disk,
// so these tests use a temp file the same way test_colondoc.cxx/
// test_medline.cxx do -- including going through
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
#include "bibtex.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_bibtex_XXXXXX";
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

TEST_CASE("BIBTEX::ParseRecords splits a file into one record per closing brace", "[bibtex]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypebibtexcxx) regression test:
	// ParseRecords() used to leak the whole file's buffer on every
	// successful call; make tests-asan's LeakSanitizer is what actually
	// verifies the fix, same as elsewhere this session.
	const std::string content =
		"@article{k1,title=\"First\"}@article{k2,title=\"Second\"}";
	TempFile file(content.c_str());
	TESTIDBOBJ db;
	BIBTEX bt(&db);

	RECORD FileRecord;
	FileRecord.SetPathName(file.Dir);
	FileRecord.SetFileName(file.Base);

	bt.ParseRecords(FileRecord);

	REQUIRE(db.Records.size() == 2);
	GPTYPE firstBrace = (GPTYPE)content.find('}');
	// The first record ends exactly at its closing brace.
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == firstBrace);
	// The second (last) record starts right after the first ends, and
	// its end is extended to the last byte of the file.
	REQUIRE(db.Records[1].first == firstBrace + 1);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("BIBTEX::ParseFields extracts the title field", "[bibtex]") {
	const char* content = "@article{key, title = \"A Great Title\"}";
	TempFile file(content);
	TESTIDBOBJ db;
	BIBTEX bt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	bt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 1);

	DF df;
	dft.GetEntry(1, &df);
	STRING name;
	df.GetFieldName(&name);
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(name == "TITLE");

	FCT fct;
	df.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "A Great Title");
}

TEST_CASE("BIBTEX::ParseFields adds no field when the record has no title", "[bibtex]") {
	// BUGFIX #4 regression test: this used to unconditionally add a
	// bogus zero-length "title" field (FieldStart=0, FieldEnd=0) even
	// when "title" never appeared in the record at all.
	const char* content = "@article{key, author = \"Someone\"}";
	TempFile file(content);
	TESTIDBOBJ db;
	BIBTEX bt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	bt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 0);
}

TEST_CASE("BIBTEX::ParseFields does not crash or leak when the title has no opening quote", "[bibtex]") {
	// BUGFIX #3 regression test (the "Cannot find quote mark after
	// title" path): used to leak both RecBuffer and the DFT. No '"'
	// appears anywhere after "title" (or at all) in this content.
	const char* content = "@article{key, title is unquoted here}";
	TempFile file(content);
	TESTIDBOBJ db;
	BIBTEX bt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	bt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("BIBTEX::ParseFields does not crash or leak when the title has no closing quote", "[bibtex]") {
	// BUGFIX #3 regression test (the "couldn't find ending quote" path).
	const char* content = "@article{key, title = \"Unterminated";
	TempFile file(content);
	TESTIDBOBJ db;
	BIBTEX bt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	bt.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

// Present() itself is not exercised directly: the ElementSet=="F"
// path just delegates to ResultRecord.GetRecordData() (RESULT, not
// BIBTEX, code), which crashes on a default-constructed RESULT for
// reasons unrelated to any BIBTEX-specific logic; and the non-"F"
// path's very first check, Db->DfdtGetTotalEntries(), always returns 0
// via IDBOBJ's own default (TESTIDBOBJ doesn't override it), so it
// would always take the early "no fields" return regardless of what
// ParseFields() populated on the RECORD itself. Building a real,
// exercisable RESULT/IDBOBJ pair is out of scope for this doctype's
// own turn; ParseFields()'s field-extraction logic (what Present()
// would otherwise report) is already covered above.
