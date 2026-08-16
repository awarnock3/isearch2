// Tests for src/string.hxx / src/string.cxx (class STRING).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "string.hxx"

#include <cstdio>
#include <sstream>
#include <unistd.h>

TEST_CASE("STRING default-constructs empty", "[string]") {
	STRING s;
	REQUIRE(s.GetLength() == 0);
}

TEST_CASE("STRING constructs from a C string and round-trips its content", "[string]") {
	STRING s("hello");
	REQUIRE(s.GetLength() == 5);
	REQUIRE(s == "hello");
	REQUIRE(s != "world");
}

TEST_CASE("STRING copy constructor and operator= are independent of the source", "[string]") {
	STRING original("original");
	STRING copied = original;
	STRING assigned;
	assigned = original;

	copied.Cat("-copy");
	assigned.Cat("-assigned");

	REQUIRE(original == "original");
	REQUIRE(copied == "original-copy");
	REQUIRE(assigned == "original-assigned");
}

TEST_CASE("STRING self-assignment is a no-op, not a self-destruct", "[string]") {
	STRING s("self");
	s = s;
	REQUIRE(s == "self");
}

TEST_CASE("STRING Equals/CaseEquals", "[string]") {
	STRING s("MixedCase");
	REQUIRE(s.Equals(STRING("MixedCase")));
	REQUIRE_FALSE(s.Equals(STRING("mixedcase")));
	REQUIRE(s.CaseEquals(STRING("mixedcase")));
	REQUIRE(s.CaseEquals("MIXEDCASE"));
}

TEST_CASE("STRING Cat grows the buffer across the small-buffer threshold", "[string]") {
	STRING s;
	for (int i = 0; i < 100; i++) {
		s.Cat('x');
	}
	REQUIRE(s.GetLength() == 100);
}

TEST_CASE("STRING Insert splices content at a 1-based position", "[string]") {
	STRING s("ac");
	s.Insert(2, STRING("b"));
	REQUIRE(s == "abc");
}

TEST_CASE("STRING Search returns a 1-based position, or 0 if not found", "[string]") {
	STRING s("hello world");
	REQUIRE(s.Search("world") == 7);
	REQUIRE(s.Search("xyz") == 0);
	REQUIRE(s.Search('w') == 7);
}

TEST_CASE("STRING SearchReverse finds the last occurrence", "[string]") {
	STRING s("abcabc");
	REQUIRE(s.SearchReverse("abc") == 4);
	REQUIRE(s.SearchReverse('a') == 4);
}

TEST_CASE("STRING Replace substitutes every occurrence and returns the count", "[string]") {
	STRING s("one two one two one");
	INT Count = s.Replace("one", "1");
	REQUIRE(Count == 3);
	REQUIRE(s == "1 two 1 two 1");
}

TEST_CASE("STRING Replace with an empty search string is a no-op, not an infinite loop", "[string]") {
	// BUGFIX #3 coverage: before the fix, Search("") always matched at
	// position 1 and EraseBefore(1) was a no-op, so this would spin
	// forever growing NewString without bound. A test that hangs is its
	// own kind of failure signal, but the point of the guard is that this
	// now returns immediately.
	STRING s("unchanged");
	INT Count = s.Replace("", "X");
	REQUIRE(Count == 0);
	REQUIRE(s == "unchanged");
}

TEST_CASE("STRING EraseBefore drops everything before a 1-based index", "[string]") {
	STRING s("abcdef");
	s.EraseBefore(3);
	REQUIRE(s == "cdef");
}

TEST_CASE("STRING EraseBefore with Index <= 1 is a no-op", "[string]") {
	STRING s("abc");
	s.EraseBefore(1);
	REQUIRE(s == "abc");
}

TEST_CASE("STRING EraseAfter truncates to a 1-based inclusive index", "[string]") {
	STRING s("abcdef");
	s.EraseAfter(3);
	REQUIRE(s == "abc");
}

TEST_CASE("STRING GetChr/SetChr are 1-based", "[string]") {
	STRING s("abc");
	REQUIRE(s.GetChr(1) == 'a');
	REQUIRE(s.GetChr(3) == 'c');
	REQUIRE(s.GetChr(99) == 0);

	s.SetChr(2, 'X');
	REQUIRE(s == "aXc");
}

TEST_CASE("STRING GetCString truncates to fit the caller's buffer", "[string]") {
	STRING s("hello world");
	CHR buf[6];
	s.GetCString(buf, sizeof(buf));
	REQUIRE(STRING(buf) == "hello");
}

