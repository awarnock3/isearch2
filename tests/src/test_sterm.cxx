// Tests for src/sterm.hxx / src/sterm.cxx (class STERM - String Search
// Term). Linked against the real implementation via TEST_ENGINE_OBJS in
// the top-level Makefile, not reimplemented here.
//
// No self-assignment test for operator=: STERM::operator= calls
// OPERAND::operator=, which copies Attributes via
// OtherOp.GetAttributes(&Attributes) -- on self-assignment that's
// ATTRLIST::operator= self-assigning, which (unlike STRING::operator=)
// has no self-assignment guard and silently clears itself. Confirmed
// real with a standalone repro (self-assigning an STERM preserves Term
// but wipes Attributes). The root cause is in src/attrlist.hxx (not yet
// processed) and src/operand.cxx (already processed, Order 6, before
// this was discovered) -- not fixed here since it isn't this file's
// bug to own. See BUG_CATALOG.md under src/sterm.hxx.

#include "catch_amalgamated.hpp"

#include "sterm.hxx"

TEST_CASE("STERM default-constructs with an empty term", "[sterm]") {
	STERM s;
	STRING Term;
	s.GetTerm(&Term);
	REQUIRE(Term.GetLength() == 0);
}

TEST_CASE("STERM SetTerm/GetTerm round-trips", "[sterm]") {
	STERM s;
	s.SetTerm(STRING("hello world"));
	STRING Term;
	s.GetTerm(&Term);
	REQUIRE(Term == "hello world");
}

TEST_CASE("STERM GetOperandType/GetOpType report the expected constants", "[sterm]") {
	STERM s;
	REQUIRE(s.GetOperandType() == TypeTerm);
	REQUIRE(s.GetOpType() == TypeOperand);
}

TEST_CASE("STERM Duplicate creates an independent deep copy", "[sterm]") {
	STERM original;
	original.SetTerm(STRING("original term"));
	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("TITLE"));
	original.SetAttributes(attrs);

	OPOBJ* copy = original.Duplicate();
	REQUIRE(copy != nullptr);
	REQUIRE(copy != (OPOBJ*)&original);

	STRING CopyTerm;
	copy->GetTerm(&CopyTerm);
	REQUIRE(CopyTerm == "original term");

	ATTRLIST CopyAttrs;
	copy->GetAttributes(&CopyAttrs);
	STRING Name;
	CopyAttrs.AttrGetFieldName(&Name);
	REQUIRE(Name == "TITLE");

	copy->SetTerm(STRING("changed"));
	STRING OriginalTerm;
	original.GetTerm(&OriginalTerm);
	REQUIRE(OriginalTerm == "original term");

	delete copy;
}

TEST_CASE("STERM operator= copies term and attributes through the polymorphic OPOBJ& interface", "[sterm]") {
	// Assigns through an OPOBJ& rather than `STERM a, b; a = b;`: direct
	// same-type assignment needs the compiler-generated
	// STERM::operator=(const STERM&), whose base-subobject assignment
	// step bottoms out needing OPOBJ::operator=(const OPOBJ&) -- pure
	// virtual, no definition anywhere -- and fails to *link*. See
	// BUG_CATALOG.md under src/termobj.hxx for the full explanation.
	STERM source;
	source.SetTerm(STRING("source term"));
	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("AUTHOR"));
	source.SetAttributes(attrs);

	STERM dest;
	dest.SetTerm(STRING("placeholder"));
	OPOBJ& DestRef = dest;
	DestRef = source;

	STRING Term;
	dest.GetTerm(&Term);
	REQUIRE(Term == "source term");

	ATTRLIST out;
	dest.GetAttributes(&out);
	STRING Name;
	out.AttrGetFieldName(&Name);
	REQUIRE(Name == "AUTHOR");
}
