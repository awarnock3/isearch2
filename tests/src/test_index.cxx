// Tests for src/index.hxx / src/index.cxx (class INDEX).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// INDEX's constructor tolerates a minimal IDBOBJ: ComposeDbFn/ffopen
// both have harmless default bodies (ffopen returns nullptr), so the
// constructor takes the "file not found" branch and returns cleanly --
// same TESTIDBOBJ shape as tests/src/test_filemap.cxx, just without
// needing a real MDT. Search/AddRecordList/merge methods need a full
// on-disk index (DFDT, MDT, postings files) to exercise meaningfully
// and aren't covered here; this focuses on the bugs fixed this turn.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
#include "idbobj.hxx"
#include "index.hxx"

#include <cstring>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

}  // namespace

TEST_CASE("INDEX constructor safely initializes pointer members", "[index]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srcindexcxx): SetCache,
	// DocTypePtr, and TheThesaurus used to be left uninitialized.
	// GetDocTypePtr() is the one public accessor that exposes one of
	// them directly, so it's the observable proxy for this fix: it must
	// read nullptr right after construction, not garbage.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_index_ctor"));
	REQUIRE(index.GetDocTypePtr() == nullptr);
}

TEST_CASE("INDEX::IsStopWord recognizes stop words case-insensitively", "[index]") {
	// BUGFIX #7 (see docs/BUG_CATALOG.md#srcindexcxx): this used to
	// unconditionally `return 0` before the binary-search lookup ever
	// ran, so every word -- stop word or not -- came back as "not a
	// stop word". These cases would all have failed before the fix.
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_index_stopword"));

	char w1[] = "the";
	REQUIRE(index.IsStopWord(w1, 3) == 1);

	char w2[] = "The";  // case-insensitive match
	REQUIRE(index.IsStopWord(w2, 3) == 1);

	char w3[] = "a";
	REQUIRE(index.IsStopWord(w3, 1) == 1);

	char w4[] = "5";  // digits are in stoplist too
	REQUIRE(index.IsStopWord(w4, 1) == 1);
}

TEST_CASE("INDEX::IsStopWord returns 0 for ordinary words", "[index]") {
	TESTIDBOBJ db;
	INDEX index(&db, STRING("/tmp/isearch2_test_index_notstopword"));

	char w[] = "aardvark";
	REQUIRE(index.IsStopWord(w, (INT)strlen(w)) == 0);
}
