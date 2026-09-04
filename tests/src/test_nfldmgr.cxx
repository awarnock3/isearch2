// Tests for src/nfldmgr.hxx / src/nfldmgr.cxx (class NUMERICFLDMGR).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// NUMERICFLDMGR has no callers anywhere in the current tree (see the
// file-level comment in nfldmgr.cxx) and depends on NUMERICLIST's
// LoadTable()/Find(), which the class's own LoadFields()/Find() never
// actually call (left commented out by the original author) -- so a
// loaded field's NUMERICLIST is always empty. These tests exercise the
// real bug fixes (a compile error, a missing bounds check, an
// unbounded sscanf) rather than the unfinished numeric-lookup feature
// itself.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "nfldmgr.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

// LoadFields() appends ".fdf" to whatever dbName it's given, so the
// helper reserves a unique base path via mkstemp() (removing that
// placeholder immediately) and creates "<base>.fdf" itself.
struct TempFdf {
	std::string DbName;
	std::string FdfPath;

	explicit TempFdf(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_nfldmgr_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		remove(tmpl);
		DbName = tmpl;
		FdfPath = DbName + ".fdf";
		FILE* fp = fopen(FdfPath.c_str(), "wb");
		fwrite(Contents, 1, strlen(Contents), fp);
		fclose(fp);
	}

	~TempFdf() {
		remove(FdfPath.c_str());
	}
};

}  // namespace

TEST_CASE("NUMERICFLDMGR::LoadFields returns 0 when the .fdf file doesn't exist", "[nfldmgr]") {
	NUMERICFLDMGR mgr;
	REQUIRE(mgr.LoadFields((PCHR)"/tmp/isearch2_test_nfldmgr_does_not_exist") == 0);
}

TEST_CASE("NUMERICFLDMGR::LoadFields skips TEXT fields and loads no NUMERIC ones (LoadTable is unimplemented)", "[nfldmgr]") {
	// See the file-level comment in nfldmgr.cxx: LoadTable() is commented
	// out in LoadFields(), so GetCount() (after BUGFIX #1's `()` fix) is
	// always 0 and every non-TEXT field is skipped too. This pins that
	// documented, pre-existing incomplete behavior rather than a bug
	// introduced by this turn's fixes.
	//
	// Also exercises BUGFIX #5: this is exactly the shape that leaked
	// under ASan before the fix -- LoadFields() allocates `fields` (the
	// .fdf has non-comment/non-blank lines) but NumFields stays 0, so
	// mgr's destructor used to skip freeing it entirely.
	TempFdf fdf(
		"# comment line\n"
		"62 TEXT\n"
		"12 NUMERIC\n"
		"\n");
	NUMERICFLDMGR mgr;
	REQUIRE(mgr.LoadFields((PCHR)fdf.DbName.c_str()) == 0);
}

TEST_CASE("NUMERICFLDMGR::LoadFields does not overflow on an oversized type token", "[nfldmgr]") {
	// BUGFIX #4 (docs/BUG_CATALOG.md#srcnfldmgrcxx): `sscanf(Input,"%d %s",
	// ...)` had no width limit on TypeString[128]. This 300-byte second
	// token would have overflowed it before the `%127s` fix.
	std::string longtoken(300, 'X');
	std::string content = "12 " + longtoken + "\n";
	TempFdf fdf(content.c_str());
	NUMERICFLDMGR mgr;
	REQUIRE(mgr.LoadFields((PCHR)fdf.DbName.c_str()) == 0);  // not "TEXT" or "NUMERIC" (case aside) -> falls to the else branch, still 0 per above
}

TEST_CASE("NUMERICFLDMGR::GetResult does nothing when no field matches", "[nfldmgr]") {
	// BUGFIX #3: LocateFieldByAttribute() returns -1 for an unloaded/
	// unmatched attribute; GetResult() used to index fields[-1]
	// unconditionally. A default-constructed mgr (NumFields == 0, fields
	// == nullptr per BUGFIX #2) reliably takes that -1 path.
	NUMERICFLDMGR mgr;
	mgr.GetResult(42, nullptr, nullptr);  // must not crash
	SUCCEED();
}

TEST_CASE("NUMERICFLDMGR::Find returns 0 when no field matches", "[nfldmgr]") {
	NUMERICFLDMGR mgr;
	REQUIRE(mgr.Find(42, 0, 1.0f) == 0);
}
