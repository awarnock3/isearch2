// Tests for doctype/fgdcsite.hxx / doctype/fgdcsite.cxx (class
// FGDCSITE). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// FGDCSITE's constructor just forwards to SGMLTAG(DbParent), so the
// same minimal TESTIDBOBJ shape used by tests/doctype/test_sgmltag.cxx
// works here too, extended with a GetDocTypeOptions() override
// (IDBOBJ's default is a no-op) so LoadFieldTable() can be pointed at
// a real FIELDTYPE file without falling into its interactive
// stdin-prompt fallback.

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
#include "fgdcsite.hxx"

#include <cstdio>
#include <cstring>
#include <string>
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
		char tmpl[] = "/tmp/isearch2_test_fgdcsite_XXXXXX";
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

// A file created directly under a chosen name (not via mkstemp), so
// its extension can be controlled -- needed to exercise Present()'s
// filename-extension-guessing logic.
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

}  // namespace

TEST_CASE("FGDCSITE header extension defines", "[fgdcsite]") {
	REQUIRE(strcmp(FGDC_SGML_EXTENSION, "sgml") == 0);
	REQUIRE(strcmp(FGDC_HTML_EXTENSION, "html") == 0);
	REQUIRE(strcmp(FGDC_TEXT_EXTENSION, "text") == 0);
}

TEST_CASE("FGDCSITE::LoadFieldTable does not crash on an empty FIELDTYPE file", "[fgdcsite]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): IsFile() only
	// checks existence, not content; an empty FIELDTYPE file made
	// strtok() return nullptr on the very first call, and the original
	// do-while unconditionally ran `Field_and_Type = pBuf;` (a null
	// pointer) before ever checking it -- STRING::operator=(const CHR*)
	// calls strlen() on it unconditionally. Same bug as, and fixed the
	// same way as, doctype/cipc.cxx's BUGFIX #5.
	TempFile file("");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	FGDCSITE fgdcsite(&db);

	fgdcsite.LoadFieldTable();  // Must not crash.
	SUCCEED();
}

TEST_CASE("FGDCSITE::LoadFieldTable loads real entries without crashing", "[fgdcsite]") {
	TempFile file("HOSTNAME TEXT\nTCPPORT NUMERIC\n");
	TESTIDBOBJ db;
	db.FieldTypeFilename = file.Path;
	FGDCSITE fgdcsite(&db);

	fgdcsite.LoadFieldTable();
	SUCCEED();
}

TEST_CASE("FGDCSITE::Present element set B falls back to a placeholder when no field data", "[fgdcsite]") {
	// IDBOBJ::GetFieldData()'s default (unoverridden) implementation
	// returns GDT_FALSE without touching ResultRecord, so this is safe
	// to exercise even with a default-constructed RESULT (unlike
	// GetRecordData(), which crashes on one -- see the other doctype/
	// tests' notes on that).
	TESTIDBOBJ db;
	FGDCSITE fgdcsite(&db);
	RESULT r;
	STRING out;
	fgdcsite.Present(r, "B", "SUTRS", &out);
	REQUIRE(out == "(title not found)");
}

TEST_CASE("FGDCSITE::Present finds the SGML variant of a record via SgmlRecordSyntax", "[fgdcsite]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefgdcsitecxx): the
	// SgmlRecordSyntax branch used to be an exact duplicate of the
	// HtmlRecordSyntax check above it, so it could never match --
	// requesting the SGML variant of a record silently fell through to
	// using the unmodified (wrong-extension) filename instead of
	// appending FGDC_SGML_EXTENSION. Confirmed here by actually reading
	// back the ".sgml" sibling file's contents through Present().
	char tmpl[] = "/tmp/isearch2_test_fgdcsite_sgml_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	remove(tmpl);  // Just wanted a unique base name.

	STRING SgmlPath = tmpl;
	SgmlPath.Cat(".sgml");
	NamedFile sgmlFile(SgmlPath, "<metadata>real sgml content</metadata>");

	TESTIDBOBJ db;
	FGDCSITE fgdcsite(&db);
	RESULT r;
	r.SetPathName("");
	STRING FakeName = tmpl;
	FakeName.Cat(".xml");  // Extension is irrelevant -- Present() strips and replaces it.
	r.SetFileName(FakeName);

	STRING out;
	fgdcsite.Present(r, "F", "SGML", &out);
	REQUIRE(out == "<metadata>real sgml content</metadata>");
}
