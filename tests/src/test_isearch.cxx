// Tests for src/isearch.hxx -- a pure convenience aggregator (declares
// nothing of its own; just #includes every core engine header a
// doctype parser or CGI frontend typically needs). There's no behavior
// of its own to test; this confirms the header is self-contained and
// that the full set of aggregated headers is mutually coherent
// (no missing includes, no redefinition/ambiguity conflicts) by
// including it as the sole header and touching a few of the aggregated
// types.

#include "catch_amalgamated.hpp"

#include "isearch.hxx"

TEST_CASE("isearch.hxx is self-contained and its aggregated types are usable", "[isearch]") {
	STRING s("hello");
	REQUIRE(s == "hello");

	STRLIST list;
	list.AddEntry("a");
	list.AddEntry("b");
	REQUIRE(list.GetTotalEntries() == 2);

	RECORD record;
	record.SetKey(STRING("doc1"));

	GSTACK stack;
	REQUIRE(stack.GetSize() == 0);
}
