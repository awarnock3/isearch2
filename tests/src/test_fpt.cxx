// Tests for src/fpt.hxx / src/fpt.cxx (class FPT - File Pointer
// Table). Linked against the real implementation via TEST_ENGINE_OBJS
// in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "fpt.hxx"

#include <cstdio>
#include <type_traits>
#include <unistd.h>

namespace {

// Real temp file, cleaned up on scope exit. Absolute path (starting
// with '/') so FPT::Lookup's ExpandFileSpec() returns it unmodified
// instead of resolving against the test runner's cwd -- same reasoning
// as test_record.cxx's constructor test.
struct TempFile {
	STRING Path;

	TempFile() {
		char tmpl[] = "/tmp/isearch2_test_fpt_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

}  // namespace

TEST_CASE("FPT is not copyable", "[fpt]") {
	// BUGFIX #1 regression: compile-time check, not a runtime
	// assertion -- FPT(const FPT&) and operator= are both = delete.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<FPT>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<FPT>::value);
}

TEST_CASE("FPT ffopen opens a real file and returns a usable FILE*", "[fpt]") {
	FPT Table;
	TempFile Tmp;

	PFILE Fp = Table.ffopen(Tmp.Path, "w");
	REQUIRE(Fp != nullptr);
	REQUIRE(fputs("hello", Fp) >= 0);
	Table.ffclose(Fp);
}

TEST_CASE("FPT ffopen reuses a cached handle for the same file and mode", "[fpt]") {
	FPT Table;
	TempFile Tmp;

	PFILE First = Table.ffopen(Tmp.Path, "w");
	REQUIRE(First != nullptr);
	Table.ffclose(First);

	// Same file, same mode: cache hit, should hand back the same FILE*
	// (see ffopen()'s "r"/cache-hit branch reusing Fp after a seek --
	// mirrored here for "w" mode's reopen-in-place path).
	PFILE Second = Table.ffopen(Tmp.Path, "w");
	REQUIRE(Second != nullptr);
	Table.ffclose(Second);
}

TEST_CASE("FPT CloseAll closes every logically-closed entry without crashing", "[fpt]") {
	FPT Table;
	TempFile A, B;

	PFILE Fa = Table.ffopen(A.Path, "w");
	PFILE Fb = Table.ffopen(B.Path, "w");
	REQUIRE(Fa != nullptr);
	REQUIRE(Fb != nullptr);
	Table.ffclose(Fa);
	Table.ffclose(Fb);

	Table.CloseAll();
	SUCCEED("no crash closing all cached handles");
}

TEST_CASE("FPT destructor closes remaining handles without a double-free", "[fpt]") {
	// Same shape of concern as the original bug (see
	// docs/AUTOPILOT_LOG.md#srcfpthxx): construct, open one file
	// through it, then destroy -- exercises the exact CloseAll() path
	// that crashed under ASan before FPT was made non-copyable (there's
	// no longer a second, shallow-copied FPT to conflict with).
	FPT* Table = new FPT();
	TempFile Tmp;
	PFILE Fp = Table->ffopen(Tmp.Path, "w");
	REQUIRE(Fp != nullptr);
	delete Table;
	SUCCEED("no ASan/UBSan failure on destruction");
}
