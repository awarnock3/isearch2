// Tests for doctype/doctype.hxx / doctype/doctype.cxx (class DOCTYPE -
// base class for every document-type parser).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_OBJS
// in the top-level Makefile, not reimplemented here.
//
// DOCTYPE isn't abstract (every virtual has a default body), but its
// constructor takes an IDBOBJ*, and IDBOBJ has 3 pure virtuals
// (DfdtAddEntry/IsStopWord/ParseWords) -- same situation as
// tests/src/test_filemap.cxx's TESTIDBOBJ, reused here with a
// configurable IsStopWord/GetFieldData so ParseWords()/Present() can be
// exercised against controlled data instead of just linking.

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
#include "doctype.hxx"

#include <cstring>
#include <strings.h>

namespace {

// Stands in for a real IDBOBJ/IDB. IsStopWord treats a fixed set of
// short words as "stop words" (skip); GetFieldData optionally returns
// a canned value for one field name, so Present()/GetMetadata()'s
// Db->GetFieldData() branch is exercised too, not just linked.
class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}

	INT IsStopWord(CHR* WordStart, INT WordMaximum) const override {
		static const char* stopwords[] = {"the", "a", "an"};
		for (const char* w : stopwords) {
			SIZE_T len = strlen(w);
			if ((SIZE_T)WordMaximum >= len &&
			    strncasecmp(WordStart, w, len) == 0 &&
			    ((SIZE_T)WordMaximum == len || !IsAlnum(WordStart[len]))) {
				return 1;
			}
		}
		return 0;
	}

	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override {
		return 0;
	}

	GDT_BOOLEAN GetFieldData(const RESULT&, const STRING& FieldName,
				 STRING* StringBuffer) const override {
		if (FieldName.Equals(CannedFieldName) && CannedFieldName.GetLength() > 0) {
			*StringBuffer = CannedFieldValue;
			return GDT_TRUE;
		}
		return GDT_FALSE;
	}

	STRING CannedFieldName;
	STRING CannedFieldValue;
};

}  // namespace

TEST_CASE("DOCTYPE ReplaceWithSpace lowercases and blanks non-alnum bytes", "[doctype]") {
	char buf[32];
	strcpy(buf, "Hello, World! 123");
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	dt.ReplaceWithSpace(buf, (INT)strlen(buf));
	REQUIRE(std::string(buf) == "hello  world  123");
}

TEST_CASE("DOCTYPE ReplaceWithSpace null-terminates at data[length]", "[doctype]") {
	// Documents the buffer contract added to doctype.hxx this turn:
	// callers must size data with at least length+1 bytes.
	char buf[8] = "abcdefg";  // 7 chars + implicit '\0' at buf[7]
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	dt.ReplaceWithSpace(buf, 7);
	REQUIRE(buf[7] == '\0');
}

TEST_CASE("DOCTYPE ParseNumeric parses a numeric string", "[doctype]") {
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	REQUIRE(dt.ParseNumeric("42.5") == Catch::Approx(42.5));
}

TEST_CASE("DOCTYPE ParseNumeric returns 0 for a non-numeric string", "[doctype]") {
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	REQUIRE(dt.ParseNumeric("not-a-number") == 0.0);
}

TEST_CASE("DOCTYPE ParseWords finds word start offsets, skipping stop words", "[doctype]") {
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	char text[] = "the cat sat";
	GPTYPE gps[8];
	GPTYPE n = dt.ParseWords(text, (INT)strlen(text), 0, gps, 8);
	// "the" is a stop word (skipped); "cat" at offset 4, "sat" at offset 8.
	REQUIRE(n == 2);
	REQUIRE(gps[0] == 4u);
	REQUIRE(gps[1] == 8u);
}

TEST_CASE("DOCTYPE ParseWords returns (GPTYPE)-1 sentinel on overflow instead of aborting", "[doctype]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#doctypedoctypecxx): this used
	// to call exit(1), killing the whole process. INDEX::BuildGpList
	// (src/index.cxx) already expects and handles a (GPTYPE)-1 return.
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	char text[] = "cat sat on mat";
	GPTYPE gps[1];  // room for only 1 GP, but 4 words match
	GPTYPE n = dt.ParseWords(text, (INT)strlen(text), 0, gps, 1);
	REQUIRE(n == (GPTYPE)-1);
}

TEST_CASE("DOCTYPE Present with ElementSet \"B\" returns the file name", "[doctype]") {
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	RESULT r;
	r.SetPathName(STRING("/tmp/"));
	r.SetFileName(STRING("doc.txt"));
	STRING out;
	dt.Present(r, STRING("B"), &out);
	REQUIRE(out == "doc.txt");
}

TEST_CASE("DOCTYPE Present with an unrecognized ElementSet defers to Db::GetFieldData", "[doctype]") {
	TESTIDBOBJ db;
	db.CannedFieldName = "author";
	db.CannedFieldValue = "Jane Doe";
	DOCTYPE dt(&db);
	RESULT r;
	STRING out;
	dt.Present(r, STRING("author"), &out);
	REQUIRE(out == "Jane Doe");
}

TEST_CASE("DOCTYPE Present returns empty when Db::GetFieldData finds nothing", "[doctype]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#doctypedoctypecxx): confirms
	// the not-found path still leaves StringBufferPtr at "" now that the
	// dead FieldName/unchecked Status variables are gone.
	TESTIDBOBJ db;
	DOCTYPE dt(&db);
	RESULT r;
	STRING out("stale value");
	dt.Present(r, STRING("unknownfield"), &out);
	REQUIRE(out == "");
}

TEST_CASE("DOCTYPE GetMetadata clones defaults and adds a title entry", "[doctype]") {
	TESTIDBOBJ db;
	db.CannedFieldName = "title";
	db.CannedFieldValue = "A Test Document";
	DOCTYPE dt(&db);

	REGISTRY defaults(STRING("meta"));
	RECORD record;
	record.SetKey(STRING("doc1"));

	REGISTRY* meta = dt.GetMetadata(record, STRING("gils"), &defaults);
	REQUIRE(meta != nullptr);

	STRLIST position, value;
	position.AddEntry("locator");
	position.AddEntry("title");
	meta->GetData(position, &value);
	REQUIRE(value.GetTotalEntries() == 1);
	STRING s;
	value.GetEntry(1, &s);
	REQUIRE(s == "A Test Document");

	delete meta;
}
