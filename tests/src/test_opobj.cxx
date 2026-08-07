// Tests for src/opobj.hxx / src/opobj.cxx (class OPOBJ - the base
// class every OPSTACK node, leaf operand or operator, derives from).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// OPOBJ is abstract (four pure virtuals), so these tests exercise it
// through a minimal concrete subclass that implements only those four,
// leaving every other method at OPOBJ's own default. Next/SetNext/
// GetNext are private to OPOBJ with `friend class OPSTACK;` only, so
// they aren't directly testable from here -- see
// tests/src/test_opstack.cxx for coverage of OPSTACK's push/pop/Reverse
// logic, which is the only code that ever reads/writes Next.

#include "catch_amalgamated.hpp"

#include "opobj.hxx"

namespace {

class TestOp : public OPOBJ {
public:
	INT GetOpType() const override { return 1; }
	INT GetOperandType() const override { return 2; }
	OPOBJ* Duplicate() const override { return new TestOp(); }
	OPOBJ& operator=(const OPOBJ&) override { return *this; }
};

}  // namespace

TEST_CASE("OPOBJ's default GetOperatorType is 0", "[opobj]") {
	TestOp op;
	REQUIRE(op.GetOperatorType() == 0);
}

TEST_CASE("OPOBJ's default GetTotalEntries is 0", "[opobj]") {
	TestOp op;
	REQUIRE(op.GetTotalEntries() == 0);
}

TEST_CASE("OPOBJ's default GetParent is nullptr", "[opobj]") {
	TestOp op;
	REQUIRE(op.GetParent() == nullptr);
}

TEST_CASE("OPOBJ's default Set*/Get*/Or/And/AndNot/Near are safe no-ops", "[opobj]") {
	TestOp op, other;
	REQUIRE_NOTHROW(op.SetTerm(STRING("term")));
	STRING s;
	REQUIRE_NOTHROW(op.GetTerm(&s));
	REQUIRE_NOTHROW(op.SetOperatorType(1));
	REQUIRE_NOTHROW(op.Or(other));
	REQUIRE_NOTHROW(op.And(other));
	REQUIRE_NOTHROW(op.AndNot(other));
	REQUIRE_NOTHROW(op.Near(other));
	REQUIRE_NOTHROW(op.SetParent(nullptr));
}

TEST_CASE("OPOBJ Duplicate() returns a working independent copy", "[opobj]") {
	TestOp op;
	OPOBJ* dup = op.Duplicate();
	REQUIRE(dup != nullptr);
	REQUIRE(dup != &op);
	REQUIRE(dup->GetOpType() == 1);
	delete dup;
}
