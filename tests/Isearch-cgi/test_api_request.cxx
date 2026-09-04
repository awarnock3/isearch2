// Tests for Isearch-cgi/api_request.hxx / Isearch-cgi/api_request.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_SRCS in the
// top-level Makefile, not reimplemented here.
//
// ParseRequest()'s GET path reads CGIAPP, which itself reads
// REQUEST_METHOD/QUERY_STRING from the environment (see
// tests/Isearch-cgi/test_cgi-util.cxx) -- each GET test sets those
// directly. The POST path is exercised by passing `method`/`body`
// explicitly to ParseRequest(), sidestepping stdin entirely.

#include "catch_amalgamated.hpp"

#include "api_config.hxx"
#include "api_request.hxx"

#include <cstdlib>

namespace {

CGIAPP MakeGetCgi(const char* query_string) {
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("QUERY_STRING", query_string, 1);
	return CGIAPP();
}

}  // namespace

TEST_CASE("ParseRequest accepts a minimal valid GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.database == "mydb");
	REQUIRE(req.terms.size() == 1);
	REQUIRE(req.terms[0].term == "whale");
	REQUIRE(req.request_id.GetLength() > 0);
}

TEST_CASE("ParseRequest rejects a GET request with no database", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("term=whale");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(error_detail.GetLength() > 0);
}

TEST_CASE("ParseRequest rejects a GET request with neither q nor terms", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest collects multiple TERM_N/FIELD_N/WEIGHT_N/PHRASE_N entries from a GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi(
		"database=mydb&TERM_1=whale&FIELD_1=TITLE&WEIGHT_1=2"
		"&TERM_2=dolphin&PHRASE_2=yes");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.terms.size() == 2);
	REQUIRE(req.terms[0].term == "whale");
	REQUIRE(req.terms[0].field == "TITLE");
	REQUIRE(req.terms[0].weight == "2");
	REQUIRE(req.terms[1].term == "dolphin");
	REQUIRE(req.terms[1].phrase == true);
}

TEST_CASE("ParseRequest rejects an unrecognized search_type on a GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&search_type=quantum");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest rejects an unrecognized operator on a GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&operator=xor");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest rejects a max_hits value that overflows INT instead of truncating it", "[api_request]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_requesthxx): confirmed
	// via a standalone repro before this fix that max_hits=4294967297
	// (2^32+1) truncated to max_hits=1 and silently passed validation.
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&max_hits=4294967297");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest rejects a start value that overflows INT", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&start=99999999999");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest accepts a minimal valid POST/JSON request", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"q\":\"whale\"}";
	REQUIRE(ParseRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.database == "mydb");
	REQUIRE(req.q == "whale");
}

TEST_CASE("ParseRequest rejects a POST request with a non-JSON Content-Type", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "text/plain", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"q\":\"whale\"}";
	REQUIRE_FALSE(ParseRequest(&cgi, "POST", body, req, error_detail));
	setenv("CONTENT_TYPE", "application/json", 1);
}

TEST_CASE("ParseRequest rejects malformed JSON", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\", this is not json}";
	REQUIRE_FALSE(ParseRequest(&cgi, "POST", body, req, error_detail));
}

TEST_CASE("ParseRequest parses a JSON terms array with mixed string/numeric weight", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body =
		"{\"database\":\"mydb\",\"terms\":["
		"{\"term\":\"whale\",\"field\":\"TITLE\",\"weight\":2},"
		"{\"term\":\"dolphin\",\"phrase\":true}"
		"]}";
	REQUIRE(ParseRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.terms.size() == 2);
	REQUIRE(req.terms[0].term == "whale");
	REQUIRE(req.terms[0].field == "TITLE");
	REQUIRE(req.terms[0].weight == "2");
	REQUIRE(req.terms[1].term == "dolphin");
	REQUIRE(req.terms[1].phrase == true);
}

TEST_CASE("ParseRequest decodes a \\u00XX JSON escape to the matching Latin-1 byte", "[api_request]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgiapi_requesthxx): confirmed
	// via a standalone repro before this fix that é (e-acute) was
	// silently replaced with a literal '?' instead of being decoded.
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"q\":\"caf\\u00e9\"}";
	REQUIRE(ParseRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.q.GetLength() == 4);
	REQUIRE((unsigned char)req.q.GetChr(4) == 0xe9);
}

TEST_CASE("ParseRequest rejects a \\uXXXX escape above 0xFF instead of silently substituting a placeholder", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body = "{\"database\":\"mydb\",\"q\":\"snow\\u2603man\"}";
	REQUIRE_FALSE(ParseRequest(&cgi, "POST", body, req, error_detail));
}

