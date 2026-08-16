// Tests for Isearch-cgi/api_response.hxx / Isearch-cgi/api_response.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_SRCS in the
// top-level Makefile, not reimplemented here.
//
// Every function under test writes directly to `cout` rather than
// returning a value, so these tests redirect cout's streambuf to an
// ostringstream for the duration of each TEST_CASE and inspect the
// captured text. g_request_id/g_request_counter (api_response.cxx) are
// file-static globals that persist across TEST_CASEs within this same
// binary, so tests check for the *presence* of a non-empty request id
// rather than any particular value.

#include "catch_amalgamated.hpp"

#include "api_response.hxx"

#include <iostream>
#include <sstream>
#include <string>

namespace {

struct CoutCapture {
	std::ostringstream stream;
	std::streambuf* saved;
	CoutCapture() : saved(std::cout.rdbuf(stream.rdbuf())) {}
	~CoutCapture() { std::cout.rdbuf(saved); }
	std::string str() const { return stream.str(); }
};

}  // namespace

TEST_CASE("WriteHttpHeader emits the expected status line and JSON content type", "[api_response]") {
	CoutCapture cap;
	WriteHttpHeader(200, false);
	std::string out = cap.str();
	REQUIRE(out.find("Status: 200 OK\n") != std::string::npos);
	REQUIRE(out.find("Content-Type: application/json; charset=utf-8\n") != std::string::npos);
	REQUIRE(out.find("X-Request-Id: ") != std::string::npos);
	REQUIRE(out.find("\n\n") != std::string::npos);
}

TEST_CASE("WriteHttpHeader emits problem+json content type when requested", "[api_response]") {
	CoutCapture cap;
	WriteHttpHeader(400, true);
	std::string out = cap.str();
	REQUIRE(out.find("Status: 400 Bad Request\n") != std::string::npos);
	REQUIRE(out.find("Content-Type: application/problem+json; charset=utf-8\n") != std::string::npos);
}

TEST_CASE("WriteHttpHeader reports the correct status text for every recognized code", "[api_response]") {
	CoutCapture cap;
	WriteHttpHeader(404, false);
	REQUIRE(cap.str().find("Status: 404 Not Found\n") != std::string::npos);
}

TEST_CASE("WriteHttpHeader falls back to Unknown for an unrecognized status", "[api_response]") {
	CoutCapture cap;
	WriteHttpHeader(999, false);
	REQUIRE(cap.str().find("Status: 999 Unknown\n") != std::string::npos);
}

TEST_CASE("WriteJsonEscaped escapes quotes, backslashes, and control characters", "[api_response]") {
	CoutCapture cap;
	STRING s = "line1\nline2\t\"quoted\"\\backslash";
	WriteJsonEscaped(s);
	std::string out = cap.str();
	REQUIRE(out == "\"line1\\nline2\\t\\\"quoted\\\"\\\\backslash\"");
}

TEST_CASE("WriteJsonEscaped escapes low control characters as \\u00XX", "[api_response]") {
	CoutCapture cap;
	CHR raw[2] = { (CHR)0x01, '\0' };
	STRING s = raw;
	WriteJsonEscaped(s);
	REQUIRE(cap.str() == "\"\\u0001\"");
}

TEST_CASE("BeginSearchResponse uses the caller-supplied request_id when given", "[api_response]") {
	CoutCapture cap;
	ApiSearchMeta meta;
	meta.request_id = "fixed-id-123";
	meta.database = "mydb";
	meta.search_type = "SIMPLE";
	meta.matching_record_count = 5;
	meta.total_retrieved = 5;
	meta.interpreted_query = "whale";
	meta.total_database_records = 100;
	meta.query_time_seconds = 0.02;
	meta.start = 1;
	meta.max_hits = 50;
	BeginSearchResponse(meta);
	std::string out = cap.str();
	REQUIRE(out.find("\"request_id\":\"fixed-id-123\"") != std::string::npos);
	REQUIRE(out.find("\"database\":\"mydb\"") != std::string::npos);
	REQUIRE(out.find("\"matching_record_count\":5") != std::string::npos);
	REQUIRE(out.find("\"results\":[") != std::string::npos);
}

TEST_CASE("WriteSearchHit separates multiple hits with a comma but not the first", "[api_response]") {
	CoutCapture cap;
	ApiHit hit;
	hit.score = 87;
	hit.filename = "doc.txt";
	hit.headline = "A Document";
	hit.record_key = "42";
	WriteSearchHit(1, hit, true);
	WriteSearchHit(2, hit, false);
	std::string out = cap.str();
	REQUIRE(out.substr(0, 1) != ",");
	REQUIRE(out.find(",{\"match_number\":2") != std::string::npos);
}

