// Tests for src/strlist.hxx / src/strlist.cxx (class STRLIST).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "strlist.hxx"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

TEST_CASE("STRLIST AddEntry/GetEntry/GetTotalEntries round-trip", "[strlist]") {
	STRLIST list;
	list.AddEntry("one");
	list.AddEntry(STRING("two"));
	REQUIRE(list.GetTotalEntries() == 2);
	STRING s;
	list.GetEntry(1, &s);
	REQUIRE(s == "one");
	list.GetEntry(2, &s);
	REQUIRE(s == "two");
}

TEST_CASE("STRLIST SetEntry overwrites an existing entry", "[strlist]") {
	STRLIST list;
	list.AddEntry("one");
	list.SetEntry(1, STRING("replaced"));
	STRING s;
	list.GetEntry(1, &s);
	REQUIRE(s == "replaced");
}

TEST_CASE("STRLIST SetEntry past the end appends filler entries", "[strlist]") {
	STRLIST list;
	list.SetEntry(3, STRING("third"));
	REQUIRE(list.GetTotalEntries() == 3);
	STRING s;
	list.GetEntry(3, &s);
	REQUIRE(s == "third");
}

TEST_CASE("STRLIST operator= deep-copies entries", "[strlist]") {
	STRLIST a;
	a.AddEntry("x");
	a.AddEntry("y");
	STRLIST b;
	b = a;
	REQUIRE(b.GetTotalEntries() == 2);
	a.AddEntry("z");
	// b must be independent of a's later mutation (a real copy, not
	// aliasing the same nodes).
	REQUIRE(b.GetTotalEntries() == 2);
}

TEST_CASE("STRLIST operator= self-assignment leaves the list intact", "[strlist]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcstrlistcxx): `list = list;`
	// used to Clear() the list before reading its own (now-empty) entry
	// count, silently wiping it.
	STRLIST list;
	list.AddEntry("keep-me");
	list.AddEntry("and-me");
	list = list;
	REQUIRE(list.GetTotalEntries() == 2);
	STRING s;
	list.GetEntry(1, &s);
	REQUIRE(s == "keep-me");
	list.GetEntry(2, &s);
	REQUIRE(s == "and-me");
}

TEST_CASE("STRLIST Split(CHR*) splits on a multi-character separator", "[strlist]") {
	STRLIST list;
	list.Split("::", STRING("a::b::c"));
	REQUIRE(list.GetTotalEntries() == 3);
	STRING s;
	list.GetEntry(1, &s);
	REQUIRE(s == "a");
	list.GetEntry(2, &s);
	REQUIRE(s == "b");
	list.GetEntry(3, &s);
	REQUIRE(s == "c");
}

TEST_CASE("STRLIST Split(CHR*) drops a trailing empty segment", "[strlist]") {
	STRLIST list;
	list.Split(",", STRING("a,b,"));
	REQUIRE(list.GetTotalEntries() == 2);
}

TEST_CASE("STRLIST Split(CHR) drops a trailing empty segment too", "[strlist]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srcstrlistcxx): this overload
	// used to unconditionally add the remainder, producing a trailing
	// empty entry the CHR*-separator overload above didn't.
	STRLIST list;
	list.Split(',', STRING("a,b,"));
	REQUIRE(list.GetTotalEntries() == 2);
}

TEST_CASE("STRLIST Split(CHR*) with an empty separator terminates", "[strlist]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcstrlistcxx): used to loop
	// forever -- confirmed by a standalone repro that hung until killed
	// -- since S.Search("") always matches at position 1 and
	// EraseBefore(1) is a no-op, so S never shrank.
	STRLIST list;
	list.Split("", STRING("abc"));
	REQUIRE(list.GetTotalEntries() == 1);
	STRING s;
	list.GetEntry(1, &s);
	REQUIRE(s == "abc");
}

TEST_CASE("STRLIST Join concatenates entries with a separator between them", "[strlist]") {
	STRLIST list;
	list.AddEntry("a");
	list.AddEntry("b");
	list.AddEntry("c");
	STRING out;
	list.Join(",", &out);
	REQUIRE(out == "a,b,c");
}

TEST_CASE("STRLIST SearchCase finds a case-insensitive match", "[strlist]") {
	STRLIST list;
	list.AddEntry("Alpha");
	list.AddEntry("Beta");
	REQUIRE(list.SearchCase(STRING("beta")) == 2);
	REQUIRE(list.SearchCase(STRING("gamma")) == 0);
}

TEST_CASE("STRLIST GetValue parses \"Title = value\" entries", "[strlist]") {
	STRLIST list;
	list.AddEntry("Name = Alice");
	list.AddEntry("Age = 30");
	STRING value;
	list.GetValue("Name", &value);
	REQUIRE(value == " Alice");
	list.GetValue(STRING("Age"), &value);
	REQUIRE(value == " 30");
}

TEST_CASE("STRLIST GetValue returns empty for a Title that isn't present", "[strlist]") {
	STRLIST list;
	list.AddEntry("Name = Alice");
	STRING value;
	list.GetValue("Missing", &value);
	REQUIRE(value == "");
}

TEST_CASE("STRLIST Dump writes one entry per line", "[strlist]") {
	STRLIST list;
	list.AddEntry("first");
	list.AddEntry("second");

	char tmpl[] = "/tmp/isearch2_test_strlist_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	FILE* fp = fopen(tmpl, "w");
	REQUIRE(fp != nullptr);
	list.Dump(fp);
	fclose(fp);

	fp = fopen(tmpl, "r");
	REQUIRE(fp != nullptr);
	char buf[256];
	STRING contents;
	while (fgets(buf, sizeof(buf), fp)) {
		contents.Cat(buf);
	}
	fclose(fp);
	remove(tmpl);

	REQUIRE(contents == "first\nsecond\n");
}
