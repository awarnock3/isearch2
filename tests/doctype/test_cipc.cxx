// Tests for doctype/cipc.hxx / doctype/cipc.cxx (class CIPC).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here. cipc.cxx calls a
// GetNumericValue() defined (for real, not commented out) in
// doctype/fgdc.cxx -- fgdc.cxx is linked in purely to satisfy that
// symbol at link time; it isn't otherwise exercised or processed here.
//
// CIPC's constructor just forwards to SGMLNORM(DbParent), so the same
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
#include "cipc.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_cipc_XXXXXX";
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

TEST_CASE("CIPC header defines", "[cipc]") {
	REQUIRE(CIPC_ACCEPT_EMPTY_TAGS == 0);
	REQUIRE(MAXNESTINGLEN == 1024);
}

TEST_CASE("CIPC extension constant", "[cipc]") {
	const char *ext = CIPC_SGML_EXTENSION;
	REQUIRE(ext != nullptr);
	REQUIRE(strcmp(ext, "cip") == 0);
}

TEST_CASE("CIPC_Element operations", "[cipc]") {
	CIPC_Element elem;
	STRING tag = "TEST";
	elem.set_tag(tag);
	REQUIRE(elem.get_tag() == tag);
}

TEST_CASE("CIPC::ParseDate parses a well-formed StartDate/EndDate interval", "[cipc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypecipccxx): Hold was uppercased
	// before searching for the mixed-case needle "<StartDate>", so this
	// branch could never match at all -- confirmed here by asserting an
	// actual, successful interval parse (not just "doesn't crash").
	TESTIDBOBJ db;
	CIPC cipc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipc.ParseDate("<StartDate>2020</StartDate><EndDate>2021</EndDate>", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(2020.0));
	REQUIRE(fEnd == Catch::Approx(2021.0));
}

TEST_CASE("CIPC::ParseDate sets both outputs to DATE_ERROR when StartDate has no closing tag", "[cipc]") {
	// Regression test for the missing-return half of BUGFIX #1: this
	// used to fall through (after the fix above made the branch
	// reachable) and leave *fEnd unset on this specific path.
	TESTIDBOBJ db;
	CIPC cipc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipc.ParseDate("<StartDate>2020", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("CIPC::ParseDateRange parses a well-formed StartDate/EndDate interval", "[cipc]") {
	// BUGFIX #2: same case-mismatch fix, in ParseDateRange. Unlike
	// ParseDate, a bare-year value here gets promoted to a full day
	// boundary (PromoteToDayStart()/PromoteToDayEnd()): YYYY -> YYYY0101
	// / YYYY1231.
	TESTIDBOBJ db;
	CIPC cipc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipc.ParseDateRange("<StartDate>2020</StartDate><EndDate>2021</EndDate>", &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(20200101.0));
	REQUIRE(fEnd == Catch::Approx(20211231.0));
}

TEST_CASE("CIPC::ParseDateRange sets both outputs to DATE_ERROR when StartDate has no closing tag", "[cipc]") {
	TESTIDBOBJ db;
	CIPC cipc(&db);
	DOUBLE fStart = -1.0, fEnd = -1.0;
	cipc.ParseDateRange("<StartDate>2020", &fStart, &fEnd);
	REQUIRE(fStart == DATE_ERROR);
	REQUIRE(fEnd == DATE_ERROR);
}

TEST_CASE("CIPC::ParseFields does not crash on a stray unmatched closing tag", "[cipc]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypecipccxx): Nested.Top() was
	// called with no GetSize()!=0 guard; a closing tag with nothing on
	// the stack (the very first tag here) made it dereference a null
	// PCIPC_Element. Confirmed via this exact repro before fixing.
	TempFile file("</foo>");
	TESTIDBOBJ db;
	CIPC cipc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	cipc.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("CIPC::ParseFields does not leak on overlapping (non-LIFO) tags", "[cipc]") {
	// BUGFIX #4 (docs/BUG_CATALOG.md#doctypecipccxx): a CIPC_Element is
	// only Pop()'d+delete'd when a closing tag matches the *top* of
	// Nested. "<A><B></A></B>" pairs A with a real </A> and B with a
	// real </B> (find_end_tag() matches both), but they close in the
	// wrong order relative to each other, so </A> (processed while B is
	// still on top) never matches and leaves A's element stuck on
	// Nested forever. Verified leak-free under make tests-asan.
	TempFile file("<A><B></A></B>");
	TESTIDBOBJ db;
	CIPC cipc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	cipc.ParseFields(&record);  // Must not leak.
	SUCCEED();
}

TEST_CASE("CIPC::LoadFieldTable does not crash on an empty FIELDTYPE file", "[cipc]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypecipccxx): IsFile() only
	// checks existence, not content; an empty FIELDTYPE file made
	// strtok() return nullptr on the very first call, and the original
	// do-while unconditionally ran `Field_and_Type = pBuf;` (a null
	// pointer) before ever checking it -- STRING::operator=(const CHR*)
	// calls strlen() on it unconditionally.
	TempFile file("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	CIPC cipc(&db);

	cipc.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("CIPC::LoadFieldTable loads real entries without crashing", "[cipc]") {
	TempFile file("62 TEXT\n12 NUMERIC\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	CIPC cipc(&db);

	cipc.LoadFieldTable();
	SUCCEED();
}
