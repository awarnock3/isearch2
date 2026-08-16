// Tests for doctype/gils.hxx / doctype/gils.cxx (class GILS). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// GILS's constructor just forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too, extended with a GetFieldData(RESULT, STRING, STRLIST*) override
// (IDBOBJ's default is a no-op returning GDT_FALSE) so Present()'s "B"
// element set has something to find.

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
#include "gils.hxx"

#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	STRING TitleValue;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
	GDT_BOOLEAN GetFieldData(const RESULT&, const STRING& FieldName,
				  STRLIST* StrlistBuffer) const override {
		if (FieldName == "title" && !TitleValue.Equals("")) {
			StrlistBuffer->AddEntry(TitleValue);
			return GDT_TRUE;
		}
		return GDT_FALSE;
	}
};

// A file created directly under a chosen name (not via mkstemp), so
// its extension can be controlled -- needed to exercise Present()'s
// RecordSyntax-driven extension guessing.
struct NamedFile {
	STRING Path;

	NamedFile(const STRING& InPath, const char* Contents) : Path(InPath) {
		FILE* fp = fopen(Path, "wb");
		fwrite(Contents, 1, strlen(Contents), fp);
		fclose(fp);
	}

	~NamedFile() {
		remove(Path);
	}
};

STRING UniqueBase() {
	char tmpl[] = "/tmp/isearch2_test_gils_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	remove(tmpl);  // Just wanted a unique base name.
	return STRING(tmpl);
}

}  // namespace

TEST_CASE("GILS header extension defines", "[gils]") {
	REQUIRE(strcmp(GILS_HTML_EXTENSION, "htm") == 0);
	REQUIRE(strcmp(GILS_TEXT_EXTENSION, "sut") == 0);
	REQUIRE(strcmp(GILS_SGML_EXTENSION, "sgm") == 0);
	REQUIRE(strcmp(GILS_XML_EXTENSION, "xml") == 0);
}

TEST_CASE("GILS::Present element set B returns the title field", "[gils]") {
	TESTIDBOBJ db;
	db.TitleValue = "A GILS Record Title";
	GILS gils(&db);

	RESULT r;
	STRING out;
	gils.Present(r, "B", "SUTRS", &out);
	REQUIRE(out == "A GILS Record Title");
}

TEST_CASE("GILS::Present element set B returns empty when there is no title", "[gils]") {
	TESTIDBOBJ db;
	GILS gils(&db);

	RESULT r;
	STRING out;
	gils.Present(r, "B", "SUTRS", &out);
	REQUIRE(out == "");
}

TEST_CASE("GILS::Present reads the HTML variant via HtmlRecordSyntax", "[gils]") {
	STRING base = UniqueBase();
	STRING htmlPath = base;
	htmlPath.Cat(".htm");
	NamedFile htmlFile(htmlPath, "<html>gils html content</html>");

	TESTIDBOBJ db;
	GILS gils(&db);
	RESULT r;
	r.SetPathName("");
	STRING fakeName = base;
	fakeName.Cat(".xml");
	r.SetFileName(fakeName);

	STRING out;
	gils.Present(r, "F", HtmlRecordSyntax, &out);
	REQUIRE(out == "<html>gils html content</html>");
}

TEST_CASE("GILS::Present reads the SGML variant via SgmlRecordSyntax", "[gils]") {
	// Regression for the dead-code cleanup (docs/BUG_CATALOG.md#doctypegilscxx):
	// SgmlRecordSyntax used to be checked twice in a row (the second
	// check unreachable); removing the duplicate must not disturb the
	// first (live) SgmlRecordSyntax branch's behavior.
	STRING base = UniqueBase();
	STRING sgmlPath = base;
	sgmlPath.Cat(".sgm");
	NamedFile sgmlFile(sgmlPath, "<sgml>gils sgml content</sgml>");

	TESTIDBOBJ db;
	GILS gils(&db);
	RESULT r;
	r.SetPathName("");
	STRING fakeName = base;
	fakeName.Cat(".xml");
	r.SetFileName(fakeName);

	STRING out;
	gils.Present(r, "F", SgmlRecordSyntax, &out);
	REQUIRE(out == "<sgml>gils sgml content</sgml>");
}

TEST_CASE("GILS::Present reads the XML variant by default (no explicit XmlRecordSyntax branch)", "[gils]") {
	STRING base = UniqueBase();
	STRING xmlPath = base;
	xmlPath.Cat(".xml");
	NamedFile xmlFile(xmlPath, "<xml>gils xml content</xml>");

	TESTIDBOBJ db;
	GILS gils(&db);
	RESULT r;
	r.SetPathName("");
	STRING fakeName = base;
	fakeName.Cat(".htm");
	r.SetFileName(fakeName);

	STRING out;
	gils.Present(r, "F", XmlRecordSyntax, &out);
	REQUIRE(out == "<xml>gils xml content</xml>");
}

TEST_CASE("GILS::Present reports a missing file rather than crashing", "[gils]") {
	TESTIDBOBJ db;
	GILS gils(&db);
	RESULT r;
	r.SetPathName("/tmp/");
	r.SetFileName("isearch2_test_gils_does_not_exist.xml");

	STRING out;
	gils.Present(r, "F", "SUTRS", &out);
	REQUIRE(out == "File not found");
}
