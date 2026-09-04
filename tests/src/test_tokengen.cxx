// Tests for src/tokengen.hxx / src/tokengen.cxx (class TOKENGEN).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "tokengen.hxx"

#include <type_traits>

TEST_CASE("TOKENGEN is non-copyable", "[tokengen]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srctokengenhxx): TOKENGEN owns a
	// heap-allocated InCharP with no correct copy semantics; rather than
	// implement deep-copy semantics nothing in the tree needs, the copy
	// constructor and operator= are both deleted.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<TOKENGEN>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<TOKENGEN>::value);
}

TEST_CASE("TOKENGEN splits plain words on whitespace", "[tokengen]") {
	TOKENGEN Gen(STRING("hello world"));
	REQUIRE(Gen.GetTotalEntries() == 2);
	STRING S;
	Gen.GetEntry(1, &S);
	REQUIRE(S == "hello");
	Gen.GetEntry(2, &S);
	REQUIRE(S == "world");
}

TEST_CASE("TOKENGEN with matched quotes returns the quoted text as one token", "[tokengen]") {
	TOKENGEN Gen(STRING("prefix \"quoted text\" suffix"));
	REQUIRE(Gen.GetTotalEntries() == 3);
	STRING S;
	Gen.GetEntry(2, &S);
	REQUIRE(S == "\"quoted text\"");
}

TEST_CASE("TOKENGEN preserves preceding text when an unmatched quote is stripped", "[tokengen]") {
	// BUGFIX #2 (most severe finding in this file, and the exact repro
	// from docs/BUG_CATALOG.md): with quote-stripping enabled, an
	// unmatched quote used to wipe the *entire* token (including
	// "prefix", accumulated before the quote was even reached) and skip
	// an extra character of the trailing text. Before this fix,
	// tokenizing `prefix"unmatched rest` with stripping enabled produced
	// ["nmatched", "rest"] -- losing "prefix" entirely and misreading
	// "unmatched" as "nmatched".
	TOKENGEN Gen(STRING("prefix\"unmatched rest"));
	Gen.SetQuoteStripping(GDT_TRUE);
	REQUIRE(Gen.GetTotalEntries() == 2);
	STRING S;
	Gen.GetEntry(1, &S);
	REQUIRE(S == "prefixunmatched");
	Gen.GetEntry(2, &S);
	REQUIRE(S == "rest");
}

TEST_CASE("TOKENGEN preserves preceding text when an unmatched brace is found", "[tokengen]") {
	// BUGFIX #2, brace case: the fallback searched for the closing '}'
	// (never present in this branch) instead of the opening '{' (always
	// present), so it always wiped the entire token, including any text
	// accumulated before the brace. Unlike the quote case, the '{'
	// itself also ends up discarded here: EraseAfter() restores the
	// token to its length from *before* the '{' was appended, and the
	// resume position is just past the '{', so it isn't re-appended
	// either -- verified empirically against the real implementation,
	// not assumed.
	TOKENGEN Gen(STRING("prefix{unmatched rest"));
	REQUIRE(Gen.GetTotalEntries() == 2);
	STRING S;
	Gen.GetEntry(1, &S);
	REQUIRE(S == "prefixunmatched");
	Gen.GetEntry(2, &S);
	REQUIRE(S == "rest");
}

TEST_CASE("TOKENGEN with matched braces returns the group as one token", "[tokengen]") {
	// Pre-existing (unchanged by this turn's fixes) asymmetry, verified
	// empirically: the opening '{' is kept in the token but the closing
	// '}' is not -- `if (*input == '}') { input++; istoken = 1; }` skips
	// past it without ever appending it. Not flagged by the original
	// docs/AUTOPILOT_LOG.md finding and not a crash/safety issue, so
	// left as-is; this test documents actual current behavior rather
	// than an assumed symmetric one.
	TOKENGEN Gen(STRING("RECT{40 30 -70 -60} other"));
	REQUIRE(Gen.GetTotalEntries() == 2);
	STRING S;
	Gen.GetEntry(1, &S);
	REQUIRE(S == "RECT{40 30 -70 -60");
}
