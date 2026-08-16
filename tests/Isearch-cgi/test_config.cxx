// Tests for Isearch-cgi/config.hxx / Isearch-cgi/config.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "config.hxx"

#include <cstring>

TEST_CASE("config.hxx is self-contained", "[config]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#isearch-cgiconfighxx): this
	// header used CHR without including its definition (src/gdt.h).
	// Merely compiling this translation unit — which includes only
	// config.hxx among the project's own headers — is the regression
	// test for that: it wouldn't have compiled beforehand.
	const CHR *v = IsearchCGIVersion;
	REQUIRE(v != nullptr);
}

TEST_CASE("IsearchCGIVersion matches the build-time VERS macro", "[config]") {
	REQUIRE(std::strcmp(IsearchCGIVersion, VERS) == 0);
}
