// Tests for doctype/cipp.hxx / doctype/cipp.cxx (class CIPP).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here. CIPP is doctype/cipc.cxx's
// sibling (product metadata instead of collection metadata) and shares
// almost all of its structure and bugs -- see docs/BUG_CATALOG.md#doctypecippcxx
// and #doctypecipccxx for the shared reasoning. cipp.cxx calls a
// GetNumericValue() defined (for real, not commented out) in
// doctype/fgdc.cxx -- fgdc.cxx is linked in (already, from cipc.cxx's
// turn) purely to satisfy that symbol at link time.
//
// CIPP's constructor just forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too, extended with a GetDocTypeOptions() override (IDBOBJ's default
// is a no-op) so LoadFieldTable() can be pointed at a real FIELDTYPE
// file without falling into its interactive stdin-prompt fallback.

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
#include "cipp.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_cipp_XXXXXX";
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

TEST_CASE("CIPP header defines", "[cipp]") {
	REQUIRE(CIPP_ACCEPT_EMPTY_TAGS == 0);
	REQUIRE(MAXNESTINGLEN == 1024);
}

TEST_CASE("CIPP_Element operations", "[cipp]") {
	CIP_Element elem;
	STRING tag = "TEST";
	elem.set_tag(tag);
	REQUIRE(elem.get_tag() == tag);
}

TEST_CASE("CIPP::ParseDate parses a well-formed StartDate/EndDate interval", "[cipp]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypecippcxx): Hold was uppercased
	// before searching for the mixed-case needle "<StartDate>" (here,
	// left over from the now fully commented-out <CALDATE> block), so
	// this branch could never match at all. Uses ParseIsoDate() rather
	// than GetFloat(), and ParseIsoDate() expects dashed YYYY-MM-DD (a
	// bare "2020" parses to 0.0, not a useful non-error value), so this
	// asserts the parse succeeded (not DATE_ERROR) rather than an exact
	// value -- the reachability is what BUGFIX #1 is about.
	TESTIDBOBJ db;
	CIPP cipp(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipp.ParseDate("<StartDate>2020-01-01</StartDate><EndDate>2020-12-31</EndDate>", &fStart, &fEnd);
	REQUIRE(fStart != DATE_ERROR);
	REQUIRE(fEnd != DATE_ERROR);
}

TEST_CASE("CIPP::ParseDate sets *fStart to DATE_ERROR when StartDate has no closing tag", "[cipp]") {
	// Regression test for the missing-return half of BUGFIX #1.
	TESTIDBOBJ db;
	CIPP cipp(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipp.ParseDate("<StartDate>2020-01-01", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("CIPP::ParseDateRange parses a well-formed StartDate/EndDate interval", "[cipp]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypecippcxx): Hold was uppercased
	// before searching for the mixed-case needle "<StartDate>", so this
	// branch could never match at all -- confirmed here by asserting an
	// actual, successful interval parse (not just "doesn't crash"). A
	// bare-year value gets promoted to a full day boundary
	// (PromoteToDayStart()/PromoteToDayEnd()): YYYY -> YYYY0101/YYYY1231.
	TESTIDBOBJ db;
	CIPP cipp(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipp.ParseDateRange("<StartDate>2020</StartDate><EndDate>2021</EndDate>", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(20200101.0));
	REQUIRE(fEnd == Catch::Approx(20211231.0));
}

TEST_CASE("CIPP::ParseDateRange sets both outputs to DATE_ERROR when StartDate has no closing tag", "[cipp]") {
	// Regression test for the missing-return half of BUGFIX #2.
	TESTIDBOBJ db;
	CIPP cipp(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipp.ParseDateRange("<StartDate>2020", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("CIPP::ParseFields does not crash on a stray unmatched closing tag", "[cipp]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypecippcxx): Nested.Top() was
	// called with no GetSize()!=0 guard; a closing tag with nothing on
	// the stack (the very first tag here) made it dereference a null
	// PCIP_Element. Confirmed via the identical repro in cipc.cxx before
	// fixing (see docs/BUG_CATALOG.md#doctypecipccxx's SEGV report).
	TempFile file("</foo>");
	TESTIDBOBJ db;
	CIPP cipp(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	cipp.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("CIPP::ParseFields does not leak on overlapping (non-LIFO) tags", "[cipp]") {
	// BUGFIX #4 (docs/BUG_CATALOG.md#doctypecippcxx): a CIP_Element is
	// only Pop()'d+delete'd when a closing tag matches the *top* of
	// Nested. "<A><B></A></B>" pairs A with a real </A> and B with a
	// real </B> (find_end_tag() matches both), but they close in the
	// wrong order relative to each other, so </A> (processed while B is
	// still on top) never matches and leaves A's element stuck on
	// Nested forever. Verified leak-free under make tests-asan.
	TempFile file("<A><B></A></B>");
	TESTIDBOBJ db;
	CIPP cipp(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	cipp.ParseFields(&record);  // Must not leak.
	SUCCEED();
}

TEST_CASE("CIPP::LoadFieldTable does not crash on an empty FIELDTYPE file", "[cipp]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypecippcxx): IsFile() only
	// checks existence, not content; an empty FIELDTYPE file made
	// strtok() return nullptr on the very first call, and the original
	// do-while unconditionally ran `Field_and_Type = pBuf;` (a null
	// pointer) before ever checking it -- STRING::operator=(const CHR*)
	// calls strlen() on it unconditionally.
	TempFile file("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	CIPP cipp(&db);

	cipp.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("CIPP::LoadFieldTable loads real entries without crashing", "[cipp]") {
	TempFile file("62 TEXT\n12 NUMERIC\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	CIPP cipp(&db);

	cipp.LoadFieldTable();
	SUCCEED();
}
