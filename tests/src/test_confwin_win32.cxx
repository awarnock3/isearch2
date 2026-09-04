// Covers src/confwin.h's #else (_WIN32 defined) branch -- needs its
// own translation unit since confwin.h's include guard would block a
// second, differently-configured #include in the same file as
// test_confwin.cxx. See docs/BUG_CATALOG.md#srcconfwinh.

#include "catch_amalgamated.hpp"

#define _WIN32 1
#include "confwin.h"

TEST_CASE("confwin.h's #else (_WIN32) branch matches Win32/Win64's LLP64 model", "[confwin]") {
	REQUIRE(SIZEOF_SHORT_INT == 2);
	REQUIRE(SIZEOF_INT == 4);
	REQUIRE(SIZEOF_LONG_INT == 4);
	REQUIRE(SIZEOF_LONG_LONG_INT == 8);
}
