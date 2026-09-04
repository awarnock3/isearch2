// Tests for doctype/filename.hxx / doctype/filename.cxx (class
// FILENAME). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// FILENAME's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too, extended with a DocTypeAddRecord() override (IDBOBJ's
// default is a no-op) so ParseRecords()'s ".fn"-sibling-file indexing
// can actually be observed.
//
// Unlike most other doctype/ tests, FILENAME::Present()'s "B" element
// set is exercised directly here (not skipped as "crashes on a
// default-constructed RESULT"): RESULT::GetRecordData() only crashes
// when fopen() fails on an empty/invalid path, and RESULT exposes real
// SetPathName()/SetFileName()/SetRecordStart()/SetRecordEnd() setters,
// so pointing it at a real file avoids that path entirely.

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
#include "filename.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	bool AddRecordCalled = false;
	STRING AddedFileName;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
	void DocTypeAddRecord(const RECORD& NewRecord) override {
		AddRecordCalled = true;
		NewRecord.GetFileName(&AddedFileName);
	}
};

struct TempFile {
	STRING Dir;
	STRING Base;
	STRING Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_filename_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

}  // namespace

TEST_CASE("FILENAME::ParseRecords writes a sibling .fn file and indexes it", "[filename]") {
	TempFile file("this content is irrelevant -- only the filename matters");
	TESTIDBOBJ db;
	FILENAME fn(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetDocumentType("FILENAME");

	fn.ParseRecords(record);

	REQUIRE(db.AddRecordCalled);
	std::string addedName(db.AddedFileName);
	std::string expectedSuffix = ".fn";
	REQUIRE(addedName.size() >= expectedSuffix.size());
	REQUIRE(addedName.compare(addedName.size() - expectedSuffix.size(),
				   expectedSuffix.size(), expectedSuffix) == 0);

	// The ".fn" sibling file itself should now exist, containing the
	// original filename text.
	STRING FnPath = file.Path;
	FnPath.Cat(".fn");
	REQUIRE(IsFile(FnPath));
	remove(FnPath);
}

TEST_CASE("FILENAME::Present element set B returns the indexed filename text", "[filename]") {
	// The ".fn" file's content is whatever ParseRecords() wrote into it
	// (the original filename) -- simulate that directly rather than
	// going through ParseRecords() again.
	TempFile fnFile("/tmp/some_original_name");
	TESTIDBOBJ db;
	FILENAME fn(&db);

	RESULT r;
	r.SetPathName("");
	r.SetFileName(fnFile.Path);
	r.SetRecordStart(0);
	r.SetRecordEnd((GPTYPE)strlen("/tmp/some_original_name"));

	STRING out;
	fn.Present(r, "B", &out);
	REQUIRE(out == "/tmp/some_original_name");
}

TEST_CASE("FILENAME::Present with a non-B element set returns the original file's contents", "[filename]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefilenamecxx): RecBuffer used
	// to leak on every successful call here -- confirmed leak-free under
	// make tests-asan. Present() strips the ".fn" suffix off the
	// RESULT's filename and reads back the *original* file, so point
	// the RESULT at "<real file>.fn" without that file needing to
	// actually exist.
	TempFile realFile("the real original file contents");
	TESTIDBOBJ db;
	FILENAME fn(&db);

	RESULT r;
	r.SetPathName("");
	STRING FakeFnName = realFile.Path;
	FakeFnName.Cat(".fn");
	r.SetFileName(FakeFnName);

	STRING out;
	fn.Present(r, "F", &out);
	REQUIRE(out == "the real original file contents");
}

TEST_CASE("FILENAME::Present does not crash when the target file is missing", "[filename]") {
	TESTIDBOBJ db;
	FILENAME fn(&db);

	RESULT r;
	r.SetPathName("/tmp/");
	r.SetFileName("isearch2_test_filename_does_not_exist.fn");

	STRING out;
	fn.Present(r, "F", &out);  // Must not crash.
	SUCCEED();
}
