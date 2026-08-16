// Tests for src/termobj.hxx / src/termobj.cxx (class TERMOBJ - Search
// Term Base Class). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.
//
// TERMOBJ is abstract (OPOBJ::Duplicate() is still unimplemented -- left
// to concrete leaves like the real STERM). A minimal concrete subclass
// tests TERMOBJ's own behavior in isolation.

#include "catch_amalgamated.hpp"

#include "termobj.hxx"

class TESTTERMOBJ : public TERMOBJ {
public:
	using TERMOBJ::operator=; // each derived class needs its own using
	                          // declaration to keep the inherited
	                          // operator= visible -- TERMOBJ's own
	                          // `using OPERAND::operator=;` (BUGFIX #2)
	                          // doesn't propagate automatically.
	OPOBJ* Duplicate() const override { return new TESTTERMOBJ(); }
};

TEST_CASE("TERMOBJ GetOperandType reports TypeTerm", "[termobj]") {
	TESTTERMOBJ t;
	REQUIRE(t.GetOperandType() == TypeTerm);
}

TEST_CASE("TERMOBJ inherits GetOpType from OPERAND", "[termobj]") {
	TESTTERMOBJ t;
	REQUIRE(t.GetOpType() == TypeOperand);
}

// Note: no "direct same-type assignment" test here (`TESTTERMOBJ a, b;
// a = b;`). That would need the compiler-generated
// TESTTERMOBJ::operator=(const TESTTERMOBJ&), whose base-subobject
// assignment step bottoms out at OPOBJ::operator=(const OPOBJ&) -- pure
// virtual, with no definition anywhere -- and fails to *link*, not just
// misbehave. Confirmed by hitting it while writing this test. This is a
// pre-existing structural property of the whole OPOBJ hierarchy (not
// introduced by BUGFIX #2, and not fixable at the TERMOBJ level alone),
// not currently triggered by any real caller in this tree -- see
// BUG_CATALOG.md. Always assign through the polymorphic OPOBJ&/OPERAND&
// interface instead, as every real caller already does.
TEST_CASE("TERMOBJ assignment through the polymorphic OPOBJ& interface works", "[termobj]") {
	TESTTERMOBJ source;
	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("TITLE"));
	source.SetAttributes(attrs);

	TESTTERMOBJ dest;
	OPOBJ& DestRef = dest;
	DestRef = source;

	ATTRLIST out;
	dest.GetAttributes(&out);
	STRING Name;
	out.AttrGetFieldName(&Name);
	REQUIRE(Name == "TITLE");
}
