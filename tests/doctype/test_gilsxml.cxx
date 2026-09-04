// Tests for doctype/gilsxml.hxx / doctype/gilsxml.cxx (class
// GILSXML). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// GILSXML's constructor just forwards to SGMLTAG(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too, extended with both GetFieldData() overloads GILSXML relies
// on: the 4-arg (FieldName, FieldType, STRING*) one used by the
// Present_<SYNTAX>_<SET>() helpers, and the 3-arg (FieldName, STRING*)
// one used by Present()'s "B" path (via DOCTYPE::Present()).

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
#include "sgmltag.hxx"
#include "gilsxml.hxx"

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
				  const STRING&, STRING* StringBuffer) const override {
		if (FieldName == "TITLE" && !TitleValue.Equals("")) {
			*StringBuffer = TitleValue;
			return GDT_TRUE;
		}
		return GDT_FALSE;
	}

	GDT_BOOLEAN GetFieldData(const RESULT&, const STRING& FieldName,
				  STRING* StringBuffer) const override {
		if (FieldName == "title" && !TitleValue.Equals("")) {
			*StringBuffer = TitleValue;
			return GDT_TRUE;
		}
		return GDT_FALSE;
	}
};

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

STRING UniquePath() {
	char tmpl[] = "/tmp/isearch2_test_gilsxml_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	return STRING(tmpl);
}

const char* kCentroidContent =
	"Title line\n"
	"<CENTROID>\n"
	"SECRET_CENTROID_COORDINATES\n"
	"</CENTROID>\n"
	"Body line after centroid\n";

RESULT MakeResult(const STRING& Path) {
	RESULT r;
	r.SetPathName("");
	r.SetFileName(Path);
	return r;
}

}  // namespace

TEST_CASE("GILSXML::Present element set B returns the title when present", "[gilsxml]") {
	TESTIDBOBJ db;
	db.TitleValue = "A GILS Title";
	GILSXML gx(&db);

	RESULT r;
	STRING out;
	gx.Present(r, "B", "SUTRS", &out);
	REQUIRE(out == "A GILS Title");
}

TEST_CASE("GILSXML::Present element set B falls back to the filename when there is no title", "[gilsxml]") {
	TESTIDBOBJ db;
	GILSXML gx(&db);

	RESULT r;
	r.SetPathName("/some/dir/");
	r.SetFileName("record.xml");
	STRING out;
	gx.Present(r, "B", "SUTRS", &out);
	REQUIRE(out == "record.xml");
}

TEST_CASE("GILSXML::Present_HTML_S does not duplicate the header into the body", "[gilsxml]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypegilsxmlcxx): Hold was reused
	// via Cat() without being reset after its header content was
	// already transferred into ESN_F, so the header text (this time
	// HTML-escaped) leaked into the <pre> body a second time. The
	// escaped "&lt;H1&gt;" is the direct signature of that bug -- it
	// must never appear in correct output, since <H1> only ever
	// belongs in the unescaped header.
	STRING path = UniquePath();
	NamedFile file(path, kCentroidContent);
	TESTIDBOBJ db;
	db.TitleValue = "S Test Title";
	GILSXML gx(&db);

	RESULT r = MakeResult(path);
	STRING out;
	gx.Present(r, "S", HtmlRecordSyntax, &out);

	REQUIRE(out.Search("&lt;H1&gt;") == 0);
	REQUIRE(out.Search("<H1>") != 0);
}

TEST_CASE("GILSXML::Present_HTML_S strips the CENTROID section", "[gilsxml]") {
	STRING path = UniquePath();
	NamedFile file(path, kCentroidContent);
	TESTIDBOBJ db;
	GILSXML gx(&db);

	RESULT r = MakeResult(path);
	STRING out;
	gx.Present(r, "S", HtmlRecordSyntax, &out);

	REQUIRE(out.Search("SECRET_CENTROID_COORDINATES") == 0);
	REQUIRE(out.Search("Body line after centroid") != 0);
}

TEST_CASE("GILSXML::Present_SUTRS_S strips the CENTROID section like its HTML/SGML siblings", "[gilsxml]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypegilsxmlcxx): this used to
	// delegate to Present_SGML_F() (full record, centroid included)
	// instead of Present_SGML_S() (centroid stripped) -- confirmed here
	// by asserting the centroid marker is absent, matching
	// Present_HTML_S()'s and Present_SGML_S()'s behavior.
	STRING path = UniquePath();
	NamedFile file(path, kCentroidContent);
	TESTIDBOBJ db;
	GILSXML gx(&db);

	RESULT r = MakeResult(path);
	STRING out;
	gx.Present(r, "S", "SUTRS", &out);

	REQUIRE(out.Search("SECRET_CENTROID_COORDINATES") == 0);
	REQUIRE(out.Search("Body line after centroid") != 0);
}

TEST_CASE("GILSXML::Present_SUTRS_F still returns the full record, centroid included", "[gilsxml]") {
	// Contrast case: "F" (full) must still include the centroid --
	// only "S" (short) strips it. Confirms BUGFIX #2's fix didn't touch
	// the (already-correct) "F" path.
	STRING path = UniquePath();
	NamedFile file(path, kCentroidContent);
	TESTIDBOBJ db;
	GILSXML gx(&db);

	RESULT r = MakeResult(path);
	STRING out;
	gx.Present(r, "F", "SUTRS", &out);

	REQUIRE(out.Search("SECRET_CENTROID_COORDINATES") != 0);
}
