// Tests for Isearch-cgi/api_config.hxx / Isearch-cgi/api_config.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_OBJS in the
// top-level Makefile, not reimplemented here.
//
// ApiConfig reads its settings from environment variables, so each test
// sets exactly the ones it cares about and clears the rest first, to stay
// independent of whatever the test runner's own environment happens to
// have set.

#include "catch_amalgamated.hpp"

#include "api_config.hxx"

#include <cstdlib>
#include <climits>

namespace {

void ClearApiEnv() {
	unsetenv("ISEARCH_DB_PATH");
	unsetenv("ISEARCH_API_MAX_HITS");
	unsetenv("ISEARCH_API_ALLOWLIST");
}

}  // namespace

TEST_CASE("ApiConfig defaults when no environment variables are set", "[api_config]") {
	ClearApiEnv();
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.db_path.GetLength() == 0);
	REQUIRE(cfg.max_hits_ceiling == API_HARD_MAX_HITS);
	REQUIRE(cfg.allow_list.GetTotalEntries() == 0);
}

TEST_CASE("ApiConfig picks up ISEARCH_DB_PATH", "[api_config]") {
	ClearApiEnv();
	setenv("ISEARCH_DB_PATH", "/some/db/path", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.db_path == "/some/db/path");
	ClearApiEnv();
}

TEST_CASE("ApiConfig parses a valid ISEARCH_API_MAX_HITS", "[api_config]") {
	ClearApiEnv();
	setenv("ISEARCH_API_MAX_HITS", "200", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.max_hits_ceiling == 200);
	ClearApiEnv();
}

TEST_CASE("ApiConfig falls back to the default for a non-numeric ISEARCH_API_MAX_HITS", "[api_config]") {
	ClearApiEnv();
	setenv("ISEARCH_API_MAX_HITS", "not-a-number", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.max_hits_ceiling == API_HARD_MAX_HITS);
	ClearApiEnv();
}

TEST_CASE("ApiConfig falls back to the default for a non-positive ISEARCH_API_MAX_HITS", "[api_config]") {
	ClearApiEnv();
	setenv("ISEARCH_API_MAX_HITS", "0", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.max_hits_ceiling == API_HARD_MAX_HITS);
	ClearApiEnv();
}

TEST_CASE("ApiConfig clamps an ISEARCH_API_MAX_HITS above INT_MAX instead of truncating it", "[api_config]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_confighxx): the
	// strtol() result (a long) used to be cast straight to INT with no
	// range check -- "99999999999" silently truncated to 1215752191
	// instead of being rejected or clamped. Confirmed with a standalone
	// repro before this fix; now clamped to INT_MAX.
	ClearApiEnv();
	setenv("ISEARCH_API_MAX_HITS", "99999999999", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.max_hits_ceiling == INT_MAX);
	ClearApiEnv();
}

TEST_CASE("ApiConfig splits, trims, and skips empty entries in ISEARCH_API_ALLOWLIST", "[api_config]") {
	ClearApiEnv();
	setenv("ISEARCH_API_ALLOWLIST", " alpha ,beta,, gamma", 1);
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.allow_list.GetTotalEntries() == 3);
	STRING s;
	cfg.allow_list.GetEntry(1, &s);
	REQUIRE(s == "alpha");
	cfg.allow_list.GetEntry(2, &s);
	REQUIRE(s == "beta");
	cfg.allow_list.GetEntry(3, &s);
	REQUIRE(s == "gamma");
	ClearApiEnv();
}

TEST_CASE("ApiConfig leaves allow_list empty for an unset ISEARCH_API_ALLOWLIST", "[api_config]") {
	ClearApiEnv();
	ApiConfig cfg = LoadApiConfig();
	REQUIRE(cfg.allow_list.GetTotalEntries() == 0);
}

TEST_CASE("API_VERSION/API_DEFAULT_MAX_HITS/API_HARD_MAX_HITS are sane", "[api_config]") {
	REQUIRE(API_VERSION != nullptr);
	REQUIRE(API_DEFAULT_MAX_HITS > 0);
	REQUIRE(API_HARD_MAX_HITS >= API_DEFAULT_MAX_HITS);
}
