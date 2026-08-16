// Tests for src/gdt.h. No implementation file (pure typedefs/macros).
// See docs/BUG_CATALOG.md#srcgdth -- no bugs found; this confirms the
// size-derived typedef cascades resolve to their documented widths on
// this platform.

#include "catch_amalgamated.hpp"

#include "gdt.h"

static_assert(sizeof(INT2) == 2, "INT2 should be 2 bytes");
static_assert(sizeof(UINT2) == 2, "UINT2 should be 2 bytes");
static_assert(sizeof(INT4) == 4, "INT4 should be 4 bytes");
static_assert(sizeof(UINT4) == 4, "UINT4 should be 4 bytes");
static_assert(sizeof(INT8) == 8, "INT8 should be 8 bytes");
static_assert(sizeof(UINT8) == 8, "UINT8 should be 8 bytes");
static_assert(sizeof(INT) == sizeof(int), "INT should alias int");
static_assert(sizeof(CHR) == 1, "CHR should be 1 byte");

TEST_CASE("gdt.h's fixed-width typedefs match their documented sizes", "[gdt]") {
	REQUIRE(sizeof(INT2) == 2);
	REQUIRE(sizeof(UINT2) == 2);
	REQUIRE(sizeof(INT4) == 4);
	REQUIRE(sizeof(UINT4) == 4);
	REQUIRE(sizeof(INT8) == 8);
	REQUIRE(sizeof(UINT8) == 8);
}

TEST_CASE("GDT_BOOLEAN round-trips through bool correctly", "[gdt]") {
	REQUIRE(static_cast<bool>(GDT_TRUE) == true);
	REQUIRE(static_cast<bool>(GDT_FALSE) == false);
}
