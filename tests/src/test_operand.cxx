// Tests for src/operand.hxx / src/operand.cxx (class OPERAND - Query
// Operand). Linked against the real implementation via TEST_ENGINE_OBJS
// in the top-level Makefile, not reimplemented here.
//
// OPERAND is abstract (OPOBJ::Duplicate()/GetOperandType() are pure
// virtual and OPERAND doesn't override either -- that's left to concrete
// leaves like IRSET). A minimal concrete subclass here tests OPERAND's
// own behavior without dragging in IRSET's much larger dependency chain
// (and IRSET has its own, separate, already-confirmed copy-constructor
// bug for its own Table member -- see BUG_CATALOG.md's src/operand.hxx
// section -- unrelated to what's under test here).

#include "catch_amalgamated.hpp"

#include "operand.hxx"

class TESTOPERAND : public OPERAND {
public:
	using OPERAND::operator=; // un-hide OPERAND::operator=(const OPOBJ&);
	                          // without this, the compiler-generated
	                          // TESTOPERAND::operator=(const TESTOPERAND&)
	                          // hides it from ordinary (non-virtual) lookup.
	OPOBJ* Duplicate() const override { return new TESTOPERAND(); }
	INT GetOperandType() const override { return 0; }
};

TEST_CASE("OPERAND GetOpType reports TypeOperand", "[operand]") {
	TESTOPERAND op;
	REQUIRE(op.GetOpType() == TypeOperand);
}

TEST_CASE("OPERAND SetAttributes/GetAttributes round-trips", "[operand]") {
	TESTOPERAND op;
	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("TITLE"));
	op.SetAttributes(attrs);

	ATTRLIST out;
	op.GetAttributes(&out);
	STRING Name;
	out.AttrGetFieldName(&Name);
	REQUIRE(Name == "TITLE");
}

TEST_CASE("OPERAND operator= copies the other operand's attributes", "[operand]") {
	TESTOPERAND source;
	ATTRLIST attrs;
	attrs.AttrSetFieldName(STRING("AUTHOR"));
	source.SetAttributes(attrs);

	TESTOPERAND dest;
	// Assign through an OPOBJ& so this dispatches virtually to
	// OPERAND::operator=(const OPOBJ&) -- assigning two TESTOPERANDs
	// directly would instead resolve to the compiler-generated same-type
	// assignment operator (an exact-match overload beats the inherited
	// virtual one), which wouldn't actually exercise OPERAND's own code.
	OPOBJ& DestRef = dest;
	DestRef = source;

	ATTRLIST out;
	dest.GetAttributes(&out);
	STRING Name;
	out.AttrGetFieldName(&Name);
	REQUIRE(Name == "AUTHOR");
}
