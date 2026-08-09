// Tests for src/datesearch.cxx (INDEX's date-search methods).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here. Reuses test_index.cxx's
// TESTIDBOBJ fixture shape: INDEX's constructor tolerates a minimal
// IDBOBJ (ComposeDbFn/ffopen both have harmless default bodies), so no
// real on-disk index is needed to exercise SingleDateSearchBefore()/
// SingleDateSearchAfter()'s own date-parsing logic, which runs before
// any file is ever touched.

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

TEST_CASE("SingleDateSearchBefore returns an empty, non-null IRSET for an unparseable date", "[datesearch]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdatesearchcxx): a query date
	// that fails to parse leaves SRCH_DATE::GetPrecision() == BAD_DATE,
	// which used to return the function's still-null YYYY local
	// straight out of the switch's default case. "garbage" parses (via
	// STRING::GetFloat()) to 0.0, which is below YEAR_LOWER (99.0), so
	// it lands on BAD_DATE the same way a malformed query term would in
	// production.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_datesearch_before"));

	SRCH_DATE badDate(STRING("garbage"));
	REQUIRE(badDate.GetPrecision() == BAD_DATE);

	PIRSET pirset = index.SingleDateSearchBefore(badDate, STRING("DATE_FIELD"),
						      START_BLOCK, GDT_TRUE);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}

TEST_CASE("SingleDateSearchAfter returns an empty, non-null IRSET for an unparseable date", "[datesearch]") {
	// BUGFIX #2: same shape as BUGFIX #1 above, in the twin function.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_datesearch_after"));

	SRCH_DATE badDate(STRING("garbage"));
	REQUIRE(badDate.GetPrecision() == BAD_DATE);

	PIRSET pirset = index.SingleDateSearchAfter(badDate, STRING("DATE_FIELD"),
						     START_BLOCK, GDT_TRUE);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}

TEST_CASE("DateRangeSearchContains doesn't crash when one endpoint fails to parse", "[datesearch]") {
	// Downstream consumer of BUGFIX #1/#2: DateRangeSearchContains()
	// unconditionally dereferences both SingleDateSearchBefore()'s and
	// SingleDateSearchAfter()'s return values (pirset->And(*pirset1)).
	// Before the fix, a DATERANGE built from an unparseable endpoint
	// would have made one of those a null-pointer dereference; this
	// confirms the whole path is safe end-to-end, not just the two
	// leaf functions in isolation.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_datesearch_range"));

	DATERANGE range(STRING("garbage garbage"));
	PIRSET pirset = index.DateRangeSearchContains(range, STRING("DATE_FIELD"),
						       START_BLOCK, GDT_TRUE);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}
