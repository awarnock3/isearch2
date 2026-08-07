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

TEST_CASE("FC FlipBytes round-trips both fields", "[fc]") {
	// GpSwab (src/common.cxx) used to read an uninitialized local
	// whenever CROSS_PLATFORM isn't defined -- which it never is
	// anywhere in this build -- so FlipBytes' result was undefined.
	// Fixed on src/common.cxx's own turn; see
	// docs/BUG_CATALOG.md#srccommoncxx (BUGFIX #2). Flipping twice
	// should now reliably restore the original values.
	FC fc;
	fc.SetFieldStart(1u);
	fc.SetFieldEnd(2u);
	fc.FlipBytes();
	fc.FlipBytes();
	REQUIRE(fc.GetFieldStart() == 1u);
	REQUIRE(fc.GetFieldEnd() == 2u);
}
