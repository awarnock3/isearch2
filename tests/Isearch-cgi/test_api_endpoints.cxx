// Tests for Isearch-cgi/api_endpoints.hxx / Isearch-cgi/api_endpoints.cxx.
// Linked against the real implementation via TEST_ENGINE_CGI_SRCS in the
// top-level Makefile, not reimplemented here.
//
// Every function under test writes directly to cout, so these tests use
// the same CoutCapture RAII helper as tests/Isearch-cgi/test_api_response.cxx.
// HandleDatabases() also needs a real directory on disk, built with the
// same mkstemp()/mkdtemp() RAII convention as tests/src/test_fpt.cxx.

#include "catch_amalgamated.hpp"

#include "api_endpoints.hxx"

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
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

// Real temp directory, cleaned up (including its contents) on scope exit.
struct TempDbDir {
	std::string path;

	TempDbDir() {
		char tmpl[] = "/tmp/isearch2_test_api_endpoints_XXXXXX";
		char* made = mkdtemp(tmpl);
		REQUIRE(made != nullptr);
		path = made;
	}

	void TouchFile(const char* name) {
		std::string full = path + "/" + name;
		FILE* f = fopen(full.c_str(), "w");
		REQUIRE(f != nullptr);
		fclose(f);
	}

	~TempDbDir() {
		DIR* dir = opendir(path.c_str());
		if (dir) {
			for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
				std::string name = entry->d_name;
				if (name == "." || name == "..") continue;
				remove((path + "/" + name).c_str());
			}
			closedir(dir);
		}
		rmdir(path.c_str());
	}
};

void ClearApiEnv() {
	unsetenv("ISEARCH_DB_PATH");
	unsetenv("ISEARCH_API_MAX_HITS");
	unsetenv("ISEARCH_API_ALLOWLIST");
}

}  // namespace

TEST_CASE("HandleHealth reports ok status and the API version", "[api_endpoints]") {
	CoutCapture cap;
	HandleHealth();
	std::string out = cap.str();
	REQUIRE(out.find("\"status\":\"ok\"") != std::string::npos);
	REQUIRE(out.find(std::string("\"version\":\"") + API_VERSION + "\"") != std::string::npos);
}

TEST_CASE("HandleCapabilities reports search types, operators, defaults, and limits", "[api_endpoints]") {
	ClearApiEnv();
	ApiConfig cfg = LoadApiConfig();
	CoutCapture cap;
	HandleCapabilities(cfg);
	std::string out = cap.str();
	REQUIRE(out.find("\"search_types\":[\"simple\",\"advanced\",\"boolean\"]") != std::string::npos);
	REQUIRE(out.find("\"operators\":[\"or\",\"and\",\"andnot\",\"near\"]") != std::string::npos);
	REQUIRE(out.find("\"max_hits_ceiling\":") != std::string::npos);
}

TEST_CASE("HandleDatabases reports a 501 problem when ISEARCH_DB_PATH is unset", "[api_endpoints]") {
	ClearApiEnv();
	ApiConfig cfg = LoadApiConfig();
	CoutCapture cap;
	HandleDatabases(cfg);
	std::string out = cap.str();
	REQUIRE(out.find("Status: 501") != std::string::npos);
	REQUIRE(out.find("databases-not-configured") != std::string::npos);
}

TEST_CASE("HandleDatabases reports a 500 problem when the configured path can't be opened", "[api_endpoints]") {
	ClearApiEnv();
	setenv("ISEARCH_DB_PATH", "/nonexistent/path/for/sure", 1);
	ApiConfig cfg = LoadApiConfig();
	CoutCapture cap;
	HandleDatabases(cfg);
	std::string out = cap.str();
	REQUIRE(out.find("Status: 500") != std::string::npos);
	REQUIRE(out.find("databases-open-failed") != std::string::npos);
	ClearApiEnv();
}

TEST_CASE("HandleDatabases lists only .mdt-suffixed stems, excluding a bare '.mdt' dotfile", "[api_endpoints]") {
	TempDbDir dir;
	dir.TouchFile("mydb.mdt");
	dir.TouchFile("otherdb.mdt");
	dir.TouchFile("notadb.txt");
	dir.TouchFile(".mdt");

	ClearApiEnv();
	setenv("ISEARCH_DB_PATH", dir.path.c_str(), 1);
	ApiConfig cfg = LoadApiConfig();
	CoutCapture cap;
	HandleDatabases(cfg);
	std::string out = cap.str();
	REQUIRE(out.find("Status: 200") != std::string::npos);
	REQUIRE(out.find("\"mydb\"") != std::string::npos);
	REQUIRE(out.find("\"otherdb\"") != std::string::npos);
	REQUIRE(out.find("notadb") == std::string::npos);
	ClearApiEnv();
}

TEST_CASE("HandleDatabases reports an empty list for a database-free directory", "[api_endpoints]") {
	TempDbDir dir;
	ClearApiEnv();
	setenv("ISEARCH_DB_PATH", dir.path.c_str(), 1);
	ApiConfig cfg = LoadApiConfig();
	CoutCapture cap;
	HandleDatabases(cfg);
	REQUIRE(cap.str().find("\"databases\":[]") != std::string::npos);
	ClearApiEnv();
}
