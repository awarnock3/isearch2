// Tests for src/registry.hxx / src/registry.cxx (class REGISTRY - a
// first-child/next-sibling tree of named nodes, plus the free function
// parseMetaDefaults()).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "registry.hxx"

#include <cstdio>
#include <unistd.h>

TEST_CASE("REGISTRY SetData/GetData round-trip under a path", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, val, out;
	pos.AddEntry("section");
	val.AddEntry("v1");
	val.AddEntry("v2");
	reg.SetData(pos, val);

	reg.GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 2);
	STRING s;
	out.GetEntry(1, &s);
	REQUIRE(s == "v1");
	out.GetEntry(2, &s);
	REQUIRE(s == "v2");
}

TEST_CASE("REGISTRY AddData appends without clearing existing entries", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, val1, val2, out;
	pos.AddEntry("section");
	val1.AddEntry("a");
	val2.AddEntry("b");
	reg.SetData(pos, val1);
	reg.AddData(pos, val2);

	reg.GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 2);
}

TEST_CASE("REGISTRY GetData on a missing path clears the buffer", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, out;
	pos.AddEntry("nonexistent");
	out.AddEntry("stale");
	reg.GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 0);
}

TEST_CASE("REGISTRY ProfileGetString/ProfileWriteString round-trip a single value", "[registry]") {
	REGISTRY reg("root");
	reg.ProfileWriteString(STRING("Sec"), STRING("Key"), STRING("value1"));
	STRING out;
	reg.ProfileGetString(STRING("Sec"), STRING("Key"), STRING("default"), &out);
	REQUIRE(out == "value1");
}

TEST_CASE("REGISTRY ProfileGetString returns the default when not set", "[registry]") {
	REGISTRY reg("root");
	STRING out;
	reg.ProfileGetString(STRING("Sec"), STRING("Missing"), STRING("fallback"), &out);
	REQUIRE(out == "fallback");
}

TEST_CASE("REGISTRY ProfileWriteString stores a comma-delimited value as multiple nodes", "[registry]") {
	REGISTRY reg("root");
	reg.ProfileWriteString(STRING("Sec"), STRING("Key"), STRING("a,b,c"));
	STRING out;
	reg.ProfileGetString(STRING("Sec"), STRING("Key"), STRING(""), &out);
	REQUIRE(out == "a,b,c");
}

TEST_CASE("REGISTRY clone() deep-copies, independent of the source's later mutation", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, val;
	pos.AddEntry("section");
	val.AddEntry("original");
	reg.SetData(pos, val);

	REGISTRY* cloned = reg.clone();

	STRLIST val2;
	val2.AddEntry("mutated");
	reg.SetData(pos, val2);

	STRLIST out;
	cloned->GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 1);
	STRING s;
	out.GetEntry(1, &s);
	REQUIRE(s == "original");
	delete cloned;
}

TEST_CASE("REGISTRY operator= deep-copies, independent of the source's later mutation", "[registry]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcregistrycxx): operator= used
	// to alias the source's Next/Child pointers instead of copying the
	// subtree, so two REGISTRY objects ended up owning (and both
	// destructing) the same nodes.
	REGISTRY a("a");
	STRLIST pos, val;
	pos.AddEntry("section");
	val.AddEntry("original");
	a.SetData(pos, val);

	REGISTRY b("b");
	b = a;

	STRLIST val2;
	val2.AddEntry("mutated");
	a.SetData(pos, val2);

	STRLIST out;
	b.GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 1);
	STRING s;
	out.GetEntry(1, &s);
	REQUIRE(s == "original");
}

TEST_CASE("REGISTRY operator= self-assignment leaves data intact", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, val;
	pos.AddEntry("section");
	val.AddEntry("keep");
	reg.SetData(pos, val);

	reg = reg;

	STRLIST out;
	reg.GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 1);
	STRING s;
	out.GetEntry(1, &s);
	REQUIRE(s == "keep");
}

