// Tests for src/sw.hxx (the `stoplist[]` data array).
//
// sw.hxx defines `stoplist` with external linkage (see the file-level
// comment in sw.hxx), so it can only safely be #include'd by exactly
// one .cxx file in a given link -- src/index.cxx already does, and
// index.cxx is already linked into this test binary via
// TEST_ENGINE_SRCS. #include'ing sw.hxx a second time here would be a
// one-definition-rule violation (duplicate-symbol link error), so
// instead this test re-parses src/sw.hxx's own source text from disk
// and checks the data directly: this is the most direct way to test
// the exact bug that was fixed (an accidental array entry) without
// touching index.cxx (already processed in an earlier turn) or
// changing sw.hxx's linkage.
//
// index.cxx's INDEX::IsStopWord() itself -- the code that actually
// binary-searches this array -- is exercised behaviorally by
// tests/src/test_index.cxx.

#include "catch_amalgamated.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

namespace {

// Extracts every double-quoted string literal appearing between
// "stoplist[] = {" and the closing "};" in src/sw.hxx. Deliberately
// simple (no escape handling beyond \") since the file itself only
// ever contains plain lowercase words and apostrophe contractions.
std::vector<std::string> ParseStopList() {
	std::ifstream in("src/sw.hxx");
	REQUIRE(in.is_open());
	std::stringstream buf;
	buf << in.rdbuf();
	std::string content = buf.str();

	size_t start = content.find("stoplist[] = {");
	REQUIRE(start != std::string::npos);
	size_t end = content.find("};", start);
	REQUIRE(end != std::string::npos);
	std::string body = content.substr(start, end - start);

	std::vector<std::string> words;
	size_t i = 0;
	while (i < body.size()) {
		if (body[i] == '"') {
			size_t j = i + 1;
			std::string word;
			while (j < body.size() && body[j] != '"') {
				if (body[j] == '\\' && j + 1 < body.size()) {
					word += body[j + 1];
					j += 2;
				} else {
					word += body[j];
					j++;
				}
			}
			words.push_back(word);
			i = j + 1;
		} else {
			i++;
		}
	}
	return words;
}

std::string ToLower(const std::string& s) {
	std::string out = s;
	for (char& c : out) {
		c = (char)std::tolower((unsigned char)c);
	}
	return out;
}

}  // namespace

TEST_CASE("sw.hxx's stoplist has no empty entries", "[sw]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcswhxx): a stray `"found", "",`
	// inserted an accidental empty-string entry into the array.
	std::vector<std::string> words = ParseStopList();
	REQUIRE(words.size() > 0);
	for (const auto& w : words) {
		REQUIRE(w != "");
	}
}

TEST_CASE("sw.hxx's stoplist is sorted case-insensitively (StrCaseCmp order)", "[sw]") {
	// index.cxx's INDEX::IsStopWord() binary-searches this array via
	// StrCaseCmp() (plain strcasecmp() on UNIX), so it must stay fully
	// sorted under that exact ordering or the search can silently miss
	// entries. This is the BUGFIX #1 regression test: the stray ""
	// entry sorted out of place (before "found", not after), and this
	// check would have caught it directly.
	std::vector<std::string> words = ParseStopList();
	REQUIRE(words.size() > 1);
	for (size_t i = 1; i < words.size(); i++) {
		INFO("comparing \"" << words[i-1] << "\" and \"" << words[i] << "\"");
		REQUIRE(ToLower(words[i-1]) <= ToLower(words[i]));
	}
}

TEST_CASE("sw.hxx's stoplist contains common English stop words", "[sw]") {
	std::vector<std::string> words = ParseStopList();
	auto contains = [&](const char* w) {
		for (const auto& s : words) {
			if (s == w) return true;
		}
		return false;
	};
	REQUIRE(contains("the"));
	REQUIRE(contains("and"));
	REQUIRE(contains("a"));
	REQUIRE(contains("z"));
}
