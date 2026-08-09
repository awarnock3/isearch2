// Tests for src/nlist.hxx / src/nlist.cxx (class NUMERICLIST).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"

#include <type_traits>

TEST_CASE("NUMERICLIST is non-copyable", "[nlist]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcnlisthxx): NUMERICLIST owns a
	// heap-allocated table with no correct copy semantics; rather than
	// implement deep-copy semantics nothing in the tree needs, the copy
	// constructor and operator= are both deleted.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<NUMERICLIST>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<NUMERICLIST>::value);
}

TEST_CASE("NUMERICLIST's Attribute and Relation start deterministically zeroed", "[nlist]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#srcnlisthxx): both constructors used
	// to leave Attribute/Relation indeterminate (every other member was
	// explicitly set).
	NUMERICLIST DefaultList;
	REQUIRE(DefaultList.GetAttribute() == 0);
	REQUIRE(DefaultList.GetRelation() == 0);
	REQUIRE(DefaultList.GetCoords() == 2);
	REQUIRE(DefaultList.GetCount() == 0);

	NUMERICLIST SizedList(3);
	REQUIRE(SizedList.GetAttribute() == 0);
	REQUIRE(SizedList.GetRelation() == 0);
	REQUIRE(SizedList.GetCoords() == 3);
}
