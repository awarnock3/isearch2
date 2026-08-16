// Tests for src/rcache.hxx / src/rcache.cxx (class RCACHE - Result Set
// Cache). Linked against the real implementation via TEST_ENGINE_OBJS in
// the top-level Makefile, not reimplemented here.
//
// irset.hxx isn't self-contained yet (its own turn hasn't come up), so
// this file pre-includes its dependencies the same way rcache.cxx itself
// does, in the same order.

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
#include "rcache.hxx"

TEST_CASE("RCACHE Check on an empty cache finds nothing", "[rcache]") {
	RCACHE cache(nullptr);
	INT Slot = cache.Check(STRING("term"), 0, STRING("field"), STRING("db"));
	REQUIRE(Slot == -1);
}

TEST_CASE("RCACHE Add then Check finds the exact query", "[rcache]") {
	RCACHE cache(nullptr);
	IRSET* s = new IRSET(nullptr);
	INT Slot = cache.Add(STRING("term"), 1, STRING("field"), STRING("db"), s);
	delete s;

	REQUIRE(Slot == 0);
	REQUIRE(cache.Check(STRING("term"), 1, STRING("field"), STRING("db")) == 0);
	REQUIRE(cache.Check(STRING("other"), 1, STRING("field"), STRING("db")) == -1);
	REQUIRE(cache.Check(STRING("term"), 2, STRING("field"), STRING("db")) == -1);
}

TEST_CASE("RCACHE Fetch returns a duplicate, not the original", "[rcache]") {
	RCACHE cache(nullptr);
	IRSET* s = new IRSET(nullptr);
	cache.Add(STRING("term"), 0, STRING("field"), STRING("db"), s);
	delete s;

	PIRSET fetched = cache.Fetch(0);
	REQUIRE(fetched != nullptr);
	REQUIRE(fetched->GetTotalEntries() == 0);
	delete fetched;
}

TEST_CASE("RCACHE Fetch with an out-of-range Location returns nullptr", "[rcache]") {
	// BUGFIX #3 coverage: before the fix, Fetch() indexed ResultSet[w]
	// with no bounds check at all.
	RCACHE cache(nullptr);
	REQUIRE(cache.Fetch(-1) == nullptr);
	REQUIRE(cache.Fetch(0) == nullptr);   // cache is empty, Count == 0
	REQUIRE(cache.Fetch(MAXCACHE) == nullptr);
}

TEST_CASE("RCACHE evicts the right entry once full, without crashing or leaking", "[rcache]") {
	// BUGFIX #2 coverage: before the fix, filling the cache and adding one
	// more entry indexed ResultSet[MAXCACHE] (one past the array) via a
	// stray loop-variable reuse instead of ResultSet[MinPos], confirmed
	// via a standalone repro as a UBSan "index out of bounds" plus a
	// LeakSanitizer-detected leak of the entry that should have been
	// evicted. This test fills the cache exactly full, then adds one more
	// -- passing under ASan+UBSan (make tests-asan) is the point.
	RCACHE cache(nullptr);
	for (int i = 0; i < MAXCACHE; i++) {
		IRSET* s = new IRSET(nullptr);
		STRING Term;
		Term = i;
		cache.Add(Term, 0, STRING("field"), STRING("db"), s);
		delete s;
	}
	REQUIRE(cache.Check(STRING("0"), 0, STRING("field"), STRING("db")) == 0);

	IRSET* s = new IRSET(nullptr);
	INT EvictedSlot = cache.Add(STRING("new-term"), 0, STRING("field"), STRING("db"), s);
	delete s;

	// All MAXCACHE entries tie at GetTotalEntries() == 0, so the
	// eviction loop's strict "<" comparison keeps the first slot it saw:
	// slot 0 is evicted and reused for the new entry.
	REQUIRE(EvictedSlot == 0);
	REQUIRE(cache.Check(STRING("0"), 0, STRING("field"), STRING("db")) == -1);
	REQUIRE(cache.Check(STRING("new-term"), 0, STRING("field"), STRING("db")) == 0);
}
