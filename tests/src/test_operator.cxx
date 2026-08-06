// Tests for src/operator.hxx / src/operator.cxx (class OPERATOR - Query
// Operator). Linked against the real implementation via TEST_ENGINE_OBJS
// in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "operator.hxx"

TEST_CASE("OPERATOR default-constructs with OperatorType 0", "[operator]") {
	OPERATOR op;
	REQUIRE(op.GetOperatorType() == 0);
}

TEST_CASE("OPERATOR GetOpType/GetOperandType report the expected constants", "[operator]") {
	OPERATOR op;
	REQUIRE(op.GetOpType() == TypeOperator);
	REQUIRE(op.GetOperandType() == 0);
}

TEST_CASE("OPERATOR SetOperatorType/GetOperatorType round-trips", "[operator]") {
	OPERATOR op;
	op.SetOperatorType(5);
	REQUIRE(op.GetOperatorType() == 5);
}

TEST_CASE("OPERATOR operator= copies OperatorType through the polymorphic OPOBJ& interface", "[operator]") {
	// Assigns through an OPOBJ& rather than `OPERATOR a, b; a = b;`:
	// direct same-type assignment needs the compiler-generated
	// OPERATOR::operator=(const OPERATOR&), whose base-subobject
	// assignment step bottoms out needing OPOBJ::operator=(const
	// OPOBJ&) -- pure virtual, no definition anywhere -- and fails to
	// *link*. See BUG_CATALOG.md under src/termobj.hxx, where this was
	// first confirmed, for the full explanation; every real caller in
	// this tree already goes through the polymorphic interface, as here.
	OPERATOR source;
	source.SetOperatorType(7);

	OPERATOR dest;
	OPOBJ& DestRef = dest;
	DestRef = source;

	REQUIRE(dest.GetOperatorType() == 7);
}

TEST_CASE("OPERATOR Duplicate creates an independent copy", "[operator]") {
	OPERATOR original;
	original.SetOperatorType(3);

	OPOBJ* copy = original.Duplicate();
	REQUIRE(copy != nullptr);
	REQUIRE(copy != (OPOBJ*)&original);
	REQUIRE(copy->GetOperatorType() == 3);

	copy->SetOperatorType(9);
	REQUIRE(original.GetOperatorType() == 3);

	delete copy;
}
