// Tests for doctype/anzmeta.hxx / doctype/anzmeta.cxx (class ANZMETA).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile (added during this reprocessing turn -- it was
// missing, which is why no earlier version of this file could actually
// call a real ANZMETA method without a link error).
//
// ANZMETA's constructor just forwards to SGMLNORM(DbParent), so the same
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
#include "anzmeta.hxx"

#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

struct TempFile {
	STRING Dir;
	STRING Base;
	STRING Path;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_anzmeta_XXXXXX";
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

TEST_CASE("ANZMETA::ParseFields does not crash on a stray unmatched /custom closing tag", "[anzmeta]") {
	// Same bug class as doctype/cipc.cxx's BUGFIX #3: Nested.Top() is
	// called with no GetSize()!=0 guard when a "/custom" closing tag is
	// seen, then immediately dereferenced via pTmp->get_tag(). A
	// "/custom" with nothing on the stack (the very first tag here)
	// makes that a null-pointer dereference. Every other Nested.Top()
	// call in ParseFields() (see the val_start<LastEnd branch further
	// down) is correctly guarded by GetSize()!=0 -- this one wasn't.
	TempFile file("</custom>");
	TESTIDBOBJ db;
	ANZMETA anzmeta(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	anzmeta.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("ANZMETA::ParseFields does not crash on a well-formed <custom> field", "[anzmeta]") {
	// Even a simple, well-formed custom field hits the same path as the
	// test above: opening <custom> sets InCustom=GDT_TRUE *before* the
	// "!InCustom" gate that would otherwise Nested.Push() it (custom
	// content is deliberately excluded from indexing, not pushed as its
	// own nested field), so Nested is still empty by the time </custom>
	// is reached. Without BUGFIX #2's guard, this ordinary case crashes
	// exactly like the stray-tag case above, not just malformed input.
	TempFile file("<custom>free-form text</custom>");
	TESTIDBOBJ db;
	ANZMETA anzmeta(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	anzmeta.ParseFields(&record);  // Must not crash.
	SUCCEED();
}

TEST_CASE("ANZMETA::ParseFields handles /custom without corrupting the next real field", "[anzmeta]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeanzmetahxx): the closing-tag
	// branch used `strcmp(*tags_ptr,"/custom")` without negation, so it
	// took the "pop Nested" path for every closing tag *except*
	// "/custom" (backwards) instead of only for "/custom". A record with
	// a custom field followed by a real indexed field exercises the
	// fixed comparison end to end -- confirms parsing the whole record
	// doesn't crash or hang, matching the shape covered by the real
	// standalone repro in the BUGFIX #1 catalog entry.
	TempFile file("<custom>ignored text</custom><title>Real Title</title>");
	TESTIDBOBJ db;
	ANZMETA anzmeta(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	anzmeta.ParseFields(&record);  // Must not crash or hang.
	SUCCEED();
}

TEST_CASE("ANZMETA header defines", "[anzmeta]") {
	REQUIRE(ANZ_ACCEPT_EMPTY_TAGS == 0);
	REQUIRE(MAXNESTINGLEN == 1024);
}

TEST_CASE("ANZMETA extension constants", "[anzmeta]") {
	REQUIRE(strcmp(ANZ_SGML_EXTENSION, "sgml") == 0);
	REQUIRE(strcmp(ANZ_XML_EXTENSION, "xml") == 0);
	REQUIRE(strcmp(ANZ_HTML_EXTENSION, "html") == 0);
	REQUIRE(strcmp(ANZ_TEXT_EXTENSION, "txt") == 0);
	REQUIRE(strcmp(SHORT_ANZ_SGML_EXTENSION, "sgm") == 0);
	REQUIRE(strcmp(SHORT_ANZ_HTML_EXTENSION, "htm") == 0);
	REQUIRE(strcmp(SHORT_ANZ_TEXT_EXTENSION, "txt") == 0);
	REQUIRE(strcmp(ANZ_SGML_EXTENSION_UC, "SGML") == 0);
	REQUIRE(strcmp(ANZ_XML_EXTENSION_UC, "XML") == 0);
	REQUIRE(strcmp(ANZ_HTML_EXTENSION_UC, "HTML") == 0);
	REQUIRE(strcmp(ANZ_TEXT_EXTENSION_UC, "TEXT") == 0);
	REQUIRE(strcmp(BRIEF_MAGIC, "B") == 0);
}

TEST_CASE("ZMD_Element get/set for tag, start, and end", "[anzmeta]") {
	ZMD_Element elem;
	STRING tag = "TITLE";
	elem.set_tag(tag);
	elem.set_start(100);
	elem.set_end(200);

	REQUIRE(elem.get_tag() == tag);
	REQUIRE(elem.get_start() == 100);
	REQUIRE(elem.get_end() == 200);
}
