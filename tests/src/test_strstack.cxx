// Tests for src/strstack.hxx / src/strstack.cxx (class STRSTACK).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "strlist.hxx"
#include "strstack.hxx"

TEST_CASE("STRSTACK starts empty", "[strstack]") {
	STRSTACK s;
	REQUIRE(s.IsEmpty() == GDT_TRUE);
	REQUIRE(s.GetTotalEntries() == 0);
}

TEST_CASE("STRSTACK Push/Pop behaves as a LIFO stack", "[strstack]") {
	STRSTACK s;
	s.Push(STRING("first"));
	s.Push(STRING("second"));
	s.Push(STRING("third"));
	REQUIRE(s.GetTotalEntries() == 3);
	REQUIRE(s.IsEmpty() == GDT_FALSE);

	STRING v;
	REQUIRE(s.Pop(&v) == GDT_TRUE);
	REQUIRE(v == "third");
	REQUIRE(s.Pop(&v) == GDT_TRUE);
	REQUIRE(v == "second");
	REQUIRE(s.GetTotalEntries() == 1);
	REQUIRE(s.Pop(&v) == GDT_TRUE);
	REQUIRE(v == "first");

	REQUIRE(s.IsEmpty() == GDT_TRUE);
	REQUIRE(s.Pop(&v) == GDT_FALSE);
}

TEST_CASE("STRSTACK Examine peeks without popping", "[strstack]") {
	STRSTACK s;
	s.Push(STRING("only"));
	STRING v;
	REQUIRE(s.Examine(&v) == GDT_TRUE);
	REQUIRE(v == "only");
	REQUIRE(s.GetTotalEntries() == 1);  // not popped
	REQUIRE(s.Examine(&v) == GDT_TRUE);
	REQUIRE(v == "only");
}

TEST_CASE("STRSTACK Examine on an empty stack returns GDT_FALSE", "[strstack]") {
	STRSTACK s;
	STRING v;
	REQUIRE(s.Examine(&v) == GDT_FALSE);
}

TEST_CASE("STRSTACK reuses popped slots correctly on a later push", "[strstack]") {
	// Pop() only moves CurrIndex back -- it doesn't erase the STRLIST
	// node -- so a later Push() must overwrite that slot's old value,
	// not leave it or append past it.
	STRSTACK s;
	s.Push(STRING("a"));
	s.Push(STRING("b"));
	STRING v;
	s.Pop(&v);  // pops "b", CurrIndex back to 1
	s.Push(STRING("c"));  // should overwrite the old "b" slot
	REQUIRE(s.GetTotalEntries() == 2);
	s.Pop(&v);
	REQUIRE(v == "c");
	s.Pop(&v);
	REQUIRE(v == "a");
	REQUIRE(s.IsEmpty() == GDT_TRUE);
}
