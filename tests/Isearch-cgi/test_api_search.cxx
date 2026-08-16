// Tests for Isearch-cgi/api_search.hxx / Isearch-cgi/api_search.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_SRCS in the
// top-level Makefile, not reimplemented here.
//
// ExecuteSearch() is the only function this file exposes; everything
// else (SearchTypeName, BuildQueryFromTerms, BuildSquery, ...) is
// `static`. Its error/validation paths (empty query, nonexistent
// database) are real, reachable code that doesn't need a real index on
// disk, so they're covered here directly. A genuinely successful
// search with real hits needs a database actually built by
// bin/Iindex -- no existing test in this suite builds one
// programmatically (confirmed: no precedent anywhere in tests/), so
// that path was instead verified with a standalone repro against a
// real Iindex-built database before this turn was committed (see
// docs/BUG_CATALOG.md#isearch-cgiapi_searchhxx-isearch-cgiapi_searchcxx),
// not reproduced as a permanent test here.

#include "catch_amalgamated.hpp"

#include "api_search.hxx"

#include <cstdio>

TEST_CASE("ExecuteSearch rejects a whitespace-only query as 422 without touching any database", "[api_search]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "irrelevant-because-query-is-checked-first";
	req.q = "   ";

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(status == 422);
	REQUIRE(error_detail.GetLength() > 0);
	REQUIRE(hits.empty());
}

TEST_CASE("ExecuteSearch rejects an entirely empty request (no q, no terms) as 422", "[api_search]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "mydb";

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(status == 422);
}

TEST_CASE("ExecuteSearch reports 404 for a database that doesn't exist", "[api_search]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "isearch2_test_api_search_definitely_does_not_exist";
	req.q = "whale";

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(status == 404);
	REQUIRE(error_detail.GetLength() > 0);
	REQUIRE(hits.empty());
}

TEST_CASE("ExecuteSearch falls back to '.' when ApiConfig.db_path is empty", "[api_search]") {
	// Not a crash-worthy scenario either way (opendir/IDB open just fails
	// cleanly against whatever "." resolves to for the test binary's
	// cwd), but confirms the empty-db_path branch runs without incident.
	// This test is also what surfaced BUGFIX #7 in
	// docs/BUG_CATALOG.md#srccommoncxx: AddTrailingSlash(".") used to
	// leave "." with no separator at all, so this path used to
	// concatenate into ".<database>.mdt" (a hidden dotfile landing in
	// the test runner's own cwd -- the repo root -- as a side effect of
	// IDB probing for it) instead of "./<database>.mdt". The explicit
	// remove() below is a defensive no-op now that the underlying bug
	// is fixed (nothing gets created against the correctly-joined path
	// for a nonexistent database), kept in case that ever regresses.
	ApiConfig cfg;  // db_path left default-constructed (empty)
	ApiRequest req;
	req.database = "isearch2_test_api_search_definitely_does_not_exist";
	req.q = "whale";

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(status == 404);
	remove("./isearch2_test_api_search_definitely_does_not_exist.mdt");
	remove(".isearch2_test_api_search_definitely_does_not_exist.mdt");
}

TEST_CASE("ExecuteSearch builds an interpreted query from terms when q is empty", "[api_search]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "isearch2_test_api_search_definitely_does_not_exist";
	ApiTerm term;
	term.term = "whale";
	term.field = "TITLE";
	req.terms.push_back(term);

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	// Fails at the (nonexistent) database open, not at query-building --
	// confirms BuildQueryFromTerms() ran and produced a non-empty query
	// (an empty one would have failed with 422 "Empty query" instead).
	REQUIRE(status == 404);
	REQUIRE(meta.interpreted_query == "TITLE/whale");
}

TEST_CASE("ExecuteSearch accepts a query already in RPN form when rpn is set", "[api_search]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_searchhxx-isearch-cgiapi_searchcxx):
	// confirmed via a live repro before this fix that a query already in
	// valid RPN (postfix) form -- exactly what req.rpn=true declares it
	// to be -- was run through INFIX2RPN::Parse() (which expects INFIX
	// notation) anyway, and got rejected outright with a 422 "query was
	// unparseable" even though it was perfectly valid RPN. A nonexistent
	// database isolates BuildSquery()'s own success from the database-
	// open step: 404 (not 422) proves the RPN query was accepted.
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "isearch2_test_api_search_definitely_does_not_exist";
	req.q = "whale dolphin AND";
	req.rpn = true;

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	int status = ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(status == 404);
	REQUIRE(meta.interpreted_query == "whale dolphin AND");
}

TEST_CASE("ExecuteSearch populates ApiSearchMeta's request-independent fields before validation can fail", "[api_search]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiRequest req;
	req.database = "mydb";
	req.request_id = "fixed-id";
	req.start = 3;
	req.max_hits = 25;
	req.search_type = SEARCH_ADVANCED;
	// q left empty and terms left empty -> 422, but meta should already
	// reflect the request before that check runs.

	ApiSearchMeta meta;
	std::vector<ApiHit> hits;
	STRING error_detail;
	ExecuteSearch(req, cfg, meta, hits, error_detail);

	REQUIRE(meta.request_id == "fixed-id");
	REQUIRE(meta.database == "mydb");
	REQUIRE(meta.search_type == "advanced");
	REQUIRE(meta.start == 3);
	REQUIRE(meta.max_hits == 25);
}
