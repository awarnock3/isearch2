// Tests for src/geosearch.cxx (INDEX's geographic bounding-box search
// methods). Linked against the real implementation via TEST_ENGINE_OBJS
// in the top-level Makefile, not reimplemented here. Reuses
// test_index.cxx's TESTIDBOBJ fixture shape: INDEX's constructor
// tolerates a minimal IDBOBJ, so no real on-disk index is needed to
// exercise BoundingRectangle()/Interval()'s own null-handling, which
// runs before any file is ever touched (NumericSearch() bails out as
// soon as it sees the field isn't numerically typed).

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

TEST_CASE("BoundingRectangle returns an empty, non-null IRSET when the boundary fields aren't numeric", "[geosearch]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcgeosearchcxx): NumericSearch()
	// (src/numsearch.cxx) returns nullptr whenever FieldName isn't a
	// numeric field in this database. TESTIDBOBJ's FieldTypes hash is
	// empty, so every one of NORTHBC/SOUTHBC/EASTBC/WESTBC looks
	// unconfigured (defaults to "TEXT" inside NumericSearch) -- the
	// same condition a real, misconfigured or non-geo database would
	// hit. Before the fix, this crashed with a null-pointer dereference
	// on the very first LessThanNorth->And(...) call.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_geosearch_rect"));

	PIRSET pirset = index.BoundingRectangle(10.0, -10.0, -10.0, 10.0);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}

TEST_CASE("Interval returns an empty, non-null IRSET when the boundary fields aren't numeric", "[geosearch]") {
	// BUGFIX #1: same as above, exercising Interval() directly rather
	// than through BoundingRectangle().
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_geosearch_interval"));

	PIRSET pirset = index.Interval(-10.0, 10.0, -10.0, 10.0);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}

TEST_CASE("BoundingRectangle handles a Date-Line-crossing query without crashing", "[geosearch]") {
	// WestBC > EastBC triggers the Date-Line-split branch, which itself
	// funnels through Interval() -> NumericSearch(); same null-safety
	// path as above, just through the split-interval code.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_geosearch_dateline"));

	PIRSET pirset = index.BoundingRectangle(10.0, -10.0, 170.0, -170.0);
	REQUIRE(pirset != nullptr);
	REQUIRE(pirset->GetTotalEntries() == 0);
	delete pirset;
}
