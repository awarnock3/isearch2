// Tests for src/irset.hxx / src/irset.cxx (class IRSET - Internal Search
// Result Set). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"

static void MakeIresult(IRESULT* Out, INT MdtIndex, DOUBLE Score) {
	Out->SetMdtIndex(MdtIndex);
	Out->SetScore(Score);
}

TEST_CASE("IRSET starts empty", "[irset]") {
	IRSET irset(nullptr);
	REQUIRE(irset.GetTotalEntries() == 0);
}

TEST_CASE("IRSET AddEntry adds distinct entries by MdtIndex", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 1.0);
	MakeIresult(&r2, 2, 2.0);
	irset.AddEntry(r1, 0);
	irset.AddEntry(r2, 0);
	REQUIRE(irset.GetTotalEntries() == 2);
}

TEST_CASE("IRSET AddEntry merges an entry with an existing MdtIndex", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 1.0);
	MakeIresult(&r2, 1, 2.0);
	irset.AddEntry(r1, 0);
	irset.AddEntry(r2, 0);
	REQUIRE(irset.GetTotalEntries() == 1);

	IRESULT out;
	irset.GetEntry(1, &out);
	REQUIRE(out.GetScore() == Catch::Approx(3.0));
}

TEST_CASE("IRSET GetEntry leaves its output untouched for an out-of-range index", "[irset]") {
	IRSET irset(nullptr);
	IRESULT only;
	MakeIresult(&only, 1, 1.0);
	irset.AddEntry(only, 0);

	IRESULT out;
	MakeIresult(&out, 999, 999.0);
	irset.GetEntry(0, &out);
	REQUIRE(out.GetMdtIndex() == 999);

	irset.GetEntry(99, &out);
	REQUIRE(out.GetMdtIndex() == 999);
}

TEST_CASE("IRSET FastAddEntry always appends, even for a duplicate MdtIndex", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 1.0);
	MakeIresult(&r2, 1, 2.0);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	REQUIRE(irset.GetTotalEntries() == 2);
}

TEST_CASE("IRSET AddEntry expands past the initial 1000-entry capacity", "[irset]") {
	IRSET irset(nullptr);
	for (int i = 0; i < 1500; i++) {
		IRESULT r;
		MakeIresult(&r, i, 0.0);
		irset.FastAddEntry(r, 0);
	}
	REQUIRE(irset.GetTotalEntries() == 1500);
}

TEST_CASE("IRSET copy constructor deep-copies entries and attributes independently of the source", "[irset]") {
	// BUGFIX #2 coverage: before the fix, IRSET declared no copy
	// constructor, so `IRSET b = a;` used the compiler-generated shallow
	// copy of Table, and both copies' destructors then deleted the same
	// heap array -- confirmed double-free/use-after-free under ASan.
	// Fixing it surfaced a second, deeper bug: naively base-constructing
	// via `OPERAND(OtherIrset)` shallow-copies ATTRLIST's own Table one
	// level down (see BUG_CATALOG.md under src/operand.hxx) -- also
	// confirmed via ASan before the fix routed attribute copying through
	// ATTRLIST::operator= instead. Passing under ASan (make tests-asan)
	// is the point of this whole test.
	IRSET original(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 1.0);
	MakeIresult(&r2, 2, 2.0);
	original.AddEntry(r1, 0);
	original.AddEntry(r2, 0);

	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("TITLE"));
	original.SetAttributes(attrs);

	IRSET copy = original;
	REQUIRE(copy.GetTotalEntries() == 2);

	ATTRLIST copiedAttrs;
	copy.GetAttributes(&copiedAttrs);
	STRING Name;
	copiedAttrs.AttrGetFieldName(&Name);
	REQUIRE(Name == "TITLE");

	IRESULT r3;
	MakeIresult(&r3, 3, 3.0);
	copy.AddEntry(r3, 0);
	REQUIRE(copy.GetTotalEntries() == 3);
	REQUIRE(original.GetTotalEntries() == 2);
}

