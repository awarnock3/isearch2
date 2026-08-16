// Tests for src/nlatlon.hxx / src/nlatlon.cxx (ParseLatToNum,
// ParseLonToNum). Linked against the real implementation via
// TEST_ENGINE_SRCS in the top-level Makefile, not reimplemented here.
//
// Both functions have no callers anywhere in the current tree (see the
// file-level comment in nlatlon.cxx), so these tests exercise the
// public contract directly rather than through any real caller.

#include "catch_amalgamated.hpp"

#include "nlatlon.hxx"

#include <cstring>

TEST_CASE("ParseLatToNum parses signed and N/S-suffixed latitudes", "[nlatlon]") {
	REQUIRE(ParseLatToNum((char*)"45.5N") == Catch::Approx(45.5));
	REQUIRE(ParseLatToNum((char*)"30.0S") == Catch::Approx(-30.0));
	REQUIRE(ParseLatToNum((char*)"-20.0") == Catch::Approx(-20.0));
	REQUIRE(ParseLatToNum((char*)"90.0N") == Catch::Approx(90.0));
}

TEST_CASE("ParseLatToNum returns LatERROR for an out-of-range magnitude", "[nlatlon]") {
	REQUIRE(ParseLatToNum((char*)"91.0N") == LatERROR);
}

TEST_CASE("ParseLatToNum returns LatERROR (and does not leak) on an invalid character", "[nlatlon]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcnlatloncxx): this early-return
	// path used to skip freeing the internal accumulator buffer. There's
	// no direct way to assert "didn't leak" from inside the test itself
	// -- make tests-asan (LeakSanitizer) is what actually verifies this,
	// same as the fix's original confirmation.
	REQUIRE(ParseLatToNum((char*)"12X34") == LatERROR);
}

TEST_CASE("ParseLatToNum does not crash on a high-bit-set byte", "[nlatlon]") {
	// BUGFIX #3: isdigit() is undefined behavior for a plain (possibly
	// signed) char with the high bit set; 0xFF isn't a recognized
	// latitude character either way, so this should just report
	// LatERROR, not crash.
	char term[] = {(char)0xFF, '\0'};
	REQUIRE(ParseLatToNum(term) == LatERROR);
}

TEST_CASE("ParseLonToNum parses signed and E/W-suffixed longitudes", "[nlatlon]") {
	REQUIRE(ParseLonToNum((char*)"120.0E") == Catch::Approx(120.0));
	// West longitudes are returned as 360 - magnitude.
	REQUIRE(ParseLonToNum((char*)"45.0W") == Catch::Approx(315.0));
	REQUIRE(ParseLonToNum((char*)"200.0") == Catch::Approx(200.0));
}

TEST_CASE("ParseLonToNum returns LonERROR for an out-of-range magnitude", "[nlatlon]") {
	REQUIRE(ParseLonToNum((char*)"361.0E") == LonERROR);
}

TEST_CASE("ParseLonToNum returns LonERROR (and does not leak) on an invalid character", "[nlatlon]") {
	// BUGFIX #2: same leak-on-early-return as ParseLatToNum's BUGFIX #1.
	REQUIRE(ParseLonToNum((char*)"12X34") == LonERROR);
}

TEST_CASE("ParseLonToNum does not crash on a high-bit-set byte", "[nlatlon]") {
	char term[] = {(char)0xFF, '\0'};
	REQUIRE(ParseLonToNum(term) == LonERROR);
}
