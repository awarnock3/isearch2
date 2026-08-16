// Tests for Isearch-cgi/cgi-util.hxx / Isearch-cgi/cgi-util.cxx
// (class CGIAPP and the plustospace/unescape_url/escape_url/
// spacetoplus/x2c helpers). Linked against the real implementation via
// TEST_ENGINE_CGI_SRCS in the top-level Makefile, not reimplemented
// here.
//
// CGIAPP::GetInput() (called from the constructor) reads directly from
// getenv() and cin -- real process/global state, not anything
// injectable through the constructor -- so these tests set the
// relevant environment variables with setenv() and, for POST, swap
// cin's streambuf, before constructing a CGIAPP. This means these
// tests are NOT safe to run in parallel with each other if this
// binary is ever split up; Catch2 runs them serially by default.
//
// CGIAPP::GetInput() calls exit(1) if REQUEST_METHOD isn't set to
// exactly "GET" or "POST", so every test case sets it first.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "cgi-util.hxx"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <string>

namespace {

// RAII helper: swaps cin's streambuf to read from a string, restoring
// the original on destruction.
struct StdinRedirect {
	std::istringstream stream;
	std::streambuf* saved;
	explicit StdinRedirect(const std::string& body)
	    : stream(body), saved(std::cin.rdbuf(stream.rdbuf())) {}
	~StdinRedirect() { std::cin.rdbuf(saved); }
};

}  // namespace

TEST_CASE("CGIAPP parses a normal GET query string", "[cgi-util]") {
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("QUERY_STRING", "a=1&b=hello+world", 1);
	CGIAPP app;
	REQUIRE(STRING(app.GetValueByName("a")) == "1");
	REQUIRE(STRING(app.GetValueByName("b")) == "hello world");
	REQUIRE(app.GetValueByName("nonexistent") == nullptr);
}

TEST_CASE("CGIAPP handles a GET request with no QUERY_STRING at all", "[cgi-util]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx): query
	// used to be pointed at a string literal "" in this case, and
	// unescape_url()/plustospace() write through their argument
	// unconditionally -- a write to read-only memory that crashed
	// (SEGV) on a typical modern OS. Confirmed with a standalone ASan
	// repro before fixing. This test just needs to construct a CGIAPP
	// without crashing; make tests-asan is what actually verifies the
	// fix, same as the repro did.
	setenv("REQUEST_METHOD", "GET", 1);
	unsetenv("QUERY_STRING");
	CGIAPP app;
	REQUIRE(app.GetValueByName("anything") == nullptr);
}

TEST_CASE("CGIAPP truncates (not overflows) an overlong GET field", "[cgi-util]") {
	// BUGFIX #2: the GET-branch copy loops had no bound on the write
	// index at all -- QUERY_STRING is fully attacker-controlled with no
	// length limit. Confirmed a real stack-buffer-overflow with a
	// standalone ASan repro (a 400-byte field name) before fixing.
	setenv("REQUEST_METHOD", "GET", 1);
	std::string longName(400, 'A');
	std::string qs = longName + "=x";
	setenv("QUERY_STRING", qs.c_str(), 1);
	CGIAPP app;
	// Should not have crashed; the truncated name (255 chars, the most
	// that fits in the 256-byte internal buffer minus the terminator)
	// won't match the full 400-char name.
	REQUIRE(app.GetValueByName(longName.c_str()) == nullptr);
}

TEST_CASE("CGIAPP parses a normal POST body", "[cgi-util]") {
	setenv("REQUEST_METHOD", "POST", 1);
	std::string body = "a=1&b=hello+world";
	setenv("CONTENT_LENGTH", std::to_string(body.size()).c_str(), 1);
	StdinRedirect redirect(body);
	CGIAPP app;
	REQUIRE(STRING(app.GetValueByName("a")) == "1");
	REQUIRE(STRING(app.GetValueByName("b")) == "hello world");
}

TEST_CASE("CGIAPP does not overflow on an overlong POST field with no '&'", "[cgi-util]") {
	// BUGFIX #1: this told getline the buffer was ContentLen+1 bytes
	// (attacker-controlled via Content-Length) when the real buffer is
	// a fixed 256-byte stack array. Confirmed a real stack-buffer-
	// overflow with a standalone ASan repro (a 400-byte POST body, no
	// '&') before fixing.
	setenv("REQUEST_METHOD", "POST", 1);
	std::string body = "a=" + std::string(400, 'B');
	setenv("CONTENT_LENGTH", std::to_string(body.size()).c_str(), 1);
	StdinRedirect redirect(body);
	CGIAPP app;
	// Should not have crashed; the field was truncated to fit.
	std::string value = app.GetValueByName("a") ? app.GetValueByName("a") : "";
	REQUIRE(value.size() <= 255);
	REQUIRE(value.size() > 0);
}

TEST_CASE("CGIAPP::GetValueByName rejects a null or empty field name", "[cgi-util]") {
	setenv("REQUEST_METHOD", "GET", 1);
	setenv("QUERY_STRING", "a=1", 1);
	CGIAPP app;
	REQUIRE(app.GetValueByName(nullptr) == nullptr);
	REQUIRE(app.GetValueByName("") == nullptr);
}

TEST_CASE("unescape_url decodes %XX escapes in place", "[cgi-util]") {
	char buf[] = "hello%20world%21";
	unescape_url(buf);
	REQUIRE(STRING(buf) == "hello world!");
}

TEST_CASE("unescape_url leaves a trailing incomplete escape alone", "[cgi-util]") {
	char buf[] = "abc%2";
	unescape_url(buf);
	REQUIRE(STRING(buf) == "abc%2");
}

TEST_CASE("plustospace and spacetoplus are inverses over +/space", "[cgi-util]") {
	char buf[] = "a+b+c";
	plustospace(buf);
	REQUIRE(STRING(buf) == "a b c");
	spacetoplus(buf);
	REQUIRE(STRING(buf) == "a+b+c");
}

TEST_CASE("x2c converts a two-hex-digit escape to its byte value", "[cgi-util]") {
	char hex1[] = "41";  // 'A'
	REQUIRE(x2c(hex1) == 'A');
	char hex2[] = "2f";  // '/' (lowercase hex digits)
	REQUIRE(x2c(hex2) == '/');
}

TEST_CASE("ExtractPathParam extracts segments by index", "[cgi-util]") {
	REQUIRE(ExtractPathParam("/mydb/search", 0) == "mydb");
	REQUIRE(ExtractPathParam("/mydb/search", 1) == "search");
	REQUIRE(ExtractPathParam("/mydb/search/extra", 2) == "extra");
}

TEST_CASE("ExtractPathParam returns empty for an out-of-range index", "[cgi-util]") {
	REQUIRE(ExtractPathParam("/mydb/search", 5).GetLength() == 0);
}

TEST_CASE("ExtractPathParam returns empty for a null or empty pathInfo", "[cgi-util]") {
	REQUIRE(ExtractPathParam(nullptr, 0).GetLength() == 0);
	REQUIRE(ExtractPathParam("", 0).GetLength() == 0);
	REQUIRE(ExtractPathParam("/", 0).GetLength() == 0);
}

TEST_CASE("ExtractPathParam works without a leading slash", "[cgi-util]") {
	REQUIRE(ExtractPathParam("mydb/search", 0) == "mydb");
	REQUIRE(ExtractPathParam("mydb/search", 1) == "search");
}

TEST_CASE("ExtractPathParam returns empty for a negative index", "[cgi-util]") {
	REQUIRE(ExtractPathParam("/mydb/search", -1).GetLength() == 0);
}