TEST_CASE("IRSET operator= self-assignment preserves entries, not a self-destruct", "[irset]") {
	// BUGFIX #4 coverage: before the fix, self-assignment (`x = x;`,
	// exercised here through the virtual OPOBJ& interface the way real
	// callers -- e.g. src/opstack.cxx's operator dispatch -- would) freed
	// Table and re-Init()'d before reading OtherIrset.GetTotalEntries(),
	// which by then was reading the same just-reset object: entries
	// silently dropped from 1 to 0. Confirmed real with a standalone repro.
	IRSET irset(nullptr);
	IRESULT r;
	MakeIresult(&r, 1, 1.0);
	irset.AddEntry(r, 0);

	OPOBJ& ref = irset;
	ref = irset;
	REQUIRE(irset.GetTotalEntries() == 1);
}

TEST_CASE("IRSET operator= deep-copies entries independently of the source", "[irset]") {
	IRSET original(nullptr);
	IRESULT r;
	MakeIresult(&r, 1, 1.0);
	original.AddEntry(r, 0);

	IRSET copy(nullptr);
	IRESULT placeholder;
	MakeIresult(&placeholder, 99, 0.0);
	copy.AddEntry(placeholder, 0);

	OPOBJ& copyRef = copy;
	copyRef = original;
	REQUIRE(copy.GetTotalEntries() == 1);

	IRESULT out;
	copy.GetEntry(1, &out);
	REQUIRE(out.GetMdtIndex() == 1);
}

TEST_CASE("IRSET SortByIndex orders entries descending by MdtIndex", "[irset]") {
	// IrsetIndexCompare computes y.MdtIndex - x.MdtIndex, the same
	// "descending" convention IrsetScoreCompare uses for score -- highest
	// index first, not ascending.
	IRSET irset(nullptr);
	IRESULT r1, r2, r3;
	MakeIresult(&r1, 30, 0.0);
	MakeIresult(&r2, 10, 0.0);
	MakeIresult(&r3, 20, 0.0);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	irset.FastAddEntry(r3, 0);
	irset.SortByIndex();

	IRESULT out;
	irset.GetEntry(1, &out);
	REQUIRE(out.GetMdtIndex() == 30);
	irset.GetEntry(2, &out);
	REQUIRE(out.GetMdtIndex() == 20);
	irset.GetEntry(3, &out);
	REQUIRE(out.GetMdtIndex() == 10);
}

TEST_CASE("IRSET SortByScore orders entries descending by score", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2, r3;
	MakeIresult(&r1, 1, 1.0);
	MakeIresult(&r2, 2, 9.0);
	MakeIresult(&r3, 3, 5.0);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	irset.FastAddEntry(r3, 0);
	irset.SortByScore();

	IRESULT out;
	irset.GetEntry(1, &out);
	REQUIRE(out.GetScore() == Catch::Approx(9.0));
	irset.GetEntry(3, &out);
	REQUIRE(out.GetScore() == Catch::Approx(1.0));
}

TEST_CASE("IRSET GetHitTotal sums hit counts across entries", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	r1.SetMdtIndex(1);
	r1.SetHitCount(3);
	r2.SetMdtIndex(2);
	r2.SetHitCount(4);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	REQUIRE(irset.GetHitTotal() == 7);
}

TEST_CASE("IRSET StoreDbNum sets DbNum on every entry", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 0.0);
	MakeIresult(&r2, 2, 0.0);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	irset.StoreDbNum(5);

	IRESULT out;
	irset.GetEntry(1, &out);
	REQUIRE(out.GetDbNum() == 5);
	irset.GetEntry(2, &out);
	REQUIRE(out.GetDbNum() == 5);
}

TEST_CASE("IRSET CleanUp shrinks capacity without losing entries", "[irset]") {
	IRSET irset(nullptr);
	IRESULT r1, r2;
	MakeIresult(&r1, 1, 0.0);
	MakeIresult(&r2, 2, 0.0);
	irset.FastAddEntry(r1, 0);
	irset.FastAddEntry(r2, 0);
	irset.CleanUp();
	REQUIRE(irset.GetTotalEntries() == 2);
}

