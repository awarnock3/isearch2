// Tests for doctype/maildigest.hxx / doctype/maildigest.cxx (class
// MAILDIGEST). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// MAILDIGEST is a byte-for-byte structural duplicate of its sibling
// doctype/listdigest.cxx (same MAILFOLDER base, same ParseRecords()-only
// override shape), differing only in the magic separator string
// ("----...----", 30 dashes, vs. LISTDIGEST's "====...====", 40 equals
// signs). This test mirrors tests/doctype/test_listdigest.cxx's
// structure and reasoning throughout.

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
#include "maildigest.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_maildigest_XXXXXX";
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

TEST_CASE("MAILDIGEST::ParseRecords splits at a magic separator line", "[maildigest]") {
	std::string line1 = "Subject: message one\n";
	std::string line2 = "body one\n";
	std::string magic  = "------------------------------\n";
	std::string line4 = "Subject: message two\n";
	std::string line5 = "body two\n";

	std::string content = line1 + line2 + magic + line4 + line5;
	TempFile file(content);
	TESTIDBOBJ db;
	MAILDIGEST md(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	md.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	GPTYPE splitAt = line1.size() + line2.size() + magic.size();
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == splitAt - 1);
	REQUIRE(db.Records[1].first == splitAt);
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("MAILDIGEST::ParseRecords does not add a bogus record when a magic separator is the first line", "[maildigest]") {
	// Confirms (empirically, not just by structural analogy to
	// doctype/listdigest.cxx) that the mid-loop RecordEnd = SavePosition
	// - 1 site cannot underflow here either: Position is always
	// incremented by the *current* (matching) line's length, at least
	// magic_len+1 (31) bytes, before SavePosition is ever captured.
	std::string magic = "------------------------------\n";
	std::string line2 = "Subject: only message\n";
	std::string line3 = "body\n";

	std::string content = magic + line2 + line3;
	TempFile file(content);
	TESTIDBOBJ db;
	MAILDIGEST md(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	md.ParseRecords(fileRecord);

	for (auto& r : db.Records) {
		REQUIRE(r.second < content.size());
	}
}

TEST_CASE("MAILDIGEST::ParseRecords does not add a bogus record for an empty file", "[maildigest]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypemaildigestcxx): GPTYPE is
	// unsigned (UINT4); Position stays 0 for a genuinely empty file
	// (fgets() never succeeds even once), and `Position - 1`
	// underflowed to UINT_MAX, adding a record ending ~4 billion bytes
	// past the real (zero-byte) file.
	TempFile file("");
	TESTIDBOBJ db;
	MAILDIGEST md(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	md.ParseRecords(fileRecord);  // Must not crash.

	REQUIRE(db.Records.empty());
}
