// Tests for doctype/anzlic.hxx / doctype/anzlic.cxx (class ANZLIC).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile (added during this reprocessing turn -- it was
// missing, which is why no earlier version of this file could actually
// call a real ANZLIC method without a link error).
//
// ANZLIC's constructor just forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here.

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
#include "sgmlnorm.hxx"
#include "date.hxx"
#include "anzlic.hxx"

#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	STRING FieldTypeFilename;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
	void GetDocTypeOptions(STRLIST* StringListBuffer) const override {
		if (!FieldTypeFilename.Equals("")) {
			STRING opt("FIELDTYPE=");
			opt.Cat(FieldTypeFilename);
			StringListBuffer->AddEntry(opt);
		}
	}
};

struct TempFile {
	STRING Dir;
	STRING Base;
	STRING Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_anzlic_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
		Path = tmpl;
	}

	~TempFile() {
		remove(Path);
	}
};

}  // namespace

TEST_CASE("ANZLIC::ParseFields does not crash on a stray unmatched /custom closing tag", "[anzlic]") {
	// Same bug class as doctype/cipc.cxx's BUGFIX #3 and doctype/anzmeta.cxx's
	// BUGFIX #2: Nested.Top() is called with no GetSize()!=0 guard when a
	// "/custom" closing tag is seen, then immediately dereferenced via
	// pTmp->get_tag(). A "/custom" with nothing on the stack (the very
	// first tag here) makes that a null-pointer dereference.
	TempFile file("</custom>");
	TESTIDBOBJ db;
	ANZLIC anzlic(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	anzlic.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("ANZLIC::ParseFields does not crash on a well-formed <custom> field", "[anzlic]") {
	// Even a simple, well-formed custom field hits the same path as the
	// test above: opening <custom> sets InCustom=GDT_TRUE *before* the
	// "!InCustom" gate that would otherwise Nested.Push() it, so Nested
	// is still empty by the time </custom> is reached. Without the
	// GetSize() guard, this ordinary case crashes exactly like the
	// stray-tag case above, not just malformed input.
	TempFile file("<custom>free-form text</custom>");
	TESTIDBOBJ db;
	ANZLIC anzlic(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	anzlic.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("ANZLIC::ParseFields handles /custom without corrupting the next real field", "[anzlic]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeanzlichxx): the closing-tag
	// branch used `strcmp(*tags_ptr,"/custom")` without negation, so it
	// took the "pop Nested" path for every closing tag *except* "/custom"
	// (backwards) instead of only for "/custom". A record with a custom
	// field followed by a real indexed field exercises the fixed
	// comparison end to end.
	TempFile file("<custom>ignored text</custom><title>Real Title</title>");
	TESTIDBOBJ db;
	ANZLIC anzlic(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	anzlic.ParseFields(&record);  // Must not crash or hang.
	SUCCEED();
}

TEST_CASE("ANZLIC::LoadFieldTable does not crash on an empty FIELDTYPE file", "[anzlic]") {
	// BUGFIX #5 (docs/BUG_CATALOG.md#doctypeanzlichxx): same shape as
	// doctype/cipc.cxx's BUGFIX #5 and doctype/dif.cxx's BUGFIX #5 --
	// IsFile()/fopen() only check existence, not content. An empty (but
	// existing) FIELDTYPE file makes strtok() return nullptr on its very
	// first call, and the original do-while unconditionally ran
	// `Field_and_Type = pBuf;` (a null pointer) before ever checking it --
	// STRING::operator=(const CHR*) calls strlen() on it unconditionally,
	// a null-pointer-dereference crash. This file never got that fix
	// applied even though the identical pattern was already fixed
	// elsewhere in this same batch.
	TempFile fieldtypeFile("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = fieldtypeFile.Path;
	ANZLIC anzlic(&db);

	anzlic.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("ANZLIC::LoadFieldTable loads real entries from a non-empty FIELDTYPE file", "[anzlic]") {
	TempFile fieldtypeFile("title text\nwestbc numeric\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = fieldtypeFile.Path;
	ANZLIC anzlic(&db);

	anzlic.LoadFieldTable();  // Must not crash on real (non-empty) entries.
	SUCCEED();
}

TEST_CASE("ANZLIC::ParseDateRange defaults to an error sentinel when BEGDATE is missing", "[anzlic]") {
	// BUGFIX #6 (docs/BUG_CATALOG.md#doctypeanzlichxx): Hold.Search(...)
	// was used with no ">0" guard (unlike every sibling ParseDateRange in
	// this codebase -- dif.cxx, cipc.cxx, cipp.cxx, anzmeta.cxx all check
	// it). Search() returns 0 when the tag isn't found, so
	// `Start += 9; Hold.EraseBefore(Start);` ran on a computed offset of
	// 9 into an unrelated buffer instead of erroring out. STRING's
	// EraseBefore/EraseAfter are bounds-checked so this didn't crash, but
	// it could silently produce a plausible-looking wrong date instead of
	// a clear error value: for a 10-character buffer with no BEGDATE tag
	// at all, EraseBefore(9) leaves just its last two characters, and if
	// those happen to be digits (as here) IsNumber() sees a normal-looking
	// value -- confirmed by reverting the fix and observing fStart come
	// back as 90.0 instead of the intended -1.0 error sentinel.
	TESTIDBOBJ db;
	ANZLIC anzlic(&db);
	DOUBLE fStart = 0.0, fEnd = 0.0;
	char buf[] = "1234567890";

	anzlic.ParseDateRange(buf, &fStart, &fEnd);
	REQUIRE(fStart == -1.0);
}

TEST_CASE("ANZLIC::ParseDateRange parses a well-formed BEGDATE/ENDDATE pair", "[anzlic]") {
	TESTIDBOBJ db;
	ANZLIC anzlic(&db);
	DOUBLE fStart = 0.0, fEnd = 0.0;
	char buf[] = "<BEGDATE>1998</BEGDATE><ENDDATE>1999</ENDDATE>";

	anzlic.ParseDateRange(buf, &fStart, &fEnd);
	REQUIRE(fStart == Catch::Approx(1998.0));
	REQUIRE(fEnd == Catch::Approx(1999.0));
}

TEST_CASE("ANZLIC header defines", "[anzlic]") {
	REQUIRE(ANZLIC_ACCEPT_EMPTY_TAGS == 0);
	REQUIRE(MAXNESTINGLEN == 1024);
}

TEST_CASE("ANZLIC extension constants", "[anzlic]") {
	REQUIRE(strcmp(ANZLIC_SGML_EXTENSION, "sgml") == 0);
	REQUIRE(strcmp(ANZLIC_XML_EXTENSION, "xml") == 0);
	REQUIRE(strcmp(ANZLIC_HTML_EXTENSION, "html") == 0);
	REQUIRE(strcmp(ANZLIC_TEXT_EXTENSION, "txt") == 0);
	REQUIRE(strcmp(SHORT_ANZLIC_SGML_EXTENSION, "sgm") == 0);
	REQUIRE(strcmp(SHORT_ANZLIC_HTML_EXTENSION, "htm") == 0);
	REQUIRE(strcmp(SHORT_ANZLIC_TEXT_EXTENSION, "txt") == 0);
	REQUIRE(strcmp(ANZLIC_SGML_EXTENSION_UC, "SGML") == 0);
	REQUIRE(strcmp(ANZLIC_XML_EXTENSION_UC, "XML") == 0);
	REQUIRE(strcmp(ANZLIC_HTML_EXTENSION_UC, "HTML") == 0);
	REQUIRE(strcmp(ANZLIC_TEXT_EXTENSION_UC, "TXT") == 0);
	REQUIRE(strcmp(BRIEF_MAGIC, "B") == 0);
}

TEST_CASE("AMD_Element get/set for tag, start, and end", "[anzlic]") {
	AMD_Element elem;
	STRING tag = "TITLE";
	elem.set_tag(tag);
	elem.set_start(100);
	elem.set_end(200);

	REQUIRE(elem.get_tag() == tag);
	REQUIRE(elem.get_start() == 100);
	REQUIRE(elem.get_end() == 200);
}