TEST_CASE("ParseRequest rejects an unsupported HTTP method", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, "DELETE", "", req, error_detail));
}

TEST_CASE("ApiRequest/ApiTerm default-construct to documented defaults", "[api_request]") {
	ApiRequest req;
	REQUIRE(req.search_type == SEARCH_SIMPLE);
	REQUIRE(req.op == OP_OR);
	REQUIRE(req.element_set == "B");
	REQUIRE(req.start == 1);
	REQUIRE(req.max_hits == API_DEFAULT_MAX_HITS);
	REQUIRE(req.include_url == true);
	REQUIRE(req.include_headline == true);
	REQUIRE(req.include_record_key == true);
	REQUIRE(req.score_scale == 100);
	REQUIRE(req.rpn == false);
	REQUIRE(req.infix == false);
	REQUIRE(req.and_mode == false);
	REQUIRE(req.synonyms == false);
	REQUIRE(req.byte_range == false);
	REQUIRE(req.has_start_doc == false);
	REQUIRE(req.has_end_doc == false);
	REQUIRE(req.has_rect == false);
	REQUIRE(req.doc_type_options.empty());
	REQUIRE(req.record_syntax == "HTML");

	ApiTerm term;
	REQUIRE(term.phrase == false);
}

TEST_CASE("ParseRequest rejects an unknown GET parameter name", "[api_request]") {
	// Confirms the known_params allowlist (and this turn's fix adding the
	// upstream-introduced rpn/infix/and_mode/etc. names to it -- see
	// docs/BUG_CATALOG.md#isearch-cgiapi_requesthxx) rejects real typos
	// rather than silently ignoring them.
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&max_hist=10");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(error_detail.GetLength() > 0);
}

TEST_CASE("ParseRequest accepts rpn/infix/and_mode/synonyms GET parameters added by upstream", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&rpn=false&infix=false&and_mode=true&synonyms=true");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.rpn == false);
	REQUIRE(req.infix == false);
	REQUIRE(req.and_mode == true);
	REQUIRE(req.synonyms == true);
}

TEST_CASE("ParseRequest rejects rpn and infix both set", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&rpn=true&infix=true");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest collects doc_type_option_N and OPTION_N from a GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&doc_type_option_1=foo&OPTION_2=bar");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.doc_type_options.size() == 2);
}

TEST_CASE("ParseRequest parses a valid rect of four comma-separated numbers on a GET request", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&rect=1.5,2.5,3.5,4.5");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.has_rect);
	REQUIRE(req.rect_north == Catch::Approx(1.5));
	REQUIRE(req.rect_south == Catch::Approx(2.5));
	REQUIRE(req.rect_west == Catch::Approx(3.5));
	REQUIRE(req.rect_east == Catch::Approx(4.5));
}

TEST_CASE("ParseRequest rejects a rect with the wrong number of values", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&rect=1,2,3");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest rejects end_doc less than start_doc", "[api_request]") {
	CGIAPP cgi = MakeGetCgi("database=mydb&term=whale&start_doc=5&end_doc=2");
	ApiRequest req;
	STRING error_detail;
	REQUIRE_FALSE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
}

TEST_CASE("ParseRequest falls back to a PATH_INFO-derived database when DATABASE is absent", "[api_request]") {
	setenv("PATH_INFO", "/v1/api/mydb/search", 1);
	CGIAPP cgi = MakeGetCgi("term=whale");
	ApiRequest req;
	STRING error_detail;
	REQUIRE(ParseRequest(&cgi, nullptr, nullptr, req, error_detail));
	REQUIRE(req.database == "mydb");
	unsetenv("PATH_INFO");
}

TEST_CASE("ParseRequest parses byte_range/doc_type_options/rpn from a JSON POST body", "[api_request]") {
	setenv("REQUEST_METHOD", "POST", 1);
	CGIAPP cgi;
	setenv("CONTENT_TYPE", "application/json", 1);
	ApiRequest req;
	STRING error_detail;
	const CHR* body =
		"{\"database\":\"mydb\",\"q\":\"whale\",\"rpn\":true,"
		"\"byte_range\":true,\"doc_type_options\":[\"foo\",\"bar\"]}";
	REQUIRE(ParseRequest(&cgi, "POST", body, req, error_detail));
	REQUIRE(req.rpn == true);
	REQUIRE(req.byte_range == true);
	REQUIRE(req.doc_type_options.size() == 2);
	REQUIRE(req.doc_type_options[0] == "foo");
	REQUIRE(req.doc_type_options[1] == "bar");
}
