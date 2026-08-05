// Tests for src/opstack.hxx / src/opstack.cxx (class OPSTACK -
// Operand/operator Stack). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"

TEST_CASE("OPSTACK Pop on an empty stack returns nullptr", "[opstack]") {
	OPSTACK s;
	POPOBJ p;
	REQUIRE((s >> p) == nullptr);
	REQUIRE(p == nullptr);
}

TEST_CASE("OPSTACK push-by-reference duplicates, and pops LIFO", "[opstack]") {
	OPSTACK s;
	IRSET a(nullptr), b(nullptr);
	s << a;
	s << b;

	POPOBJ p1, p2, p3;
	s >> p1;
	s >> p2;
	REQUIRE((s >> p3) == nullptr);
	REQUIRE(p3 == nullptr);
	// p1/p2 are OPSTACK's own duplicates, not &a/&b -- distinct objects.
	REQUIRE(p1 != (OPOBJ*)&a);
	REQUIRE(p2 != (OPOBJ*)&b);
	delete p1;
	delete p2;
}

TEST_CASE("OPSTACK push-by-pointer takes ownership directly", "[opstack]") {
	OPSTACK s;
	IRSET* p = new IRSET(nullptr);
	s << p;

	POPOBJ out;
	s >> out;
	REQUIRE(out == (OPOBJ*)p);
	delete out;
}

TEST_CASE("OPSTACK destructor frees remaining entries, not a leak", "[opstack]") {
	// BUGFIX #3 coverage: before the fix, ~OPSTACK() was empty, so any
	// entries still on the stack at destruction time leaked -- confirmed
	// real with a standalone LeakSanitizer repro. This test deliberately
	// leaves entries un-popped when `s` goes out of scope; passing under
	// make tests-asan (which enables LeakSanitizer by default alongside
	// ASan) is the point.
	{
		OPSTACK s;
		IRSET a(nullptr), b(nullptr);
		s << a;
		s << b;
	}
	SUCCEED("stack destroyed without leaking");
}

TEST_CASE("OPSTACK copy constructor deep-copies independently of the source", "[opstack]") {
	// BUGFIX #2 coverage: before the fix, OPSTACK declared no copy
	// constructor, so `OPSTACK b = a;` used the compiler-generated
	// shallow copy of Head -- both stacks would share the same node
	// chain, and (once BUGFIX #3 made ~OPSTACK() actually free entries)
	// destroying either would double-free the shared nodes the other
	// still pointed to.
	OPSTACK original;
	IRSET a(nullptr), b(nullptr);
	original << a;
	original << b;

	OPSTACK copy = original;

	POPOBJ p;
	INT copyCount = 0;
	while (copy >> p) {
		delete p;
		copyCount++;
	}
	REQUIRE(copyCount == 2);

	// original's own chain must still be intact and independently valid.
	INT originalCount = 0;
	while (original >> p) {
		delete p;
		originalCount++;
	}
	REQUIRE(originalCount == 2);
}

TEST_CASE("OPSTACK operator= drains the destination before deep-copying the source", "[opstack]") {
	OPSTACK original;
	IRSET a(nullptr);
	original << a;

	OPSTACK dest;
	IRSET placeholder1(nullptr), placeholder2(nullptr);
	dest << placeholder1;
	dest << placeholder2;

	dest = original;

	POPOBJ p;
	INT count = 0;
	while (dest >> p) {
		delete p;
		count++;
	}
	REQUIRE(count == 1);
}

TEST_CASE("OPSTACK Reverse flips pop order", "[opstack]") {
	OPSTACK s;
	IRSET* a = new IRSET(nullptr);
	IRSET* b = new IRSET(nullptr);
	IRSET* c = new IRSET(nullptr);
	s << a;  // pointer overload: takes ownership, LIFO pop order is c,b,a
	s << b;
	s << c;

	s.Reverse();  // pop order is now a,b,c

	POPOBJ p;
	s >> p;
	REQUIRE(p == (OPOBJ*)a);
	delete p;
	s >> p;
	REQUIRE(p == (OPOBJ*)b);
	delete p;
	s >> p;
	REQUIRE(p == (OPOBJ*)c);
	delete p;
}

TEST_CASE("OPSTACK operator>>(PIRSET&) pops and downcasts an IRSET", "[opstack]") {
	OPSTACK s;
	IRSET a(nullptr);
	s << a;

	PIRSET out;
	s >> out;
	REQUIRE(out != nullptr);
	delete out;
}
