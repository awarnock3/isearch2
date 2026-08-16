// Tests for src/soundex.hxx / src/soundex.cxx (SoundexEncode).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "soundex.hxx"

TEST_CASE("SoundexEncode handles simple names with no first-letter collision", "[soundex]") {
	STRING out;
	SoundexEncode(STRING("Robert"), &out);
	REQUIRE(out == "R163");
	SoundexEncode(STRING("Rupert"), &out);
	REQUIRE(out == "R163");
}

TEST_CASE("SoundexEncode collapses a second letter that shares the first letter's code", "[soundex]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcsoundexcxx): P and F share
	// digit 1; the old implementation never compared against the first
	// letter's own code, so it kept both instead of collapsing them.
	STRING out;
	SoundexEncode(STRING("Pfister"), &out);
	REQUIRE(out == "P236");
}

TEST_CASE("SoundexEncode re-codes a repeated digit separated by a vowel", "[soundex]") {
	// BUGFIX #1 continued: stripping zeros before deduplicating merged
	// same-digit letters that were originally separated by a vowel into
	// a false adjacency, undercounting them.
	STRING out;
	SoundexEncode(STRING("Tymczak"), &out);
	REQUIRE(out == "T522");
	SoundexEncode(STRING("Honeyman"), &out);
	REQUIRE(out == "H555");
}

TEST_CASE("SoundexEncode pads short codes with zeros", "[soundex]") {
	STRING out;
	SoundexEncode(STRING("Lee"), &out);
	REQUIRE(out == "L000");
}

TEST_CASE("SoundexEncode truncates to 4 characters", "[soundex]") {
	STRING out;
	SoundexEncode(STRING("Washington"), &out);
	REQUIRE(out.GetLength() == 4);
	REQUIRE(out == "W252");
}

TEST_CASE("SoundexEncode is case-insensitive", "[soundex]") {
	STRING upper, lower;
	SoundexEncode(STRING("ROBERT"), &upper);
	SoundexEncode(STRING("robert"), &lower);
	REQUIRE(upper == lower);
}

TEST_CASE("SoundexEncode on empty input produces empty output", "[soundex]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcsoundexcxx): the old code
	// read GetChr(1) on an empty STRING (documented to return 0, not the
	// character '0') and appended that null byte as the "first letter",
	// producing a 4-byte result starting with an embedded null instead
	// of a clean empty string.
	STRING out;
	SoundexEncode(STRING(""), &out);
	REQUIRE(out == "");
	REQUIRE(out.GetLength() == 0);
}
