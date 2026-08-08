// Tests for doctype/oneline.hxx / doctype/oneline.cxx (class ONELINE).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// ONELINE's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_colondoc.cxx
// works here too, extended with a DocTypeAddRecord() override
// (IDBOBJ's default is a no-op) so ParseRecords()'s record-splitting
// can actually be observed.

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
#include "oneline.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_oneline_XXXXXX";
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

}  // namespace

TEST_CASE("ONELINE::ParseRecords indexes each newline-terminated line as its own record", "[oneline]") {
	std::string line1 = "Alice Smith, 555-1234\n";
	std::string line2 = "Bob Jones, 555-5678\n";
	std::string content = line1 + line2;
	TempFile file(content);
	TESTIDBOBJ db;
	ONELINE dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == line1.size() - 1);
	REQUIRE(db.Records[1].first == line1.size());
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("ONELINE::ParseRecords indexes a final line with no trailing newline", "[oneline]") {
	std::string line1 = "Alice Smith, 555-1234\n";
	std::string line2 = "Bob Jones, 555-5678";  // no trailing '\n'
	std::string content = line1 + line2;
	TempFile file(content);
	TESTIDBOBJ db;
	ONELINE dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	REQUIRE(db.Records[1].first == line1.size());
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("ONELINE::ParseRecords does not add a bogus record for an empty file", "[oneline]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeonelinecxx): GPTYPE is
	// unsigned (UINT4); the scan loop's phantom Position increment on
	// the EOF-triggering read used to make `Start != Position` wrongly
	// true even for a 0-byte file, and RecordEnd = Position - 2
	// underflowed to UINT_MAX, adding a bogus record ending ~4 billion
	// bytes past the real (empty) file.
	TempFile file("");
	TESTIDBOBJ db;
	ONELINE dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);  // Must not crash.

	REQUIRE(db.Records.empty());
}

TEST_CASE("ONELINE::ParseRecords does not add a spurious trailing record when the file ends in a newline", "[oneline]") {
	// Same BUGFIX #1 underflow, but the far more commonly-hit trigger:
	// any real text file whose very last byte is '\n' (the normal case)
	// used to get one extra, invalid record appended after the real
	// last line, with RecordEnd < RecordStart.
	std::string content = "Only line\n";
	TempFile file(content);
	TESTIDBOBJ db;
	ONELINE dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == content.size() - 1);
}

TEST_CASE("ONELINE::ParseRecords indexes consecutive blank lines as their own minimal records", "[oneline]") {
	std::string content = "First\n\nThird\n";
	TempFile file(content);
	TESTIDBOBJ db;
	ONELINE dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 3);
}