TEST_CASE("STRING GetInt/GetLong/GetFloat parse numeric content", "[string]") {
	REQUIRE(STRING("42").GetInt() == 42);
	REQUIRE(STRING("123456789012").GetLong() == 123456789012L);
	REQUIRE(STRING("3.5").GetFloat() == Catch::Approx(3.5));
}

TEST_CASE("STRING Trim/TrimLeading remove whitespace", "[string]") {
	STRING s("   padded   ");
	s.TrimLeading();
	REQUIRE(s == "padded   ");
	s.Trim();
	REQUIRE(s == "padded");
}

TEST_CASE("STRING UpperCase converts in place", "[string]") {
	STRING s("MixedCase");
	s.UpperCase();
	REQUIRE(s == "MIXEDCASE");
}

TEST_CASE("STRING IsNumber/IsPrint", "[string]") {
	REQUIRE(STRING("-123.45").IsNumber());
	REQUIRE_FALSE(STRING("12a").IsNumber());
	REQUIRE(STRING("printable").IsPrint());
}

TEST_CASE("STRING Cmp behaves like strcmp, not a boolean", "[string]") {
	REQUIRE(STRING("a").Cmp(STRING("b")) < 0);
	REQUIRE(STRING("b").Cmp(STRING("a")) > 0);
	REQUIRE(STRING("a").Cmp(STRING("a")) == 0);
}

TEST_CASE("STRING operator<< writes exactly Length bytes, no implicit terminator", "[string]") {
	STRING s("abc");
	std::ostringstream os;
	os << s;
	REQUIRE(os.str() == "abc");
}

TEST_CASE("STRING WriteFile/ReadFile round-trips content through disk", "[string]") {
	STRING path;
	{
		char tmpl[] = "/tmp/isearch2_test_string_XXXXXX";
		int fd = mkstemp(tmpl);
		REQUIRE(fd >= 0);
		close(fd);
		path = tmpl;
	}

	STRING original("round-trip me");
	original.WriteFile(path);

	STRING restored;
	GDT_BOOLEAN Ok = restored.ReadFile(path);
	REQUIRE(Ok == GDT_TRUE);
	REQUIRE(restored == "round-trip me");

	remove((const CHR*)path);
}

TEST_CASE("STRING ReadFile on a nonexistent path leaves the string usable, not double-freed", "[string]") {
	// BUGFIX #1 coverage: before the fix, this sequence -- ReadFile()
	// failing, then the STRING being used again (here, just letting it
	// destruct) -- crashed with a double-free under ASan, because the
	// original unconditionally freed Buffer before checking whether the
	// file existed, and never reallocated it on the failure path.
	STRING s("original content");
	GDT_BOOLEAN Ok = s.ReadFile("/nonexistent/path/isearch2_test_never_exists");
	REQUIRE(Ok == GDT_FALSE);
	REQUIRE(s == "original content");
}

TEST_CASE("STRING ReadFile(STRING&) overload behaves the same as ReadFile(CHR*)", "[string]") {
	STRING s;
	GDT_BOOLEAN Ok = s.ReadFile(STRING("/nonexistent/path/isearch2_test_never_exists"));
	REQUIRE(Ok == GDT_FALSE);
}

TEST_CASE("STRING XmlCleanup escapes special characters", "[string]") {
	STRING s("<a href=\"x\">Tom & Jerry's</a>");
	s.XmlCleanup();
	REQUIRE(s == "&lt;a href=&quot;x&quot;&gt;Tom &amp; Jerry&apos;s&lt;/a&gt;");
}

TEST_CASE("STRING XmlCleanup doesn't truncate the last numeric entity", "[string]") {
	// BUGFIX #2 coverage: before the fix, transcode()'s output buffer
	// (strlen(obuf)*6 bytes, no slack for the terminator) was exactly
	// sized for an all-high-byte input, so the last "&#NNN;" entity's
	// closing ';' was silently dropped. Confirmed real with a standalone
	// program: transcoding three bytes >= 128 produced "&#200;&#201;&#202"
	// (17 chars) instead of the correct 18-char string ending in ';'.
	UCHR raw[] = { 200, 201, 202, '\0' };
	STRING s(raw);
	s.XmlCleanup();
	REQUIRE(s == "&#200;&#201;&#202;");
}

TEST_CASE("STRING XmlCleanup on an empty string stays empty", "[string]") {
	STRING s("");
	s.XmlCleanup();
	REQUIRE(s.GetLength() == 0);
}
