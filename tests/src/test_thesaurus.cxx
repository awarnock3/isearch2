// Tests for src/thesaurus.hxx / src/thesaurus.cxx (classes TH_PARENT,
// TH_PARENT_LIST, TH_ENTRY, TH_ENTRY_LIST, THESAURUS).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// THESAURUS's own constructors need real files on disk (a synonym
// source file to parse, or a previously-built .syn/.spx/.scx set to
// load). Most of the bugs fixed in this file live in TH_PARENT_LIST/
// TH_ENTRY_LIST, which are usable standalone with no file I/O -- those
// are exercised directly below. The THESAURUS-level bugs (BUGFIX
// #6/#7/#8/#9) need a real source file on disk, built via TempFile
// below (same mkstemp() convention as test_fpt.cxx / test_record.cxx).

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "thesaurus.hxx"

#include <cstdio>
#include <dirent.h>
#include <type_traits>
#include <unistd.h>

namespace {

// Real temp file, cleaned up on scope exit. Absolute path so
// THESAURUS's ExpandFileSpec()-free STRING::Cat() usage doesn't need
// to resolve against the test runner's cwd.
struct TempFile {
	STRING Path;

	explicit TempFile(const char* contents) {
		char tmpl[] = "/tmp/isearch2_test_thesaurus_src_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, contents, strlen(contents));
		close(fd);
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

// Counts this process's open file descriptors via /proc/self/fd, to
// confirm THESAURUS::GetParent()/GetChildren() don't leak one per call
// (BUGFIX #8).
int CountOpenFds() {
	int n = 0;
	DIR* d = opendir("/proc/self/fd");
	if (!d) return -1;
	struct dirent* e;
	while ((e = readdir(d)) != nullptr) n++;
	closedir(d);
	return n;
}

}  // namespace

TEST_CASE("TH_PARENT_LIST and TH_ENTRY_LIST are non-copyable", "[thesaurus]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#srcthesaurushxx): both own a
	// heap-allocated table with no correct copy semantics; rather than
	// implement deep-copy semantics nothing in the tree needs, the copy
	// constructor and operator= are both deleted for each.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<TH_PARENT_LIST>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<TH_PARENT_LIST>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<TH_ENTRY_LIST>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<TH_ENTRY_LIST>::value);
}

TEST_CASE("TH_PARENT_LIST::AddEntry grows past its initial 100-entry table", "[thesaurus]") {
	// BUGFIX #1 (most severe finding in this file): AddEntry() used to
	// write table[Count] unconditionally against the fixed 100-entry
	// table, with no growth -- confirmed as a real heap-buffer-overflow
	// via a before/after test-revert under ASan, and confirmed actively
	// reachable via THESAURUS's index-time constructor parsing a
	// user-supplied synonym file with no upper bound on parent count.
	TH_PARENT_LIST List;
	for (INT4 i = 0; i < 150; ++i) {
		TH_PARENT P;
		STRING S = "TERM";
		S += STRING((INT)i);
		P.SetString(S);
		P.SetGlobalStart(i);
		List.AddEntry(P);
	}
	REQUIRE(List.GetCount() == 150);

	TH_PARENT* Last = List.GetEntry(149);
	REQUIRE(Last != nullptr);
	REQUIRE(Last->GetGlobalStart() == 149);
}

TEST_CASE("TH_ENTRY_LIST::AddEntry grows past its initial 100-entry table", "[thesaurus]") {
	// BUGFIX #1, second class -- see TH_PARENT_LIST above.
	TH_ENTRY_LIST List;
	for (INT4 i = 0; i < 150; ++i) {
		TH_ENTRY E;
		E.SetGlobalStart(i);
		E.SetParentPtr(i * 2);
		List.AddEntry(E);
	}
	REQUIRE(List.GetCount() == 150);

	TH_ENTRY* Last = List.GetEntry(149);
	REQUIRE(Last != nullptr);
	REQUIRE(Last->GetParentPtr() == 298);
}

TEST_CASE("TH_PARENT_LIST::GetEntry treats index==Count as out of range", "[thesaurus]") {
	// BUGFIX #2: both GetEntry() overloads used to guard with
	// `index <= Count` instead of `index < Count` -- Count is the index
	// of the next *unused* slot, so `index == Count` silently returned
	// an indeterminate/never-written entry.
	TH_PARENT_LIST List;
	TH_PARENT P;
	P.SetString("ONLY");
	List.AddEntry(P);

	REQUIRE(List.GetEntry(0) != nullptr);
	REQUIRE(List.GetEntry(1) == nullptr);

	TH_PARENT Out;
	Out.SetString("UNCHANGED");
	List.GetEntry(1, &Out);
	STRING S;
	Out.GetString(&S);
	REQUIRE(S == "UNCHANGED");
}

TEST_CASE("TH_PARENT::Copy duplicates GlobalStart and Term", "[thesaurus]") {
	// BUGFIX #5: Copy()'s body was empty, unlike the adjacent (correct)
	// operator= -- confirmed dead code (no live call site), completed to
	// match operator='s behavior.
	TH_PARENT Source;
	Source.SetString("SPATIAL");
	Source.SetGlobalStart(42);

	TH_PARENT Dest;
	Dest.Copy(Source);

	STRING S;
	Dest.GetString(&S);
	REQUIRE(S == "SPATIAL");
	REQUIRE(Dest.GetGlobalStart() == 42);
}

TEST_CASE("TH_PARENT's default constructor starts GlobalStart at a deterministic value", "[thesaurus]") {
	// BUGFIX #4: GlobalStart was left indeterminate.
	TH_PARENT P;
	REQUIRE(P.GetGlobalStart() == 0);

	TH_ENTRY E;
	REQUIRE(E.GetGlobalStart() == 0);
	REQUIRE(E.GetParentPtr() == 0);
}

TEST_CASE("THESAURUS index-time constructor tolerates a missing/empty source file", "[thesaurus]") {
	// BUGFIX #6: SourceFileName not existing left sBuf empty, so
	// strtok() on its NewCString() returned nullptr -- dereferenced
	// unconditionally by the parsing loop below, a real SIGSEGV.
	// Confirmed via a standalone repro before this fix.
	STRING SourceFileName("/tmp/isearch2_test_thesaurus_definitely_does_not_exist");
	STRING DbPathName("/tmp");
	STRING DbFileName("isearch2_test_thesaurus_missing_src");

	THESAURUS Th(SourceFileName, DbPathName, DbFileName);

	// Degenerates to an empty thesaurus: any term is its own parent and
	// its own (only) child.
	STRING Parent;
	Th.GetParent(STRING("WHATEVER"), &Parent);
	REQUIRE(Parent == "WHATEVER");
}

TEST_CASE("THESAURUS index-time constructor doesn't leak its parse buffer when OpenSynonymFile fails", "[thesaurus]") {
	// BUGFIX #7: found alongside BUGFIX #6 -- `b` (the parsed source
	// buffer) was only freed after the parsing loop completed normally,
	// leaking it on this early-return path too. Not independently
	// observable through the public API (no crash, just a leak); this
	// test exists mainly as a construction/regression smoke test for
	// the DbPathName-not-writable early-return path.
	TempFile Src("DOG=CANINE+PUPPY\n");
	STRING SourceFileName(Src.Path);
	STRING DbPathName("/tmp/isearch2_test_thesaurus_nonexistent_dir");
	STRING DbFileName("db");

	THESAURUS Th(SourceFileName, DbPathName, DbFileName);
	SUCCEED();
}

TEST_CASE("THESAURUS::GetChildren strips the trailing newline from the last child term", "[thesaurus]") {
	// BUGFIX #9: fgets() reads the line's trailing '\n' into buf, never
	// stripped before Replace()/Split() -- so the last child term in the
	// list came back with an embedded newline (e.g. "PUPPY\n" instead of
	// "PUPPY"). Confirmed via a standalone repro; reachable through the
	// live SQUERY::ExpandQuery() synonym-expansion path.
	TempFile Src("DOG=CANINE+PUPPY\nCAT=FELINE+KITTEN\n");
	STRING SourceFileName(Src.Path);
	STRING DbPathName("/tmp");
	STRING DbFileName("isearch2_test_thesaurus_children");

	THESAURUS Th(SourceFileName, DbPathName, DbFileName);

	STRLIST Children;
	Th.GetChildren(STRING("DOG"), &Children);

	// GetChildren() always includes the original term first (its own
	// documented contract), so DOG=CANINE+PUPPY yields 3 entries.
	REQUIRE(Children.GetTotalEntries() == 3);

	STRING Last;
	Children.GetEntry(Children.GetTotalEntries(), &Last);
	REQUIRE(Last == "PUPPY");
	REQUIRE(Last.GetLength() == 5);  // not 6 -- no trailing '\n'
}

TEST_CASE("THESAURUS::GetParent resolves a child term to its parent", "[thesaurus]") {
	TempFile Src("DOG=CANINE+PUPPY\n");
	STRING SourceFileName(Src.Path);
	STRING DbPathName("/tmp");
	STRING DbFileName("isearch2_test_thesaurus_parent");

	THESAURUS Th(SourceFileName, DbPathName, DbFileName);

	STRING Parent;
	Th.GetParent(STRING("PUPPY"), &Parent);
	REQUIRE(Parent == "DOG");
}

TEST_CASE("THESAURUS::GetParent and GetChildren don't leak a file descriptor per call", "[thesaurus]") {
	// BUGFIX #8: both used to open the synonym file via
	// OpenSynonymFile("rb") and never close it on the success path.
	// Confirmed via /proc/self/fd inspection: reverting the fix showed
	// the open-fd count growing by 2 per GetParent()+GetChildren() pair.
	TempFile Src("DOG=CANINE+PUPPY\n");
	STRING SourceFileName(Src.Path);
	STRING DbPathName("/tmp");
	STRING DbFileName("isearch2_test_thesaurus_fdleak");

	THESAURUS Th(SourceFileName, DbPathName, DbFileName);

	int Before = CountOpenFds();
	REQUIRE(Before >= 0);
	for (int i = 0; i < 25; i++) {
		STRING Parent;
		Th.GetParent(STRING("PUPPY"), &Parent);
		STRLIST Children;
		Th.GetChildren(STRING("DOG"), &Children);
	}
	int After = CountOpenFds();
	REQUIRE(After == Before);
}
