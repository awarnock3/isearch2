// Tests for src/stopword.hxx / src/stopword.cxx (class STOPWORD).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// STOPWORD has no callers anywhere in the current tree (see the
// file-level comment in stopword.cxx) -- it's unrelated to
// IDBOBJ::IsStopWord() despite the similar name.

#include "catch_amalgamated.hpp"

#include "defs.hxx"
#include "string.hxx"
#include "strlist.hxx"
#include "stopword.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {

struct TempFile {
	std::string Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_stopword_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
		close(fd);
		Path = tmpl;
	}

	~TempFile() {
		remove(Path.c_str());
	}
};

}  // namespace

TEST_CASE("STOPWORD constructed from a nonexistent file has no words", "[stopword]") {
	STOPWORD sw(STRING("/tmp/isearch2_test_stopword_does_not_exist"));
	REQUIRE(sw.IsStopWord("the") == GDT_FALSE);
}

TEST_CASE("STOPWORD::AddWords and IsStopWord round-trip", "[stopword]") {
	TempFile file("");
	STOPWORD sw(STRING(file.Path.c_str()));
	STRLIST words;
	words.AddEntry(STRING("the"));
	words.AddEntry(STRING("and"));
	REQUIRE(sw.AddWords(words) == 2);
	REQUIRE(sw.IsStopWord("the") == GDT_TRUE);
	REQUIRE(sw.IsStopWord("and") == GDT_TRUE);
	REQUIRE(sw.IsStopWord("dog") == GDT_FALSE);
}

TEST_CASE("STOPWORD::AddWords does not add a word already in the list", "[stopword]") {
	TempFile file("");
	STOPWORD sw(STRING(file.Path.c_str()));
	STRLIST first;
	first.AddEntry(STRING("the"));
	REQUIRE(sw.AddWords(first) == 1);
	STRLIST second;
	second.AddEntry(STRING("the"));
	second.AddEntry(STRING("and"));
	// "the" is already present, so only "and" is actually new.
	REQUIRE(sw.AddWords(second) == 1);
	REQUIRE(sw.IsStopWord("and") == GDT_TRUE);
}

TEST_CASE("STOPWORD::ImportFromTextFile does not hang or underflow on a blank line", "[stopword]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcstopwordcxx): a line with no
	// alphanumeric characters at all (a blank line here) used to drive
	// the trailing-trim loop's `strlen(WordBuffer)-1` into an unsigned
	// underflow once the line was fully trimmed to empty, indexing
	// WordBuffer[-1] forever. Confirmed via a standalone repro before
	// fixing; this regression test would hang (and, under ASan, report
	// a stack-buffer-underflow) without the fix.
	//
	// Uses two separate temp files deliberately: STOPWORD's own
	// persistence file (a flat binary fixed-record format) is unrelated
	// to the plain-text file ImportFromTextFile() reads from, and
	// pointing both at the same path would load the text content back
	// in as bogus binary word records.
	TempFile swfile("");
	TempFile textfile("\nthe\n\ndog\n");
	STOPWORD sw(STRING(swfile.Path.c_str()));
	REQUIRE(sw.ImportFromTextFile(STRING(textfile.Path.c_str())) == 2);
	REQUIRE(sw.IsStopWord("the") == GDT_TRUE);
	REQUIRE(sw.IsStopWord("dog") == GDT_TRUE);
}

TEST_CASE("STOPWORD::ImportFromTextFile does not hang on an all-punctuation line", "[stopword]") {
	TempFile swfile("");
	TempFile textfile("---\ncat\n");
	STOPWORD sw(STRING(swfile.Path.c_str()));
	REQUIRE(sw.ImportFromTextFile(STRING(textfile.Path.c_str())) == 1);
	REQUIRE(sw.IsStopWord("cat") == GDT_TRUE);
}

TEST_CASE("STOPWORD::ImportFromTextFile strips leading/trailing punctuation from a real word", "[stopword]") {
	// Also the BUGFIX #2 regression test: the leading-punctuation strip
	// below used to shift WordBuffer left via an overlapping strcpy();
	// confirmed via a real ASan strcpy-param-overlap report triggered
	// by this exact input before fixing (see
	// docs/BUG_CATALOG.md#srcstopwordcxx).
	TempFile swfile("");
	TempFile textfile("(hello)\n");
	STOPWORD sw(STRING(swfile.Path.c_str()));
	REQUIRE(sw.ImportFromTextFile(STRING(textfile.Path.c_str())) == 1);
	REQUIRE(sw.IsStopWord("hello") == GDT_TRUE);
}
