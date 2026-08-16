// Tests for doctype/dif.hxx / doctype/dif.cxx (class DIF).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// DIF's constructor forwards to COLONDOC(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too, extended with a GetDocTypeOptions() override (IDBOBJ's
// default is a no-op) so LoadFieldTable() can be pointed at a real
// FIELDTYPE file without falling into its "assume all fields are
// text" early return.

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
#include "colondoc.hxx"
#include "date.hxx"
#include "dif.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	STRING FieldTypeFilename;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
	void GetDocTypeOptions(STRLIST* StringListBuffer) const override {
		if (!FieldTypeFilename.Equals("")) {
			STRING opt("FIELDTYPE=");
			opt.Cat(FieldTypeFilename);
			StringListBuffer->AddEntry(opt);
		}
	}
};

struct TempFile {
	STRING Dir;
	STRING Base;
	STRING Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_dif_XXXXXX";
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

TEST_CASE("DIF extension constants", "[dif]") {
	const char *sgml = DIF_SGML_EXTENSION;
	const char *html = DIF_HTML_EXTENSION;
	const char *text = DIF_TEXT_EXTENSION;

	REQUIRE(sgml != nullptr);
	REQUIRE(html != nullptr);
	REQUIRE(text != nullptr);

	REQUIRE(strcmp(sgml, "sgm") == 0);
	REQUIRE(strcmp(html, "htm") == 0);
	REQUIRE(strcmp(text, "sut") == 0);
}

TEST_CASE("DIF::ParseFields extracts a simple field", "[dif]") {
	const char* content = "Entry_ID: X123\n";
	TempFile file(content);
	TESTIDBOBJ db;
	DIF dif(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dif.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 1);

	DF df;
	dft.GetEntry(1, &df);
	STRING name;
	df.GetFieldName(&name);
	REQUIRE(name == "ENTRY_ID");
}

TEST_CASE("DIF::ParseFields does not crash on an empty record", "[dif]") {
	TempFile file("");
	TESTIDBOBJ db;
	DIF dif(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dif.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("DIF::ParseFields does not overflow on an unclosed Group", "[dif]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypedifcxx): sgetc() had no
	// bounds check at all; group() unconditionally calls nextToken()
	// once more after groupbody() returns, even when groupbody() (via
	// nested atomtail()/textML() calls) already ran the scanner off the
	// end of the buffer looking for a "End_Group" that was never
	// there. Confirmed a real heap-buffer-overflow with this exact
	// input before fixing.
	const char* content = "Group: Test\nEntry_ID: X123\n";
	TempFile file(content);
	TESTIDBOBJ db;
	DIF dif(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dif.ParseFields(&record);  // Must not crash or read out of bounds.
	SUCCEED();
}

TEST_CASE("DIF::ParseFields parses a well-formed Group", "[dif]") {
	const char* content = "Group: Test\nEntry_ID: X123\nEnd_Group\n";
	TempFile file(content);
	TESTIDBOBJ db;
	DIF dif(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dif.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("DIF::LoadFieldTable does not crash on an empty FIELDTYPE file", "[dif]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypedifcxx): an empty (but
	// existing) FIELDTYPE file made strtok() return nullptr on the very
	// first call, and the original do-while unconditionally ran
	// `Field_and_Type = pBuf;` (a null pointer) before ever checking it.
	TempFile file("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	DIF dif(&db);

	dif.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("DIF::LoadFieldTable loads real entries without crashing", "[dif]") {
	TempFile file("62 TEXT\n12 NUMERIC\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	DIF dif(&db);

	dif.LoadFieldTable();
	SUCCEED();
}

TEST_CASE("DIF::ParseDate parses simple values", "[dif]") {
	TESTIDBOBJ db;
	DIF dif(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;

	dif.ParseDate("present", &fStart, &fEnd);
	REQUIRE(fStart == DATE_PRESENT);
	REQUIRE(fEnd == DATE_PRESENT);

	dif.ParseDate("unknown", &fStart, &fEnd);
	REQUIRE(fStart == DATE_UNKNOWN);
	REQUIRE(fEnd == DATE_UNKNOWN);

	dif.ParseDate("2020", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(2020.0));
	REQUIRE(fEnd == Catch::Approx(2020.0));

	dif.ParseDate("not-a-date", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("DIF::ParseDateRange parses a well-formed range", "[dif]") {
	// A bare-year value gets promoted to a full day boundary
	// (PromoteToDayStart()/PromoteToDayEnd()): YYYY -> YYYY0101/YYYY1231.
	TESTIDBOBJ db;
	DIF dif(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	dif.ParseDateRange("START_DATE: 2020\nSTOP_DATE: 2021\n", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(20200101.0));
	REQUIRE(fEnd == Catch::Approx(20211231.0));
}

TEST_CASE("DIF::ParseDateRange assigns DATE_PRESENT when STOP_DATE is absent", "[dif]") {
	TESTIDBOBJ db;
	DIF dif(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	dif.ParseDateRange("START_DATE: 2020\n", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(20200101.0));
	REQUIRE(fEnd == DATE_PRESENT);
}