TEST_CASE("WriteSearchHit emits a null url when the hit has none", "[api_response]") {
	CoutCapture cap;
	ApiHit hit;
	hit.filename = "doc.txt";
	WriteSearchHit(1, hit, true);
	REQUIRE(cap.str().find("\"url\":null") != std::string::npos);
}

TEST_CASE("WriteSearchHit escapes a real url when present", "[api_response]") {
	CoutCapture cap;
	ApiHit hit;
	hit.url = "http://example.com/doc";
	WriteSearchHit(1, hit, true);
	REQUIRE(cap.str().find("\"url\":\"http://example.com/doc\"") != std::string::npos);
}

TEST_CASE("WriteSearchHit omits record_start/record_end when has_byte_range is false", "[api_response]") {
	CoutCapture cap;
	ApiHit hit;
	hit.filename = "doc.txt";
	hit.record_start = 10;
	hit.record_end = 20;
	WriteSearchHit(1, hit, true);
	REQUIRE(cap.str().find("record_start") == std::string::npos);
	REQUIRE(cap.str().find("record_end") == std::string::npos);
}

TEST_CASE("WriteSearchHit includes record_start/record_end when has_byte_range is true", "[api_response]") {
	CoutCapture cap;
	ApiHit hit;
	hit.filename = "doc.txt";
	hit.has_byte_range = true;
	hit.record_start = 10;
	hit.record_end = 20;
	WriteSearchHit(1, hit, true);
	std::string out = cap.str();
	REQUIRE(out.find("\"record_start\":10") != std::string::npos);
	REQUIRE(out.find("\"record_end\":20") != std::string::npos);
}

TEST_CASE("EndSearchResponse emits null links when empty, real ones when set", "[api_response]") {
	{
		CoutCapture cap;
		ApiLinks links;
		EndSearchResponse(links);
		std::string out = cap.str();
		REQUIRE(out.find("\"next\":null") != std::string::npos);
		REQUIRE(out.find("\"prev\":null") != std::string::npos);
	}
	{
		CoutCapture cap;
		ApiLinks links;
		links.next = "/search?start=51";
		EndSearchResponse(links);
		REQUIRE(cap.str().find("\"next\":\"/search?start=51\"") != std::string::npos);
	}
}

TEST_CASE("WriteProblem falls back to about:blank / status text / no detail when given nullptr", "[api_response]") {
	CoutCapture cap;
	WriteProblem(500, nullptr, nullptr, nullptr);
	std::string out = cap.str();
	REQUIRE(out.find("\"type\":\"about:blank\"") != std::string::npos);
	REQUIRE(out.find("\"title\":\"Internal Server Error\"") != std::string::npos);
	REQUIRE(out.find("\"status\":500") != std::string::npos);
	REQUIRE(out.find("\"detail\"") == std::string::npos);
}

TEST_CASE("WriteProblem includes detail when given", "[api_response]") {
	CoutCapture cap;
	WriteProblem(400, "https://example.com/errors/bad-query", "Bad Query", "missing ISEARCH_TERM");
	std::string out = cap.str();
	REQUIRE(out.find("\"type\":\"https://example.com/errors/bad-query\"") != std::string::npos);
	REQUIRE(out.find("\"title\":\"Bad Query\"") != std::string::npos);
	REQUIRE(out.find("\"detail\":\"missing ISEARCH_TERM\"") != std::string::npos);
}

TEST_CASE("ApiSearchMeta/ApiHit/ApiLinks default-construct to sane, empty values", "[api_response]") {
	ApiSearchMeta meta;
	REQUIRE(meta.matching_record_count == 0);
	REQUIRE(meta.total_retrieved == 0);
	REQUIRE(meta.total_database_records == 0);
	REQUIRE(meta.start == 1);
	REQUIRE(meta.max_hits == 0);

	ApiHit hit;
	REQUIRE(hit.score == 0);
	REQUIRE(hit.filename.GetLength() == 0);
	REQUIRE(hit.has_byte_range == false);
	REQUIRE(hit.record_start == 0);
	REQUIRE(hit.record_end == 0);

	ApiLinks links;
	REQUIRE(links.next.GetLength() == 0);
	REQUIRE(links.prev.GetLength() == 0);
}
