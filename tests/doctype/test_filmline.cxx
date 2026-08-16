// Tests for doctype/filmline.hxx / doctype/filmline.cxx (class
// FILMLINE). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// FILMLINE's constructor forwards to MEDLINE(DbParent), which itself
// forwards to DOCTYPE(DbParent), so the same minimal TESTIDBOBJ shape
// used by the other doctype/ tests works here too.

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
#include "medline.hxx"
#include "filmline.hxx"

#include <cstring>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

}  // namespace

TEST_CASE("FILMLINE::UnifiedName maps known Filmline field codes", "[filmline]") {
	TESTIDBOBJ db;
	FILMLINE fl(&db);
	REQUIRE(strcmp(fl.UnifiedName("TI"), "title_true") == 0);
	REQUIRE(strcmp(fl.UnifiedName("DI"), "director") == 0);
	REQUIRE(fl.UnifiedName("ZZ") == nullptr);
}

TEST_CASE("FILMLINE::Present element set B does not crash when the title field is empty", "[filmline]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefilmlinecxx): when the "TI"
	// (title_true) field comes back empty, Present() used to fall back
	// to `Tag = UnifiedName("TO");` -- but "TO" isn't a code in
	// UnifiedName()'s table at all, so UnifiedName() always returned
	// nullptr here, and `Tag = nullptr` (STRING::operator=(const CHR*))
	// called strlen(nullptr) unconditionally -- a real, always-reachable
	// null-pointer-dereference crash. A default (no-op) TESTIDBOBJ's
	// GetFieldData() never finds anything, so Title.GetLength()==0 is
	// unconditionally true here, making this crash 100% reliable before
	// the fix -- confirmed via a standalone run that segfaulted here
	// prior to applying BUGFIX #1.
	TESTIDBOBJ db;
	FILMLINE fl(&db);

	RESULT r;
	STRING out;
	fl.Present(r, "B", &out);  // Must not crash.
	SUCCEED();
}