TEST_CASE("REGISTRY SaveToFile/ProfileAddFromFile-style round-trip via fprint format", "[registry]") {
	REGISTRY reg("root");
	STRLIST pos, val;
	pos.AddEntry("section");
	val.AddEntry("leaf");
	reg.SetData(pos, val);

	char tmpl[] = "/tmp/isearch2_test_registry_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	STRLIST empty;
	reg.SaveToFile(STRING(tmpl), empty);

	FILE* fp = fopen(tmpl, "r");
	REQUIRE(fp != nullptr);
	char buf[256];
	STRING contents;
	while (fgets(buf, sizeof(buf), fp)) {
		contents.Cat(buf);
	}
	fclose(fp);
	remove(tmpl);

	REQUIRE(contents.Search("section") != 0);
	REQUIRE(contents.Search("leaf") != 0);
}

TEST_CASE("parseMetaDefaults parses nested tags into a path-keyed tree", "[registry]") {
	char tmpl[] = "/tmp/isearch2_test_registry_meta_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	FILE* fp = fopen(tmpl, "w");
	REQUIRE(fp != nullptr);
	fprintf(fp, "<outer><inner>leafvalue</inner></outer>");
	fclose(fp);

	REGISTRY* metadef = parseMetaDefaults(STRING(tmpl));
	remove(tmpl);

	STRLIST pos, out;
	pos.AddEntry("outer");
	pos.AddEntry("inner");
	metadef->GetData(pos, &out);
	REQUIRE(out.GetTotalEntries() == 1);
	STRING s;
	out.GetEntry(1, &s);
	REQUIRE(s == "leafvalue");
	delete metadef;
}

TEST_CASE("parseMetaDefaults doesn't overflow on a long unbroken text run", "[registry]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcregistrycxx): the tokenizer's
	// 1024-byte stack buffer had no bounds check, so any tag or text
	// content longer than that overflowed it -- confirmed under ASan
	// before the fix.
	char tmpl[] = "/tmp/isearch2_test_registry_long_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	FILE* fp = fopen(tmpl, "w");
	REQUIRE(fp != nullptr);
	fprintf(fp, "<tag>");
	for (int i = 0; i < 2000; i++) {
		fputc('x', fp);
	}
	fprintf(fp, "</tag>");
	fclose(fp);

	REGISTRY* metadef = parseMetaDefaults(STRING(tmpl));
	remove(tmpl);
	delete metadef;
	SUCCEED("did not crash");
}

TEST_CASE("parseMetaDefaults doesn't crash or leak on tags-only content", "[registry]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srcregistrycxx): `data` was left
	// uninitialized when no text/data token was ever seen, and the
	// trailing `delete data;` ran unconditionally -- undefined behavior
	// on whatever garbage was on the stack.
	char tmpl[] = "/tmp/isearch2_test_registry_notext_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	FILE* fp = fopen(tmpl, "w");
	REQUIRE(fp != nullptr);
	fprintf(fp, "<a></a>");
	fclose(fp);

	REGISTRY* metadef = parseMetaDefaults(STRING(tmpl));
	remove(tmpl);
	delete metadef;
	SUCCEED("did not crash");
}

TEST_CASE("parseMetaDefaults doesn't leak across multiple data tokens", "[registry]") {
	// BUGFIX #3 continued: every data token before the last one leaked
	// its STRLIST -- confirmed under LeakSanitizer before the fix.
	char tmpl[] = "/tmp/isearch2_test_registry_multi_XXXXXX";
	int fd = mkstemp(tmpl);
	REQUIRE(fd != -1);
	close(fd);
	FILE* fp = fopen(tmpl, "w");
	REQUIRE(fp != nullptr);
	fprintf(fp, "<a>val1</a><b>val2</b><c>val3</c>");
	fclose(fp);

	REGISTRY* metadef = parseMetaDefaults(STRING(tmpl));
	remove(tmpl);
	delete metadef;
	SUCCEED("did not leak");
}
