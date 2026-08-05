// Tests for src/fct.hxx / src/fct.cxx (class FCT - Field Coordinate Table).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "fct.hxx"

#include <cstdio>
#include <sstream>

static FC MakeFc(GPTYPE Start, GPTYPE End) {
	FC fc;
	fc.SetFieldStart(Start);
	fc.SetFieldEnd(End);
	return fc;
}

TEST_CASE("FCT starts empty", "[fct]") {
	FCT fct;
	REQUIRE(fct.GetTotalEntries() == 0);
}

TEST_CASE("FCT AddEntry grows the count and preserves insertion order", "[fct]") {
	FCT fct;
	fct.AddEntry(MakeFc(10u, 20u));
	fct.AddEntry(MakeFc(30u, 40u));
	REQUIRE(fct.GetTotalEntries() == 2);

	FC out;
	fct.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 10u);
	REQUIRE(out.GetFieldEnd() == 20u);

	fct.GetEntry(2, &out);
	REQUIRE(out.GetFieldStart() == 30u);
	REQUIRE(out.GetFieldEnd() == 40u);
}

TEST_CASE("FCT GetEntry leaves its output untouched for an out-of-range index", "[fct]") {
	FCT fct;
	fct.AddEntry(MakeFc(1u, 2u));

	FC out = MakeFc(111u, 222u);
	fct.GetEntry(99, &out);
	REQUIRE(out.GetFieldStart() == 111u);
	REQUIRE(out.GetFieldEnd() == 222u);
}

TEST_CASE("FCT SortByFc orders entries by FieldStart ascending", "[fct]") {
	FCT fct;
	fct.AddEntry(MakeFc(30u, 31u));
	fct.AddEntry(MakeFc(10u, 11u));
	fct.AddEntry(MakeFc(20u, 21u));

	fct.SortByFc();

	FC out;
	fct.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 10u);
	fct.GetEntry(2, &out);
	REQUIRE(out.GetFieldStart() == 20u);
	fct.GetEntry(3, &out);
	REQUIRE(out.GetFieldStart() == 30u);
}

TEST_CASE("FCT Write/Read round-trips every entry through a file", "[fct]") {
	FCT original;
	original.AddEntry(MakeFc(5u, 9u));
	original.AddEntry(MakeFc(100u, 200u));

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	FCT restored;
	restored.Read(fp);
	fclose(fp);

	REQUIRE(restored.GetTotalEntries() == 2);
	FC out;
	restored.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 5u);
	REQUIRE(out.GetFieldEnd() == 9u);
	restored.GetEntry(2, &out);
	REQUIRE(out.GetFieldStart() == 100u);
	REQUIRE(out.GetFieldEnd() == 200u);
}

TEST_CASE("FCT operator<< prints every entry via FC::operator<<", "[fct]") {
	FCT fct;
	fct.AddEntry(MakeFc(5u, 9u));
	fct.AddEntry(MakeFc(1u, 2u));
	std::ostringstream os;
	os << fct;
	REQUIRE(os.str() == "5 9\n1 2\n");
}

TEST_CASE("FCT SubtractOffset shifts every entry", "[fct]") {
	FCT fct;
	fct.AddEntry(MakeFc(100u, 150u));
	fct.AddEntry(MakeFc(200u, 250u));

	fct.SubtractOffset(50u);

	FC out;
	fct.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 50u);
	REQUIRE(out.GetFieldEnd() == 100u);
	fct.GetEntry(2, &out);
	REQUIRE(out.GetFieldStart() == 150u);
	REQUIRE(out.GetFieldEnd() == 200u);
}

TEST_CASE("FCT operator= deep-copies entries independently of the source", "[fct]") {
	FCT original;
	original.AddEntry(MakeFc(7u, 8u));
	original.AddEntry(MakeFc(9u, 10u));

	FCT copy;
	copy = original;
	REQUIRE(copy.GetTotalEntries() == 2);

	copy.SubtractOffset(1u);

	FC out;
	copy.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 6u);
	original.GetEntry(1, &out);
	REQUIRE(out.GetFieldStart() == 7u);
}
