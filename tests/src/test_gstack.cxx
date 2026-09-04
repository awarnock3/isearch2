// Tests for src/gstack.hxx / src/gstack.cxx (class GSTACK).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// GSTACK (like GLIST, see tests/src/test_glist.cxx) has no destructor,
// so every push in these tests is popped before the GSTACK goes out of
// scope.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "glist.hxx"
#include "gstack.hxx"

TEST_CASE("GSTACK starts empty", "[gstack]") {
	GSTACK s;
	REQUIRE(s.GetSize() == 0);
}

TEST_CASE("GSTACK::Top and Pop on an empty stack return nullptr instead of crashing", "[gstack]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcgstackhxx): both used to
	// unconditionally dereference the (null, on an empty stack)
	// CurrentIndex via GLIST::Retrieve() -- confirmed a real
	// null-pointer-dereference crash (SEGV) with a standalone repro
	// before fixing.
	GSTACK s;
	REQUIRE(s.Top() == nullptr);
	REQUIRE(s.Pop() == nullptr);
}

TEST_CASE("GSTACK Push/Top/Pop behaves as a LIFO stack", "[gstack]") {
	GSTACK s;
	int a = 1, b = 2, c = 3;
	s.Push(&a);
	s.Push(&b);
	s.Push(&c);
	REQUIRE(s.GetSize() == 3);

	REQUIRE(*(int*)s.Top() == 3);  // peek doesn't pop
	REQUIRE(s.GetSize() == 3);

	REQUIRE(*(int*)s.Pop() == 3);
	REQUIRE(s.GetSize() == 2);
	REQUIRE(*(int*)s.Pop() == 2);
	REQUIRE(*(int*)s.Pop() == 1);
	REQUIRE(s.GetSize() == 0);
	REQUIRE(s.Pop() == nullptr);
}
