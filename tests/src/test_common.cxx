// Tests for src/common.hxx / src/common.cxx (free-function grab bag:
// path helpers, file/db existence checks, endianness helpers, and a
// few string/date utilities).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "common.hxx"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace {

// Owns a real temp file on disk and removes it on scope exit.
struct TempFile {
	STRING Path;

	TempFile() {
		char tmpl[] = "/tmp/isearch2_test_common_XXXXXX";
		int fd = mkstemp(tmpl);
		REQUIRE(fd != -1);
		REQUIRE(write(fd, "hello", 5) == 5);
		close(fd);
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

}  // namespace

TEST_CASE("AddTrailingSlash appends DIR_SLASH when missing", "[common]") {
	STRING s("/some/dir");
	AddTrailingSlash(&s);
	REQUIRE(s == "/some/dir/");
}

TEST_CASE("AddTrailingSlash is a no-op when already present", "[common]") {
	STRING s("/some/dir/");
	AddTrailingSlash(&s);
	REQUIRE(s == "/some/dir/");
}

TEST_CASE("AddTrailingSlash appends a slash to a single-character path like \".\"", "[common]") {
	// BUGFIX #7 (docs/BUG_CATALOG.md#srccommoncxx): the length guard used
	// to be `> 1`, so a single-character path never got a trailing slash
	// at all -- confirmed via Isearch-cgi/api_search.cxx's own "." db_path
	// fallback silently concatenating into ".mydb.mdt" instead of
	// "./mydb.mdt".
	STRING s(".");
	AddTrailingSlash(&s);
	REQUIRE(s == "./");
}

TEST_CASE("AddTrailingSlash leaves an empty path empty", "[common]") {
	// Not the same as the single-character case above: appending a slash
	// to "" would change "no path" into "root directory", a real,
	// deliberate behavior difference -- confirmed this stays untouched.
	STRING s("");
	AddTrailingSlash(&s);
	REQUIRE(s.GetLength() == 0);
}

TEST_CASE("RemovePath keeps only the last path component", "[common]") {
	STRING s("/some/dir/file.txt");
	RemovePath(&s);
	REQUIRE(s == "file.txt");
}

TEST_CASE("RemovePath is a no-op with no DIR_SLASH", "[common]") {
	STRING s("file.txt");
	RemovePath(&s);
	REQUIRE(s == "file.txt");
}

TEST_CASE("RemoveFileName keeps only the directory prefix", "[common]") {
	STRING s("/some/dir/file.txt");
	RemoveFileName(&s);
	REQUIRE(s == "/some/dir/");
}

TEST_CASE("RemoveFileName yields empty with no DIR_SLASH", "[common]") {
	// Complement of RemovePath(): no directory component means the
	// directory part IS empty -- see Iindex.cxx's DBPathName/DBFileName
	// split, which relies on exactly this pairing.
	STRING s("file.txt");
	RemoveFileName(&s);
	REQUIRE(s == "");
}

TEST_CASE("RemoveFileExtension strips the last extension, keeping the dot", "[common]") {
	STRING s("archive.tar.gz");
	RemoveFileExtension(&s);
	REQUIRE(s == "archive.tar.");
}

TEST_CASE("RemoveFileExtension is a no-op with no '.'", "[common]") {
	// BUGFIX #4 (see docs/BUG_CATALOG.md#srccommoncxx): used to wipe the
	// whole string via EraseAfter(0) instead of leaving it untouched.
	STRING s("myfile");
	RemoveFileExtension(&s);
	REQUIRE(s == "myfile");
}

TEST_CASE("GetFileSize overloads agree on a real file", "[common]") {
	TempFile f;
	REQUIRE(GetFileSize(f.Path) == 5);
	REQUIRE(GetFileSize((const CHR*)f.Path) == 5);

	PFILE fp = fopen(f.Path, "rb");
	REQUIRE(fp != nullptr);
	REQUIRE(GetFileSize(fp) == 5);
	fclose(fp);
}

TEST_CASE("GetFileSize returns -1 for a missing file", "[common]") {
	REQUIRE(GetFileSize(STRING("/nonexistent/isearch2_test_common_missing")) == (off_t)-1);
}

TEST_CASE("IsFile is true for a regular file, false for a directory or missing path", "[common]") {
	TempFile f;
	REQUIRE(IsFile(f.Path) == GDT_TRUE);
	REQUIRE(IsFile((const CHR*)f.Path) == GDT_TRUE);
	REQUIRE(IsFile(STRING("/tmp")) == GDT_FALSE);
	REQUIRE(IsFile(STRING("/nonexistent/isearch2_test_common_missing")) == GDT_FALSE);
}

TEST_CASE("DBExists finds a matching .dbi or .vdb, not an unrelated stem", "[common]") {
	char tmpl[] = "/tmp/isearch2_test_common_db_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	remove(tmpl);
	STRING Stem(tmpl);

	REQUIRE(DBExists(Stem) == GDT_FALSE);

	STRING DbiPath = Stem;
	DbiPath.Cat(".dbi");
	FILE* fp = fopen(DbiPath, "w");
	REQUIRE(fp != nullptr);
	fclose(fp);

	REQUIRE(DBExists(Stem) == GDT_TRUE);
	remove(DbiPath);
}

TEST_CASE("IsAlnum accepts letters, digits, and underscore", "[common]") {
	REQUIRE(IsAlnum('a') == GDT_TRUE);
	REQUIRE(IsAlnum('Z') == GDT_TRUE);
	REQUIRE(IsAlnum('5') == GDT_TRUE);
	REQUIRE(IsAlnum('_') == GDT_TRUE);
}

TEST_CASE("IsAlnum rejects whitespace and punctuation", "[common]") {
	REQUIRE(IsAlnum(' ') == GDT_FALSE);
	REQUIRE(IsAlnum('.') == GDT_FALSE);
	REQUIRE(IsAlnum(',') == GDT_FALSE);
}

TEST_CASE("trimWhitespace strips leading and trailing spaces", "[common]") {
	char buf[32];
	strcpy(buf, "  hello world  ");
	trimWhitespace(buf);
	REQUIRE(std::string(buf) == "hello world");
}

TEST_CASE("trimWhitespace handles an empty string without crashing", "[common]") {
	char buf[8] = "";
	trimWhitespace(buf);
	REQUIRE(std::string(buf) == "");
}

TEST_CASE("ParseIsoDate returns 0.0 for an empty string", "[common]") {
	REQUIRE(ParseIsoDate(STRING("")) == 0.0);
}

TEST_CASE("ParseIsoDate parses a bare date to a whole-number day value", "[common]") {
	DOUBLE d = ParseIsoDate(STRING("2026-08-07"));
	REQUIRE(d == 20260807.0);
}

TEST_CASE("ParseIsoDate folds a T..Z time into the fractional part", "[common]") {
	// BUGFIX #5 (see docs/BUG_CATALOG.md#srccommoncxx): date_val/time_val
	// used to be uninitialized reads whenever the date lacked '-' or the
	// time lacked ':'; a well-formed date+time exercises both branches
	// that now seed them explicitly.
	DOUBLE d = ParseIsoDate(STRING("2026-08-07T12:00:00Z"));
	REQUIRE(d > 20260807.0);
	REQUIRE(d < 20260808.0);
	REQUIRE(d == Catch::Approx(20260807.5).epsilon(0.001));
}

TEST_CASE("ParseIsoDate returns the sentinel for a non-digit date", "[common]") {
	REQUIRE(ParseIsoDate(STRING("not-a-date")) == -99999999.0);
}

TEST_CASE("IsBigEndian returns a stable value", "[common]") {
	// Just confirms it runs and is self-consistent; the actual value is
	// host-dependent.
	GDT_BOOLEAN a = IsBigEndian();
	GDT_BOOLEAN b = IsBigEndian();
	REQUIRE(a == b);
}

TEST_CASE("GpSwab reverses all four bytes in place", "[common]") {
	GPTYPE g = 0;
	unsigned char* b = (unsigned char*)&g;
	b[0] = 0x11; b[1] = 0x22; b[2] = 0x33; b[3] = 0x44;
	GpSwab(&g);
	REQUIRE(b[0] == 0x44);
	REQUIRE(b[1] == 0x33);
	REQUIRE(b[2] == 0x22);
	REQUIRE(b[3] == 0x11);
}

TEST_CASE("GpSwab is its own inverse", "[common]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srccommoncxx): previously read
	// an uninitialized local on every call in this build.
	GPTYPE original = 0x12345678;
	GPTYPE g = original;
	GpSwab(&g);
	REQUIRE(g != original);
	GpSwab(&g);
	REQUIRE(g == original);
}

TEST_CASE("rename() actually renames a file instead of recursing forever", "[common]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srccommoncxx): this overload
	// used to call itself instead of the C library, overflowing the
	// stack on every invocation.
	char tmpl[] = "/tmp/isearch2_test_common_rename_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	STRING From(tmpl);
	STRING To(tmpl);
	To.Cat(".renamed");

	REQUIRE(rename(From, To) == 0);
	REQUIRE(IsFile(To) == GDT_TRUE);
	REQUIRE(IsFile(From) == GDT_FALSE);
	remove(To);
}
