// Tests for src/mdt.hxx / src/mdt.cxx (class MDT - Multiple Document
// Table). Linked against the real implementation via TEST_ENGINE_OBJS
// in the top-level Makefile, not reimplemented here.
//
// MDT has no default constructor -- it always opens/creates a real
// on-disk .mdt/.mdg/.mdk trio via a file stem. Tests go through a
// TempMdt fixture, same pattern as test_filemap.cxx's.

#include "catch_amalgamated.hpp"

#include "mdt.hxx"
#include "mdtrec.hxx"
#include "string.hxx"

#include <cstdio>
#include <unistd.h>
#include <type_traits>

namespace {

// Owns the temp MDT files (<stem>.mdt/.mdg/.mdk) and cleans them up.
struct TempMdt {
	STRING Stem;
	MDT* Mdt;

	TempMdt() {
		char tmpl[] = "/tmp/isearch2_test_mdt_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		remove(tmpl);  // MDT creates its own <stem>.mdt/.mdg/.mdk instead
		Stem = tmpl;
		Mdt = new MDT(Stem, GDT_FALSE);
	}

	SIZE_T AddDoc(const CHR* Key, GPTYPE GpStart, GPTYPE LocalStart,
		      GPTYPE LocalEnd) {
		MDTREC rec;
		rec.SetKey(STRING(Key));
		rec.SetGlobalFileStart(GpStart);
		rec.SetLocalRecordStart(LocalStart);
		rec.SetLocalRecordEnd(LocalEnd);
		Mdt->AddEntry(rec);
		return Mdt->GetTotalEntries();
	}

	~TempMdt() {
		delete Mdt;
		STRING Fn;
		Fn = Stem; Fn.Cat(".mdt"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdg"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdk"); remove(Fn);
	}
};

}  // namespace

TEST_CASE("MDT is not copyable", "[mdt]") {
	// BUGFIX #1 regression: this is a compile-time check, not a
	// runtime assertion -- MDT(const MDT&) and operator= are both
	// = delete, so std::is_copy_constructible / is_copy_assignable
	// must report false.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<MDT>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<MDT>::value);
}

TEST_CASE("MDT starts empty on a fresh file stem", "[mdt]") {
	TempMdt Tmp;
	REQUIRE(Tmp.Mdt->GetTotalEntries() == 0);
	REQUIRE(Tmp.Mdt->GetChanged() == GDT_FALSE);
}

TEST_CASE("MDT AddEntry/GetEntry round-trips (1-based)", "[mdt]") {
	TempMdt Tmp;
	Tmp.AddDoc("doc-1", 0, 0, 99);
	REQUIRE(Tmp.Mdt->GetTotalEntries() == 1);
	REQUIRE(Tmp.Mdt->GetChanged() == GDT_TRUE);

	MDTREC out;
	Tmp.Mdt->GetEntry(1, &out);
	STRING s;
	out.GetKey(&s);
	REQUIRE(s == "doc-1");
	REQUIRE(out.GetGlobalFileStart() == 0u);
	REQUIRE(out.GetLocalRecordEnd() == 99u);
}

TEST_CASE("MDT LookupByKey finds an entry by its key", "[mdt]") {
	TempMdt Tmp;
	Tmp.AddDoc("alpha", 0, 0, 9);
	Tmp.AddDoc("beta", 10, 0, 9);

	REQUIRE(Tmp.Mdt->LookupByKey(STRING("beta")) == 2);
	REQUIRE(Tmp.Mdt->LookupByKey(STRING("does-not-exist")) == 0);
}

TEST_CASE("MDT LookupByGp finds the entry containing a global offset", "[mdt]") {
	TempMdt Tmp;
	Tmp.AddDoc("doc-1", 0, 0, 99);    // global range [0, 99]
	Tmp.AddDoc("doc-2", 100, 0, 49);  // global range [100, 149]

	REQUIRE(Tmp.Mdt->LookupByGp(50) == 1);
	REQUIRE(Tmp.Mdt->LookupByGp(120) == 2);
	REQUIRE(Tmp.Mdt->LookupByGp(1000) == 0);
}

TEST_CASE("MDT SetEntry updates an existing on-disk record", "[mdt]") {
	TempMdt Tmp;
	Tmp.AddDoc("doc-1", 0, 0, 99);

	MDTREC updated;
	updated.SetKey(STRING("doc-1-renamed"));
	updated.SetGlobalFileStart(0);
	updated.SetLocalRecordStart(0);
	updated.SetLocalRecordEnd(199);
	Tmp.Mdt->SetEntry(1, updated);

	MDTREC out;
	Tmp.Mdt->GetEntry(1, &out);
	REQUIRE(out.GetLocalRecordEnd() == 199u);
}

TEST_CASE("MDT RemoveDeleted compacts out deleted entries", "[mdt]") {
	TempMdt Tmp;
	Tmp.AddDoc("keep-1", 0, 0, 9);
	Tmp.AddDoc("delete-me", 10, 0, 9);
	Tmp.AddDoc("keep-2", 20, 0, 9);

	MDTREC rec;
	Tmp.Mdt->GetEntry(2, &rec);
	rec.SetDeleted(GDT_TRUE);
	Tmp.Mdt->SetEntry(2, rec);

	SIZE_T RemovedCount = Tmp.Mdt->RemoveDeleted();
	REQUIRE(RemovedCount == 1u);
	REQUIRE(Tmp.Mdt->GetTotalEntries() == 2u);

	STRING s;
	MDTREC out;
	Tmp.Mdt->GetEntry(1, &out);
	out.GetKey(&s);
	REQUIRE(s == "keep-1");
	Tmp.Mdt->GetEntry(2, &out);
	out.GetKey(&s);
	REQUIRE(s == "keep-2");
}

TEST_CASE("MDT GetUniqueKey produces distinct keys across calls", "[mdt]") {
	// BUGFIX #3 regression: sprintf -> snprintf; also exercises the
	// fixed-size stack buffer with a realistic value.
	TempMdt Tmp;
	STRING a, b;
	Tmp.Mdt->GetUniqueKey(&a);
	Tmp.Mdt->GetUniqueKey(&b);
	REQUIRE(!(a == b));
}

TEST_CASE("MDT survives destruction after FlushMDTIndexes writes the on-disk indexes", "[mdt]") {
	// Same shape of concern as the original bug (see
	// docs/AUTOPILOT_LOG.md#srcmdthxx): the crash happened one level
	// inside FlushMDTIndexes() -> SortGpIndex() -> qsort(), which now
	// only runs against a single, non-copied MDT (MDT is no longer
	// copyable at all, so there's no copy to double-free).
	TempMdt Tmp;
	Tmp.AddDoc("doc-1", 0, 0, 9);
	Tmp.AddDoc("doc-2", 10, 0, 9);
	Tmp.Mdt->FlushMDTIndexes();
	SUCCEED("no ASan/UBSan failure flushing and destroying a real MDT");
}
