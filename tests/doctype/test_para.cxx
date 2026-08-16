// Tests for doctype/para.hxx / doctype/para.cxx (class PARA). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// PARA's constructor forwards to DOCTYPE(DbParent), so the same
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
#include "para.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_para_XXXXXX";
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

TEST_CASE("PARA::ParseRecords splits two paragraphs at a blank line", "[para]") {
	std::string para1 = "First paragraph, some text here.";
	std::string sep = "\n\n";
	std::string para2 = "Second paragraph, more text.";
	std::string content = para1 + sep + para2;
	TempFile file(content);
	TESTIDBOBJ db;
	PARA dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	REQUIRE(db.Records[0].first == 0);
	// Like doctype/oneline.cxx's documented "RecordEnd lands on the
	// newline itself" convention, a paragraph's record here includes
	// its trailing blank-line separator, not just the paragraph's own
	// text -- RecordEnd lands on the *last* burned-through newline.
	REQUIRE(db.Records[0].second == para1.size() + sep.size() - 1);
	REQUIRE(db.Records[1].first == para1.size() + sep.size());
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("PARA::ParseRecords does not add a bogus empty record for a file starting with a blank line", "[para]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeparacxx): i and Start are
	// both GPTYPE (unsigned); when the "\n\n" marker was found at the
	// very start of the file (i == 0), `i - 1` underflowed to UINT_MAX,
	// always greater than Start (0), wrongly passing a check whose
	// whole purpose is to reject a degenerate paragraph this close to
	// Start -- adding a bogus, empty record spanning just the two
	// leading newlines, *in addition to* a second record for the real
	// text after it. With the guard working correctly, the leading
	// blank line is never treated as a paragraph boundary at all (Start
	// only advances on an accepted split), so it's simply absorbed into
	// the one real paragraph that follows -- a single record spanning
	// the whole file, not two.
	std::string content = "\n\nReal paragraph text.";
	TempFile file(content);
	TESTIDBOBJ db;
	PARA dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == content.size() - 1);
}

TEST_CASE("PARA::ParseRecords indexes a single-paragraph file with no blank line as one record", "[para]") {
	std::string content = "Just one paragraph, no blank line anywhere.";
	TempFile file(content);
	TESTIDBOBJ db;
	PARA dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 1);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == content.size() - 1);
}

TEST_CASE("PARA::ParseRecords does not add a bogus record for an empty file", "[para]") {
	TempFile file("");
	TESTIDBOBJ db;
	PARA dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);  // Must not crash.

	REQUIRE(db.Records.empty());
}