TEST_CASE("IRSET Concat appends the other set's entries without deduplicating", "[irset]") {
	IRSET a(nullptr);
	IRESULT r1;
	MakeIresult(&r1, 1, 0.0);
	a.AddEntry(r1, 0);

	IRSET b(nullptr);
	IRESULT r2;
	MakeIresult(&r2, 1, 0.0);
	b.AddEntry(r2, 0);

	a.Concat(b);
	REQUIRE(a.GetTotalEntries() == 2);
}

TEST_CASE("IRSET Expand recovers after CleanUp on an empty set", "[irset]") {
	// BUGFIX #5 coverage: Expand() used to Resize(TotalEntries*2), which
	// never grows once MaxEntries reaches 0 (reachable via CleanUp() on an
	// empty IRSET) -- 0*2 is still 0, so the next AddEntry() wrote one
	// element past a zero-size allocation. Confirmed real with a
	// standalone repro under ASan (heap-buffer-overflow at the write in
	// AddEntry()). Falling back to a minimum of 1 when TotalEntries is 0
	// breaks the stuck-at-zero cycle.
	IRSET irset(nullptr);
	irset.CleanUp();
	REQUIRE(irset.GetTotalEntries() == 0);

	IRESULT r;
	MakeIresult(&r, 1, 1.0);
	irset.AddEntry(r, 0);
	REQUIRE(irset.GetTotalEntries() == 1);

	IRESULT out;
	irset.GetEntry(1, &out);
	REQUIRE(out.GetMdtIndex() == 1);
}

TEST_CASE("IRSET AndNot's TotalEntries matches its Table when OtherIrset has duplicate MdtIndex entries", "[irset]") {
	// BUGFIX #6 coverage: AndNot() set TotalEntries to `count`, the
	// pre-MergeEntries() match count, instead of MyResult's actual
	// post-merge entry count. FastAddEntry() (used internally by
	// AndNot()) bypasses AddEntry()'s own dedup, so when OtherIrset
	// contains duplicate MdtIndex entries not present in `a`,
	// MergeEntries(0) collapses them in Table but TotalEntries kept the
	// higher pre-merge count -- any GetEntry() past Table's real end then
	// read out of bounds. Confirmed real with a standalone repro under
	// ASan (heap-buffer-overflow read in IRESULT::operator= via
	// GetEntry()).
	IRSET a(nullptr);
	IRESULT r1;
	MakeIresult(&r1, 1, 1.0);
	a.AddEntry(r1, 0);

	IRSET b(nullptr);
	IRESULT r2, r3;
	MakeIresult(&r2, 5, 2.0);
	MakeIresult(&r3, 5, 3.0);
	b.FastAddEntry(r2, 0);
	b.FastAddEntry(r3, 0);

	OPOBJ& bRef = b;
	a.AndNot(bRef);
	REQUIRE(a.GetTotalEntries() == 1);

	IRESULT out;
	a.GetEntry(1, &out);
	REQUIRE(out.GetMdtIndex() == 5);
}

TEST_CASE("IRSET And's TotalEntries matches its Table when OtherIrset has duplicate MdtIndex entries", "[irset]") {
	// BUGFIX #6 coverage: same defect as AndNot() above, in And()'s
	// mirror-image assignment of TotalEntries=count.
	IRSET a(nullptr);
	IRESULT r1;
	MakeIresult(&r1, 5, 1.0);
	a.AddEntry(r1, 0);

	IRSET b(nullptr);
	IRESULT r2, r3;
	MakeIresult(&r2, 5, 2.0);
	MakeIresult(&r3, 5, 3.0);
	b.FastAddEntry(r2, 0);
	b.FastAddEntry(r3, 0);

	OPOBJ& bRef = b;
	a.And(bRef);
	REQUIRE(a.GetTotalEntries() == 1);

	IRESULT out;
	a.GetEntry(1, &out);
	REQUIRE(out.GetMdtIndex() == 5);
}
