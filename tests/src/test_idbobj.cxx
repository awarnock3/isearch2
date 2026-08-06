// Tests for src/idbobj.hxx (class IDBOBJ - abstract database-object
// interface). IDBOBJ has no .cxx of its own; every method exercised
// here is a header-inline default. IDB (src/idb.hxx/.cxx, both still
// pending) is the only real subclass, so a minimal stand-in is used
// here to override just the pure-virtual methods.
//
// Note: BUGFIX #2 (GpFwrite/GpFread's un-overridden defaults now
// abort() instead of closing stdout/stderr) is deliberately not
// exercised here -- calling it would abort the whole test binary, not
// just fail one assertion. That fix was confirmed separately via a
// standalone repro; see docs/BUG_CATALOG.md#srcidbobjhxx.

#include "catch_amalgamated.hpp"

#include "idbobj.hxx"

namespace {

class MinimalDb : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

} // namespace

TEST_CASE("idbobj.hxx is self-contained", "[idbobj]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcidbobjhxx): this header had
	// all of its dependency includes commented out. Merely compiling
	// this translation unit is the regression test for that: it
	// wouldn't have compiled beforehand.
	MinimalDb db;
	SUCCEED();
}

TEST_CASE("IDBOBJ un-overridden defaults report inert/empty results", "[idbobj]") {
	MinimalDb db;
	REQUIRE(db.GetIndexingMemory() == 0);
	REQUIRE(db.DfdtGetTotalEntries() == 0);

	RESULT r;
	STRING out;
	REQUIRE(db.GetFieldData(r, STRING("field"), &out) == GDT_FALSE);
	REQUIRE(db.GetFieldData(r, STRING("field"), STRING("type"), &out) == GDT_FALSE);

	STRLIST sl;
	REQUIRE(db.GetFieldData(r, STRING("field"), &sl) == GDT_FALSE);

	DOUBLE d;
	REQUIRE(db.GetFieldData(r, STRING("field"), &d) == GDT_FALSE);

	REQUIRE(db.ffopen(STRING("/nonexistent"), "r") == nullptr);
	REQUIRE(db.ffclose(nullptr) == 0);
}

TEST_CASE("IDBOBJ::FieldTypes/FileNames are usable HASH members", "[idbobj]") {
	MinimalDb db;
	db.FieldTypes.AddEntry(STRING("type=text"));
	STRING out;
	db.FieldTypes.GetValue(STRING("type"), &out);
	REQUIRE(out == STRING("text"));

	db.FileNames.AddEntry(STRING("main=main.dbf"));
	db.FileNames.GetValue(STRING("main"), &out);
	REQUIRE(out == STRING("main.dbf"));
}

TEST_CASE("IDBOBJ pure-virtual methods must be provided by a subclass", "[idbobj]") {
	MinimalDb db;
	DFD dfd;
	db.DfdtAddEntry(dfd);
	REQUIRE(db.IsStopWord(nullptr, 0) == 0);
	REQUIRE(db.ParseWords(STRING("doctype"), nullptr, 0, 0, nullptr, 0) == 0);
}
