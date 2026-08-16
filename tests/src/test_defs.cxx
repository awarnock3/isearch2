// Tests for src/defs.hxx. Extern declarations are defined in defs.cxx
// (already linked into TEST_ENGINE_SRCS), so those are checked against
// their real values, not just declared-and-linkable.

#include "catch_amalgamated.hpp"

#include "defs.hxx"

#include <sstream>

TEST_CASE("GPTYPE/PGPTYPE alias UINT4 as documented", "[defs]") {
	static_assert(sizeof(GPTYPE) == sizeof(UINT4), "GPTYPE should alias UINT4");
	GPTYPE g = 42;
	PGPTYPE pg = &g;
	REQUIRE(*pg == 42);
}

TEST_CASE("extern string constants match their defs.cxx values", "[defs]") {
	REQUIRE(std::string(IsearchDefaultDbName) == "ISEARCH");
	REQUIRE(std::string(Bib1AttributeSet) == "1.2.840.10003.3.1");
	REQUIRE(std::string(GilsAttributeSet) == "1.2.840.10003.3.5");
	REQUIRE(std::string(SutrsRecordSyntax) == "SUTRS");
	REQUIRE(std::string(XmlRecordSyntax) == "XML");
	REQUIRE(std::string(DbExtIndex) == ".inx");
	REQUIRE(std::string(DbExtMdt) == ".mdt");
}

TEST_CASE("Z39.50 attribute/structure constants keep their protocol-defined values", "[defs]") {
	// A transcription slip here (this file is hand-copied from the
	// Z39.50/GILS spec, not computed) would silently desync wire
	// protocol values -- pin the ones most likely to be reached by
	// real query code.
	REQUIRE(ZdistUseAttr == 1);
	REQUIRE(ZRelEQ == 3);
	REQUIRE(ZTruncRight == 1);
	REQUIRE(ZTruncLeftRight == 3);
	REQUIRE(ZStructWord == 2);
	REQUIRE(OperatorOr == 1);
	REQUIRE(OperatorAnd == 2);
	REQUIRE(OperatorAndNot == 3);
}

TEST_CASE("DB state and indexing status enums are distinct", "[defs]") {
	REQUIRE(IsearchDbStateReady == 0);
	REQUIRE(IsearchDbStateBusy == 1);
	REQUIRE(IsearchDbStateInvalid == 2);
	REQUIRE(IndexingStatusParsingDocument != IndexingStatusIndexing);
}

TEST_CASE("COUT is std::cout, not an ambient-scope-dependent name", "[defs]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdefshxx): confirms COUT
	// resolves and actually writes through std::cout's stream, without
	// this test file itself doing `using namespace std;` anywhere.
	std::streambuf* orig = std::cout.rdbuf();
	std::ostringstream captured;
	std::cout.rdbuf(captured.rdbuf());
	COUT << "defs-test-marker";
	std::cout.rdbuf(orig);
	REQUIRE(captured.str() == "defs-test-marker");
}

// EXIT_ERROR/RETURN_ERROR/RETURN_ZERO are deliberately not exercised
// here -- they call exit()/return out of the caller, which would tear
// down the whole test binary. See docs/BUG_CATALOG.md#srcdefshxx for
// the deferred do/while(0)-hygiene finding.
