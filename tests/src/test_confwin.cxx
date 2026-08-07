// Tests for src/confwin.h. No implementation file. See
// docs/BUG_CATALOG.md#srcconfwinh -- no bugs found; this confirms both
// of its data-model branches (16-bit MS-DOS vs. Win32/Win64 LLP64)
// compile and hold their documented values. Can't be exercised against
// a real Windows/DOS toolchain from this Linux build, so this is a
// static/compile-time check of the header's own internal consistency,
// not an integration test against either target platform.

#include "catch_amalgamated.hpp"

#include "confwin.h"

TEST_CASE("confwin.h's #ifndef _WIN32 branch matches classic 16-bit MS-DOS", "[confwin]") {
	// _WIN32 is not defined in this translation unit, so this is the
	// branch actually in effect here.
	REQUIRE(SIZEOF_SHORT_INT == 2);
	REQUIRE(SIZEOF_INT == 2);
	REQUIRE(SIZEOF_LONG_INT == 4);
	REQUIRE(SIZEOF_LONG_LONG_INT == 8);
}
