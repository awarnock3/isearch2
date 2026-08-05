// Tests for src/fc.hxx / src/fc.cxx (class FC - Field Coordinates).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "fc.hxx"

#include <cstdio>
#include <sstream>

TEST_CASE("FC field-start round-trips through Set/Get", "[fc]") {
	FC fc;
	fc.SetFieldStart(12345u);
	REQUIRE(fc.GetFieldStart() == 12345u);
}

TEST_CASE("FC field-end round-trips through Set/Get", "[fc]") {
	FC fc;
	fc.SetFieldEnd(67890u);
	REQUIRE(fc.GetFieldEnd() == 67890u);
}

TEST_CASE("FC Write/Read round-trips both fields through a file", "[fc]") {
	FC original;
	original.SetFieldStart(100u);
	original.SetFieldEnd(200u);

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	FC restored;
	restored.Read(fp);
	fclose(fp);

	REQUIRE(restored.GetFieldStart() == 100u);
	REQUIRE(restored.GetFieldEnd() == 200u);
}

TEST_CASE("FC operator<< prints \"start end\"", "[fc]") {
	FC fc;
	fc.SetFieldStart(5u);
	fc.SetFieldEnd(9u);
	std::ostringstream os;
	os << fc;
	REQUIRE(os.str() == "5 9\n");
}

TEST_CASE("FC FlipBytes executes without throwing", "[fc]") {
	// NOTE: GpSwab (src/common.cxx) reads an uninitialized local
	// whenever CROSS_PLATFORM isn't defined -- which it never is
	// anywhere in this build -- so FlipBytes' result is presently
	// undefined. That's src/common.cxx's bug to fix on its own turn;
	// this only guards against a crash, not correctness, until then.
	FC fc;
	fc.SetFieldStart(1u);
	fc.SetFieldEnd(2u);
	REQUIRE_NOTHROW(fc.FlipBytes());
}
