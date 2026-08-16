// Tests for src/numsearch.cxx (INDEX's numeric search methods).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// NumericSearch()'s main body (the on-disk NUMERICLIST binary search)
// needs a real on-disk numeric index file to exercise meaningfully,
// the same documented scope boundary tests/src/test_index.cxx already
// draws for this area -- not covered here. The bug fixed this turn is
// on the early-return path before any file I/O, so it's testable with
// the same minimal TESTIDBOBJ fixture test_index.cxx already
// established (a fresh IDBOBJ's FieldTypes HASH is empty, so any
// FieldName resolves to FieldType "TEXT" without needing any on-disk
// setup).

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
#include "index.hxx"

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

}  // namespace

TEST_CASE("INDEX::NumericSearch returns an empty IRSET, not nullptr, for a TEXT field", "[numsearch]") {
	// BUGFIX #1 coverage: NumericSearch() used to `return nullptr` when
	// FieldName's type isn't numeric (defaults to "TEXT" when unset, as
	// it is here since this fixture never registers any field types) --
	// the actual source of a null-return pattern already confirmed to
	// crash 8+ call sites in src/geosearch.cxx and src/datesearch.cxx
	// this pass, all since fixed to guard against it downstream. Fixed
	// at the source to match every other "can't search this way"
	// boundary in this codebase: return an empty IRSET, never nullptr.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_numsearch"));

	PIRSET result = index.NumericSearch(42.0, STRING("ANYFIELD"), ZRelEQ);
	REQUIRE(result != nullptr);
	REQUIRE(result->GetTotalEntries() == 0);
	delete result;
}
