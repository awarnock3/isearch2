// Tests for doctype/usmarc.hxx / doctype/usmarc.cxx (class USMARC).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// USMARC's constructor forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by other doctype/ tests works here
// too, extended with a DocTypeAddRecord() override so
// ParseRecords()'s record-splitting can be observed.
//
// A minimal, hand-built, well-formed MARC record is used throughout:
// a 24-byte leader (record length "00053", base address "00037"), a
// single directory entry for field "245" (length 15, offset 0), the
// directory terminator (0x1e), the field's own bytes ("  " indicators,
// 0x1f + 'a' + "Test Title", 0x1e field terminator), and the record
// terminator (0x1d). Byte offsets were verified with a standalone
// script before being hard-coded here (see conversation notes) --
// tagPos=41, tagLength=10, so the subfield 'a' content occupies bytes
// [41, 50] inclusive ("Test Title").

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
#include "usmarc.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_usmarc_XXXXXX";
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

// One well-formed record: leader + one "245" directory entry (a title
// field, subfield 'a' = "Test Title") + field data. See the file
// header comment for the exact byte layout.
std::string MakeWellFormedRecord() {
	std::string leader = "00053" + std::string("0000000") + "00037" + std::string("0000000");
	std::string directory = "245" + std::string("0015") + std::string("00000");
	std::string dirterm(1, (char)0x1e);
	std::string fielddata = "  " + std::string(1, (char)0x1f) + "a" + "Test Title" + std::string(1, (char)0x1e);
	std::string recterm(1, (char)0x1d);
	return leader + directory + dirterm + fielddata + recterm;
}

}  // namespace

TEST_CASE("USMARC::ParseRecords splits a batch file at each record's own length prefix", "[usmarc]") {
	std::string record1 = MakeWellFormedRecord();
	std::string record2 = MakeWellFormedRecord();
	std::string content = record1 + record2;
	TempFile file(content);
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD fileRecord;
	fileRecord.SetPathName(file.Dir);
	fileRecord.SetFileName(file.Base);

	dt.ParseRecords(fileRecord);

	REQUIRE(db.Records.size() == 2);
	REQUIRE(db.Records[0].first == 0);
	REQUIRE(db.Records[0].second == record1.size() - 1);
	REQUIRE(db.Records[1].first == record1.size());
	REQUIRE(db.Records[1].second == content.size() - 1);
}

TEST_CASE("USMARC::ParseFields indexes both the raw MARC tag and the friendly field name", "[usmarc]") {
	std::string content = MakeWellFormedRecord();
	TempFile file(content);
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF rawDf;
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(FindField(dft, "245", &rawDf));

	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));

	// BUGFIX #7 (docs/BUG_CATALOG.md#doctypeusmarccxx): the
	// subfield-specific addSearchEntry() call used to store an
	// exclusive end (tagPos + tagLength), one byte past the real
	// content -- this field's stored span would have included the
	// 0x1e field-terminator byte right after "Test Title".
	FCT fct;
	titleDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Test Title");

	// RecBuffer/marcDir are globals ParseFields() left allocated for a
	// following ParseWords() call to free (see BUGFIX #3) -- clean up
	// explicitly here so this test doesn't leave a leak for whichever
	// test Catch2's randomized run order happens to execute last.
	GPTYPE gpBuffer[64];
	dt.ParseWords((CHR*)content.c_str(), (INT)content.size(), 0, gpBuffer, 64);
}

TEST_CASE("USMARC::ParseWords returns a sentinel instead of overflowing GpBuffer", "[usmarc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeusmarccxx): GpLength was
	// never checked at all before writing to GpBuffer[GpListSize++] --
	// confirmed unused via -Wunused-parameter before the fix. "Test
	// Title" has two words; GpLength=1 forces an overflow on the
	// second. This also exercises BUGFIX #2 (the IsStopWord bound
	// fix) along the way, since IsStopWord() runs for every candidate
	// word found here.
	std::string content = MakeWellFormedRecord();
	TempFile file(content);
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);
	dt.ParseFields(&record);  // Populates the global marcDir/RecBuffer.

	GPTYPE gpBuffer[1];
	GPTYPE result = dt.ParseWords((CHR*)content.c_str(), (INT)content.size(),
				       0, gpBuffer, 1);
	REQUIRE(result == (GPTYPE)-1);
}

TEST_CASE("USMARC::ParseFields does not leak RecBuffer/marcDir when called twice without an intervening ParseWords", "[usmarc]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypeusmarccxx): RecBuffer and
	// marcDir are globals, reassigned on every ParseFields() call
	// without freeing whatever they held from a previous call --
	// exactly the scenario a multi-record batch file creates, since
	// ParseFields() runs once per record while ParseWords() (which
	// frees them) may not run again until much later, for a different
	// record.
	std::string content1 = MakeWellFormedRecord();
	std::string content2 = MakeWellFormedRecord();
	TempFile file1(content1);
	TempFile file2(content2);
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD record1;
	record1.SetPathName(file1.Dir);
	record1.SetFileName(file1.Base);
	record1.SetRecordStart(0);
	record1.SetRecordEnd(0);
	dt.ParseFields(&record1);

	RECORD record2;
	record2.SetPathName(file2.Dir);
	record2.SetFileName(file2.Base);
	record2.SetRecordStart(0);
	record2.SetRecordEnd(0);
	dt.ParseFields(&record2);  // Must not leak the first call's buffers.

	// Clean up what the second call allocated so this test itself
	// doesn't leak (mirrors the normal ParseWords() cleanup step).
	GPTYPE gpBuffer[64];
	dt.ParseWords((CHR*)content2.c_str(), (INT)content2.size(), 0, gpBuffer, 64);

	SUCCEED();
}

TEST_CASE("USMARC::readFileContents (via ParseFields) does not crash or leak when the file can't be opened", "[usmarc]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypeusmarccxx): `file` (a
	// NewCString() copy of the path, used only for perror()) was never
	// freed anywhere -- leaking on every single call, not just this
	// error path.
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD record;
	record.SetPathName("/tmp/");
	record.SetFileName("isearch2_test_usmarc_does_not_exist");
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash or leak.
	SUCCEED();
}

TEST_CASE("USMARC::ParseFields does not overrun the buffer on a field missing its end-of-field delimiter", "[usmarc]") {
	// BUGFIX #6 (docs/BUG_CATALOG.md#doctypeusmarccxx): findNextTag()'s
	// scan loops used to have no bound but END_OF_FIELD/
	// START_OF_SUBFIELD -- for a malformed record where a subfield's
	// content never hits either of those bytes, the scan ran straight
	// past the record terminator (0x1d, not itself a checked stop
	// byte) and into whatever memory follows the buffer's allocation.
	// This corrupts the well-formed record by overwriting its 0x1e
	// field terminator (the last content byte before the record
	// terminator) with an ordinary character, so findNextTag()'s
	// second scan (computing the subfield's tagLength) never finds a
	// real delimiter and must fall back to RecBuffer's own null
	// terminator instead.
	std::string content = MakeWellFormedRecord();
	content[content.size() - 2] = 'X';  // overwrite the 0x1e field terminator
	TempFile file(content);
	TESTIDBOBJ db;
	USMARC dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);  // Must not crash (heap-buffer-overflow).

	// Clean up RecBuffer/marcDir explicitly (see the comment in the
	// "indexes both..." test above for why).
	GPTYPE gpBuffer[64];
	dt.ParseWords((CHR*)content.c_str(), (INT)content.size(), 0, gpBuffer, 64);
	SUCCEED();
}
