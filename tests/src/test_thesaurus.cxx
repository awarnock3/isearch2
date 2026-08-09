// Tests for src/thesaurus.hxx / src/thesaurus.cxx (classes TH_PARENT,
// TH_PARENT_LIST, TH_ENTRY, TH_ENTRY_LIST; THESAURUS itself is not
// exercised here -- see the note below).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// THESAURUS's own constructors need real files on disk (a synonym
// source file to parse, or a previously-built .syn/.spx/.scx set to
// load); the bugs fixed in this file all live in TH_PARENT_LIST/
// TH_ENTRY_LIST, which are usable standalone with no file I/O, so the
// tests below exercise those directly instead of building a THESAURUS
// fixture.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "thesaurus.hxx"

#include <type_traits>

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
