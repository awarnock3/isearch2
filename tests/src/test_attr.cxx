// Tests for src/attr.hxx / src/attr.cxx (class ATTR - a single
// Z39.50/GILS-style search attribute).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "attr.hxx"

TEST_CASE("ATTR default-constructs to empty ids and zero type", "[attr]") {
	ATTR a;
	STRING s;
	a.GetSetId(&s);
	REQUIRE(s == "");
	REQUIRE(a.GetAttrType() == 0);
	a.GetAttrValue(&s);
	REQUIRE(s == "");
}

TEST_CASE("ATTR SetSetId/GetSetId round-trip", "[attr]") {
	ATTR a;
	a.SetSetId(STRING("1.2.840.10003.3.1"));
	STRING s;
	a.GetSetId(&s);
	REQUIRE(s == "1.2.840.10003.3.1");
}

TEST_CASE("ATTR SetAttrType/GetAttrType round-trip", "[attr]") {
	ATTR a;
	a.SetAttrType(1);
	REQUIRE(a.GetAttrType() == 1);
}

TEST_CASE("ATTR SetAttrValue(STRING) is readable back as a STRING", "[attr]") {
	ATTR a;
	a.SetAttrValue(STRING("hello"));
	STRING s;
	a.GetAttrValue(&s);
	REQUIRE(s == "hello");
}

TEST_CASE("ATTR SetAttrValue(INT) is readable back as both STRING and INT", "[attr]") {
	ATTR a;
	a.SetAttrValue(42);
	REQUIRE(a.GetAttrValue() == 42);
	STRING s;
	a.GetAttrValue(&s);
	REQUIRE(s == "42");
}

TEST_CASE("ATTR GetAttrValue() as INT parses a numeric STRING value", "[attr]") {
	ATTR a;
	a.SetAttrValue(STRING("123"));
	REQUIRE(a.GetAttrValue() == 123);
}

TEST_CASE("ATTR operator= copies all fields independently of the source", "[attr]") {
	ATTR a;
	a.SetSetId(STRING("set-a"));
	a.SetAttrType(3);
	a.SetAttrValue(STRING("value-a"));

	ATTR b;
	b = a;

	a.SetSetId(STRING("set-b-mutated"));
	a.SetAttrType(9);
	a.SetAttrValue(STRING("mutated"));

	STRING s;
	b.GetSetId(&s);
	REQUIRE(s == "set-a");
	REQUIRE(b.GetAttrType() == 3);
	b.GetAttrValue(&s);
	REQUIRE(s == "value-a");
}

TEST_CASE("ATTR operator= self-assignment leaves fields intact", "[attr]") {
	ATTR a;
	a.SetSetId(STRING("self"));
	a.SetAttrType(7);
	a.SetAttrValue(STRING("keep"));
	a = a;

	STRING s;
	a.GetSetId(&s);
	REQUIRE(s == "self");
	REQUIRE(a.GetAttrType() == 7);
	a.GetAttrValue(&s);
	REQUIRE(s == "keep");
}
