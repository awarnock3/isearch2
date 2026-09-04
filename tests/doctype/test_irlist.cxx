// Tests for doctype/irlist.hxx / doctype/irlist.cxx (class IRLIST).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// IRLIST's constructor forwards to MAILFOLDER(DbParent), so the same
// minimal TESTIDBOBJ shape used by tests/doctype/test_bibtex.cxx works
// here too, extended with a DocTypeAddRecord() override (IDBOBJ's
// default is a no-op) so ParseRecords()'s record-splitting can
// actually be observed.

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
#include "mailfolder.hxx"
#include "irlist.hxx"

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

// See tests/doctype/test_sgmlnorm.cxx's TempFile for why Dir/Base are
// kept separate rather than a single combined path.
struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_irlist_XXXXXX";
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

TEST_CASE("IRLIST::ParseRecords does not add a bogus record when the first line is a From line", "[irlist]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypeirlistcxx): GPTYPE is
	// unsigned (UINT4); when the triggering "From " line is the very
	// first line of the file, the original `SavePosition - 1`
	// underflowed to UINT_MAX, and `RecordEnd > Start` (0) then passed,
	// adding a record ending ~4 billion bytes past the real file.
	std::string content =
		"From alice@example.com Mon Jan 1 00:00:00 2024\n"
		"Subject: first message\n"
		"\n"
		"body text\n";
	TempFile file(content);
	TESTIDBOBJ db;
	IRLIST il(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	il.ParseRecords(fileRecord);

	// Every recorded end must stay within the real file -- nothing
	// anywhere close to underflowing to UINT_MAX.
	for (auto& r : db.Records) {
		REQUIRE(r.second < content.size());
	}
}

TEST_CASE("IRLIST::ParseRecords ignores an embedded From-line not preceded by a blank line", "[irlist]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeirlistcxx): IsMailFromLine()
	// matches any well-formed mbox envelope header, including one
	// quoted/forwarded inside another message's body -- common in
	// digests. Without the missing `Look` blank-line guard (mirroring
	// doctype/mailfolder.cxx's own ParseRecords()), that embedded line
	// would spuriously fragment the record.
	std::string line1 = "From alice@example.com Mon Jan 1 00:00:00 2024\n";
	std::string line2 = "Subject: digest message\n";
	std::string line3 = "\n";
	std::string line4 = "Quoted forwarded mail below:\n";
	// Embedded From-line, NOT preceded by a blank line -- must not split.
	std::string line5 = "From bob@example.com Tue Jan 2 00:00:00 2024\n";
	std::string line6 = "This text must stay part of message 1's record.\n";
	std::string line7 = "\n";
	// A real second message, correctly blank-line-preceded -- must split.
	std::string line8 = "From carol@example.com Wed Jan 3 00:00:00 2024\n";
	std::string line9 = "Second real message.\n";

	std::string content = line1 + line2 + line3 + line4 + line5 + line6 + line7 + line8 + line9;
	TempFile file(content);
	TESTIDBOBJ db;
	IRLIST il(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	il.ParseRecords(fileRecord);

	// Exactly one split (at line8), not two -- the embedded line5 must
	// not have created a spurious extra record.
	REQUIRE(db.Records.size() == 2);

	GPTYPE secondStart = line1.size() + line2.size() + line3.size() + line4.size()
			    + line5.size() + line6.size() + line7.size();
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == secondStart - 1);
	REQUIRE(db.Records[1].first == secondStart);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("IRLIST::ParseRecords splits at a magic separator line", "[irlist]") {
	std::string line1 = "From alice@example.com Mon Jan 1 00:00:00 2024\n";
	std::string line2 = "msg1 body\n";
	std::string magic  = "*********\n";
	std::string line4 = "From bob@example.com Tue Jan 2 00:00:00 2024\n";
	std::string line5 = "msg2 body\n";

	std::string content = line1 + line2 + magic + line4 + line5;
	TempFile file(content);
	TESTIDBOBJ db;
	IRLIST il(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	il.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	GPTYPE splitAt = line1.size() + line2.size() + magic.size();
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == splitAt - 1);
	REQUIRE(db.Records[1].first == splitAt);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("IRLIST::ParseRecords does not add a bogus record for an empty file", "[irlist]") {
	// BUGFIX #2 (continued): the same underflow guard applies to the
	// post-loop final-record logic -- Position stays 0 for a genuinely
	// empty file (fgets() never succeeds even once).
	TempFile file("");
	TESTIDBOBJ db;
	IRLIST il(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	il.ParseRecords(fileRecord);  // Must not crash.

	REQUIRE(db.Records.empty());
}
