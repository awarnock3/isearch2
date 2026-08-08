// Tests for doctype/doc_conf.hxx (a pure-macro configuration header,
// no functions/classes). doctype/mailfolder.cxx is its only current
// includer in the tree.

#include "catch_amalgamated.hpp"

// BUGFIX #1 (docs/BUG_CATALOG.md#doctypedoc_confhxx): doc_conf.hxx had
// no include guard at all. Including it twice in a row here is the
// regression test -- without the fix, this would have redefined
// STRICT_HTML/RESTRICT_MAIL_FIELDS/SHOW_MAIL_DATE/USE_UNIFIED_NAMES a
// second time (harmlessly, since the values are identical each time)
// and, more importantly, re-run the REFER_UNIFIED_NAMES/
// MEDLINE_UNIFIED_NAMES/FILMLINE_UNIFIED_NAMES/BSN_EXTENSIONS/
// BRIEF_MAGIC #ifndef blocks a second time -- which happen to be
// idempotent too, so this wouldn't have actually failed to compile
// even before the fix. The guard is still the correct, standard fix;
// this test just confirms double-inclusion is safe going forward.
#include "doc_conf.hxx"
#include "doc_conf.hxx"

#include <cstring>

TEST_CASE("doc_conf.hxx defines its documented default macro values", "[doc_conf]") {
	REQUIRE(STRICT_HTML == 1);
	REQUIRE(RESTRICT_MAIL_FIELDS == 1);
	REQUIRE(SHOW_MAIL_DATE == 0);
	REQUIRE(USE_UNIFIED_NAMES == 1);
	REQUIRE(BSN_EXTENSIONS == 0);
	REQUIRE(std::strcmp(BRIEF_MAGIC, "B") == 0);
}

TEST_CASE("doc_conf.hxx's per-doctype overrides default to USE_UNIFIED_NAMES", "[doc_conf]") {
	// BUGFIX #2: the duplicate USE_UNIFIED_NAMES definition was removed;
	// this confirms the single remaining definition still correctly
	// feeds these three fallbacks.
	REQUIRE(REFER_UNIFIED_NAMES == USE_UNIFIED_NAMES);
	REQUIRE(MEDLINE_UNIFIED_NAMES == USE_UNIFIED_NAMES);
	REQUIRE(FILMLINE_UNIFIED_NAMES == USE_UNIFIED_NAMES);
}
