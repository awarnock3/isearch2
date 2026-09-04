// Tests for Isearch-cgi/api_fetch.hxx / Isearch-cgi/api_fetch.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_SRCS in the
// top-level Makefile, not reimplemented here.
//
// ParseFetchRequest()'s GET path reads CGIAPP, which itself reads
// REQUEST_METHOD/QUERY_STRING from the environment (see
// tests/Isearch-cgi/test_cgi-util.cxx) -- each GET test sets those
// directly, and every CGIAPP construction sets REQUEST_METHOD explicitly
// first (CGIAPP::GetInput() calls exit(1) if it isn't "GET" or "POST",
// and env vars persist process-wide across TEST_CASEs -- see the same
// fix applied to tests/Isearch-cgi/test_api_request.cxx). The POST path
// is exercised by passing method/body directly to ParseFetchRequest(),
// sidestepping stdin entirely.
//
// ExecuteFetch()'s successful-fetch path needs a real Iindex-built
// database -- same scope note as tests/Isearch-cgi/test_api_search.cxx --
// so it was instead verified with a live repro against the real isrch_api
// binary before this turn was committed (see
// docs/BUG_CATALOG.md#isearch-cgiapi_fetchhxx-isearch-cgiapi_fetchcxx),
// not reproduced as a permanent test here.

#include "catch_amalgamated.hpp"

#include "api_fetch.hxx"

#include <cstdlib>
#include <cstdio>

namespace {

CGIAPP MakeGetCgi(const char* query_string) {
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("QUERY_STRING", query_string, 1);
	return CGIAPP();
}

}  // namespace

TEST_CASE("ParseFetchRequest rejects a GET request with no database", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("record_key=42");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
	REQUIRE(error_detail.GetLength() > 0);
}

TEST_CASE("ParseFetchRequest rejects a GET request with no record_key", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("database=mydb");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
}

TEST_CASE("ParseFetchRequest rejects an unknown GET parameter", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&record_key=42&bogus=1");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
	REQUIRE(error_detail.GetLength() > 0);
}

TEST_CASE("ParseFetchRequest rejects an unsupported record_syntax on a GET request", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&record_key=42&record_syntax=XML");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
}

TEST_CASE("ParseFetchRequest defaults element_set to F and record_syntax to SUTRS", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&record_key=42");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
	REQUIRE(req.element_set == "F");
	REQUIRE(req.record_syntax == "SUTRS");
}

TEST_CASE("ParseFetchRequest accepts an explicit record_syntax on a GET request", "[api_fetch]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&record_key=42&record_syntax=HTML&element_set=B");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
	REQUIRE(req.record_syntax == "HTML");
	REQUIRE(req.element_set == "B");
}

TEST_CASE("ParseFetchRequest prefers ISEARCH_API_DB_FROM_PATH over an explicit database param", "[api_fetch]") {
	// isrch_api.cxx sets this env var when the database came from
	// PATH_INFO-based routing (/v1/api/{database}/fetch); it takes
	// priority over an explicit ?database= query param, matching the
	// same precedence already established for isrch_srch.cxx.
	setenv("ISEARCH_API_DB_FROM_PATH", "pathdb", 1);
	CGIAPP cgi = MakeGetCgi("database=querydb&record_key=42");
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE(ParseFetchRequest(&cgi, "GET", nullptr, req, error_detail));
	REQUIRE(req.database == "pathdb");
	unsetenv("ISEARCH_API_DB_FROM_PATH");
}

TEST_CASE("ParseFetchRequest accepts a minimal valid POST/JSON request", "[api_fetch]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiFetchRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"record_key\":\"42\"}";
	REQUIRE(ParseFetchRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.database == "mydb");
	REQUIRE(req.record_key == "42");
}

TEST_CASE("ParseFetchRequest rejects a POST request with an empty body", "[api_fetch]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiFetchRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseFetchRequest(&cgi, "POST", "", req, error_detail));
}

TEST_CASE("ParseFetchRequest decodes a \\u00XX JSON escape in a POST body to the matching Latin-1 byte", "[api_fetch]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_fetchhxx-isearch-cgiapi_fetchcxx):
	// confirmed via a live repro before this fix that GetJsonField() had
	// no \u handling at all and fell into its default case, corrupting
	// "10" (meant to decode to "10") into the literal garbage
	// text "u0031u0030" instead.
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiFetchRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"record_key\":\"\\u0031\\u0030\"}";
	REQUIRE(ParseFetchRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.record_key == "10");
}

TEST_CASE("ParseFetchRequest silently drops a \\uXXXX escape above 0xFF instead of corrupting the string", "[api_fetch]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiFetchRequest req;
	STRING error_detail;
	// ☃ is a snowman (above 0xFF, no lossless Latin-1 representation).
	const CHR* body = "{\"database\":\"mydb\",\"record_key\":\"1\\u26030\"}";
	REQUIRE(ParseFetchRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.record_key == "10");
}

TEST_CASE("ParseFetchRequest decodes standard JSON escapes (quote, backslash, newline, tab) in a POST body", "[api_fetch]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiFetchRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"record_key\":\"a\\\"b\\\\c\\nd\\te\"}";
	REQUIRE(ParseFetchRequest(&cgi, "POST", body, req, error_detail));
	STRING expected = "a\"b\\c";
	expected.Cat('\n');
	expected.Cat('d');
	expected.Cat('\t');
	expected.Cat('e');
	REQUIRE(req.record_key == expected);
}

TEST_CASE("ExecuteFetch reports 404 for a database that doesn't exist", "[api_fetch]") {
	ApiConfig cfg;
	cfg.db_path = "/tmp";
	ApiFetchRequest req;
	req.database = "isearch2_test_api_fetch_definitely_does_not_exist";
	req.record_key = "42";

	ApiFetchResult result;
	STRING error_detail;
	int status = ExecuteFetch(req, cfg, result, error_detail);

	REQUIRE(status == 404);
	REQUIRE(error_detail.GetLength() > 0);
}

TEST_CASE("ExecuteFetch falls back to '.' when ApiConfig.db_path is empty", "[api_fetch]") {
	ApiConfig cfg;  // db_path left default-constructed (empty)
	ApiFetchRequest req;
	req.database = "isearch2_test_api_fetch_definitely_does_not_exist";
	req.record_key = "42";

	ApiFetchResult result;
	STRING error_detail;
	int status = ExecuteFetch(req, cfg, result, error_detail);

	REQUIRE(status == 404);
	remove("./isearch2_test_api_fetch_definitely_does_not_exist.mdt");
	remove(".isearch2_test_api_fetch_definitely_does_not_exist.mdt");
}
