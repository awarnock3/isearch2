// Tests for doctype/fgdc.hxx / doctype/fgdc.cxx (class FGDC). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// FGDC's constructor just forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too, extended with a GetDocTypeOptions() override (IDBOBJ's default
// is a no-op) so LoadFieldTable() can be pointed at a real FIELDTYPE
// file without falling into its interactive stdin-prompt fallback.
//
// FGDC is the third sibling in the CIPC/CIPP/FGDC family (see
// doctype/cipc.hxx for the shared architecture description); most of
// these tests mirror tests/doctype/test_cipc.cxx's, adjusted for
// FGDC's BEGDATE/ENDDATE tag spelling (vs. CIPC's StartDate/EndDate).

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
#include "sgmlnorm.hxx"
#include "date.hxx"
#include "fgdc.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_fgdc_XXXXXX";
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

TEST_CASE("FGDC header defines", "[fgdc]") {
	REQUIRE(FGDC_ACCEPT_EMPTY_TAGS == 0);
	REQUIRE(MAXNESTINGLEN == 1024);
}

TEST_CASE("FGDC extension constant", "[fgdc]") {
	const char *ext = FGDC_SGML_EXTENSION;
	REQUIRE(ext != nullptr);
	REQUIRE(strcmp(ext, "sgml") == 0);
}

TEST_CASE("MD_Element operations", "[fgdc]") {
	MD_Element elem;
	STRING tag = "TEST";
	elem.set_tag(tag);
	REQUIRE(elem.get_tag() == tag);
}

TEST_CASE("FGDC::ParseDate parses a well-formed BEGDATE/ENDDATE interval", "[fgdc]") {
	// FGDC's needles (<BEGDATE>, <ENDDATE>) are already spelled
	// uppercase in the source, so unlike doctype/cipc.cxx's BUGFIX #1,
	// there's no case-mismatch half to this bug here -- just confirms
	// the interval actually parses.
	TESTIDBOBJ db;
	FGDC fgdc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	fgdc.ParseDate("<BEGDATE>2020</BEGDATE><ENDDATE>2021</ENDDATE>", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(2020.0));
	REQUIRE(fEnd == Catch::Approx(2021.0));
}

TEST_CASE("FGDC::ParseDate sets both outputs to DATE_ERROR when BEGDATE has no closing tag", "[fgdc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdccxx): this branch used
	// to fall through instead of returning, leaving *fEnd unset on this
	// path. Same bug as, and fixed the same way as, doctype/cipc.cxx's
	// BUGFIX #1's missing-return half.
	TESTIDBOBJ db;
	FGDC fgdc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	fgdc.ParseDate("<BEGDATE>2020", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("FGDC::ParseDateRange parses a well-formed BEGDATE/ENDDATE interval", "[fgdc]") {
	// A bare year gets promoted to a full day boundary
	// (PromoteToDayStart()/PromoteToDayEnd()): YYYY -> YYYY0101 /
	// YYYY1231, same as doctype/cipc.cxx's ParseDateRange().
	TESTIDBOBJ db;
	FGDC fgdc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	fgdc.ParseDateRange("<BEGDATE>2020</BEGDATE><ENDDATE>2021</ENDDATE>", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(20200101.0));
	REQUIRE(fEnd == Catch::Approx(20211231.0));
}

TEST_CASE("FGDC::ParseDateRange sets both outputs to DATE_ERROR when BEGDATE has no closing tag", "[fgdc]") {
	// BUGFIX #2: same shape and same fix as ParseDate's BUGFIX #1 above.
	TESTIDBOBJ db;
	FGDC fgdc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	fgdc.ParseDateRange("<BEGDATE>2020", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("FGDC::ParseFields does not crash on a stray unmatched closing tag", "[fgdc]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypefgdccxx): Nested.Top() was
	// called with no GetSize()!=0 guard; a closing tag with nothing on
	// the stack (the very first tag here) made it dereference a null
	// PMD_Element. Same shape as doctype/cipc.cxx's BUGFIX #3 (see that
	// entry for the confirmed SEGV repro).
	TempFile file("</foo>");
	TESTIDBOBJ db;
	FGDC fgdc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	fgdc.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("FGDC::ParseFields does not leak on overlapping (non-LIFO) tags", "[fgdc]") {
	// BUGFIX #4 (docs/BUG_CATALOG.md#doctypefgdccxx): an MD_Element is
	// only Pop()'d+delete'd when a closing tag matches the *top* of
	// Nested. "<A><B></A></B>" pairs A with a real </A> and B with a
	// real </B> (find_end_tag() matches both), but they close in the
	// wrong order relative to each other, so </A> (processed while B is
	// still on top) never matches and leaves A's element stuck on
	// Nested forever. Verified leak-free under make tests-asan.
	TempFile file("<A><B></A></B>");
	TESTIDBOBJ db;
	FGDC fgdc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	fgdc.ParseFields(&record);  // Must not leak.
	SUCCEED();
}

TEST_CASE("FGDC::LoadFieldTable does not crash on an empty FIELDTYPE file", "[fgdc]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypefgdccxx): IsFile() only
	// checks existence, not content; an empty FIELDTYPE file made
	// strtok() return nullptr on the very first call, and the original
	// do-while unconditionally ran `Field_and_Type = pBuf;` (a null
	// pointer) before ever checking it -- STRING::operator=(const CHR*)
	// calls strlen() on it unconditionally.
	TempFile file("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	FGDC fgdc(&db);

	fgdc.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("FGDC::LoadFieldTable loads real entries without crashing", "[fgdc]") {
	TempFile file("62 TEXT\n12 NUMERIC\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	FGDC fgdc(&db);

	fgdc.LoadFieldTable();
	SUCCEED();
}
