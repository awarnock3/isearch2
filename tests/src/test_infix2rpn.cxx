// Tests for src/infix2rpn.hxx / src/infix2rpn.cxx (class INFIX2RPN).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "strstack.hxx"
#include "tokengen.hxx"
#include "infix2rpn.hxx"

TEST_CASE("INFIX2RPN converts a simple two-term query to RPN", "[infix2rpn]") {
	INFIX2RPN p;
	STRING out;
	p.Parse(STRING("a AND b"), &out);
	// Parse()'s final flush loop only puts spaces *between* popped
	// operators, so the very last token in the output has no trailing
	// space.
	REQUIRE(out == "a b AND");
	REQUIRE(p.InputParsedOK() == GDT_TRUE);
}

TEST_CASE("INFIX2RPN honors parens over operator order", "[infix2rpn]") {
	INFIX2RPN p;
	STRING out;
	p.Parse(STRING("(a OR b) AND c"), &out);
	REQUIRE(out == "a b OR c AND");
}

TEST_CASE("INFIX2RPN recognizes OR/ANDNOT/NEAR and their symbolic aliases", "[infix2rpn]") {
	INFIX2RPN p;
	STRING out;
	p.Parse(STRING("a || b"), &out);
	REQUIRE(out == "a b OR");
	p.Parse(STRING("a &! b"), &out);
	REQUIRE(out == "a b ANDNOT");
	p.Parse(STRING("a NEAR b"), &out);
	REQUIRE(out == "a b NEAR");
}

TEST_CASE("INFIX2RPN inserts the default operator between adjacent terms", "[infix2rpn]") {
	INFIX2RPN p;
	STRING out;
	p.Parse(STRING("a b"), &out);
	REQUIRE(out == "a b AND");
}

TEST_CASE("INFIX2RPN 3-arg constructor honors a custom default operator", "[infix2rpn]") {
	STRING out;
	INFIX2RPN p(STRING("a b"), &out, "OR");
	REQUIRE(out == "a b OR");
	CHR op[MAX_OP_LEN];
	p.GetDefaultOp(op);
	REQUIRE(STRING(op) == "OR");
}

TEST_CASE("INFIX2RPN 3-arg constructor does not overflow on an oversized Op", "[infix2rpn]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcinfix2rpncxx): this
	// constructor used to strcpy(DefaultOp, Op) with no length check,
	// overflowing the fixed 8-byte DefaultOp buffer -- confirmed with a
	// standalone ASan repro before fixing. Op here is well past
	// MAX_OP_LEN (8), so this is a regression test for that overflow;
	// running under `make tests-asan` is what actually verifies it.
	STRING out;
	INFIX2RPN p(STRING("a b"), &out,
		    "THIS_OP_NAME_IS_DEFINITELY_LONGER_THAN_EIGHT_BYTES");
	// SetDefaultOp()'s documented fallback for an oversized Op is "AND".
	CHR op[MAX_OP_LEN];
	p.GetDefaultOp(op);
	REQUIRE(STRING(op) == "AND");
	REQUIRE(out == "a b AND");
}

TEST_CASE("INFIX2RPN default constructor starts with a deterministic parse state", "[infix2rpn]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcinfix2rpncxx): TermsWithNoOps
	// used to be left uninitialized by this constructor (the other two
	// indirectly zero it via Parse()). InputParsedOK() reads it, so
	// calling it before any Parse() call used to read garbage.
	INFIX2RPN p;
	REQUIRE(p.InputParsedOK() == GDT_FALSE);
}

TEST_CASE("INFIX2RPN SetDefaultOp falls back to AND when Op is too long", "[infix2rpn]") {
	INFIX2RPN p;
	p.SetDefaultOp("WAYTOOLONGANOPNAME");
	CHR op[MAX_OP_LEN];
	p.GetDefaultOp(op);
	REQUIRE(STRING(op) == "AND");
}

TEST_CASE("INFIX2RPN GetErrorMessage is empty when nothing was registered", "[infix2rpn]") {
	INFIX2RPN p;
	STRING out;
	p.Parse(STRING("a AND b"), &out);
	STRING err;
	REQUIRE(p.GetErrorMessage(&err) == GDT_FALSE);
}
