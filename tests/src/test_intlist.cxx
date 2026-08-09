// Tests for src/intlist.hxx / src/intlist.cxx (class INTERVALLIST).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"

#include <type_traits>

TEST_CASE("INTERVALLIST is non-copyable", "[intlist]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcintlisthxx): INTERVALLIST owns a
	// heap-allocated table with no correct copy semantics; rather than
	// implement deep-copy semantics nothing in the tree needs, the copy
	// constructor and operator= are both deleted.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<INTERVALLIST>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<INTERVALLIST>::value);
}

TEST_CASE("INTERVALLIST constructs and destroys cleanly", "[intlist]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#srcintlisthxx) is not directly
	// observable here: it fixes INTERVALLIST's own shadowed
	// Attribute/Relation members, but INTERVALLIST declares no accessor
	// for them (GetAttribute()/GetRelation() are inherited from
	// NUMERICLIST and read the *base* class's already-fixed copies, not
	// INTERVALLIST's own) -- verified by inspection only; see
	// docs/BUG_CATALOG.md. This test just confirms basic construction
	// still behaves as expected.
	INTERVALLIST DefaultList;
	REQUIRE(DefaultList.GetCoords() == 3);
	REQUIRE(DefaultList.GetCount() == 0);

	INTERVALLIST SizedList(4);
	REQUIRE(SizedList.GetCoords() == 4);
}
