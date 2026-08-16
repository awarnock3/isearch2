// Tests for doctype/incoming/sgmlgils.hxx / .cxx (class SGMLGILS).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// SGMLGILS's constructor forwards to SGMLNORM(DbParent). Present()
// dispatches by RecordSyntax OID string, then GetSUTRSRecord()'s "B"
// element set looks up several field names via the inherited
// DOCTYPE::Present(), which delegates to Db->GetFieldData(RESULT,
// STRING, STRING*) -- the 3-arg overload -- so TESTIDBOBJ overrides
// exactly that, backed by a small field map (same pattern as
// tests/doctype/test_iafadoc.cxx).

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
#include "incoming/sgmlgils.hxx"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	std::map<std::string, std::string> Fields;

	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }

	GDT_BOOLEAN GetFieldData(const RESULT&, const STRING& FieldName,
				  STRING* StringBuffer) const override {
		auto it = Fields.find(std::string((const char*)FieldName));
		if (it != Fields.end()) {
			*StringBuffer = it->second.c_str();
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

}  // namespace

TEST_CASE("SGMLGILS::Present dispatches SUTRS OID to GetSUTRSRecord", "[sgmlgils]") {
	TESTIDBOBJ db;
	db.Fields["Title"] = "A GILS Record";
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.Present(r, "B", SutrsRecordSyntaxOID, &out);
	REQUIRE(out.Search("A GILS Record") != 0);
}

TEST_CASE("SGMLGILS::Present dispatches GRS1 OID to the GetGRS1Record placeholder", "[sgmlgils]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypeincomingsgmlgilscxx):
	// GetGRS1Record() didn't exist at all before this turn -- Present()
	// called it, but it was neither declared nor defined, so this file
	// could never compile, let alone be exercised.
	TESTIDBOBJ db;
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.Present(r, "B", GRS1RecordSyntaxOID, &out);
	REQUIRE(out.Search("GRS1 not implemented") != 0);
}

TEST_CASE("SGMLGILS::Present reports an unsupported record syntax", "[sgmlgils]") {
	TESTIDBOBJ db;
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.Present(r, "B", "some-other-oid", &out);
	REQUIRE(out.Search("Unsupported record syntax") != 0);
}

TEST_CASE("SGMLGILS::GetSUTRSRecord element set B composes Title-Originator", "[sgmlgils]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeincomingsgmlgilscxx):
	// SUTRS_OID (undeclared) was meant to be SutrsRecordSyntaxOID --
	// confirmed by the exact OID value ("1.2.840.10003.5.101") already
	// named in this file's own "Unsupported record syntax" message
	// matching src/defs.cxx's real SutrsRecordSyntaxOID constant.
	TESTIDBOBJ db;
	db.Fields["Title"] = "My Title";
	db.Fields["Originator"] = "Jane Doe";
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.GetSUTRSRecord(r, "B", &out);
	// Title + "-" + Control-Identifier ("") + "-" + Originator + "-" +
	// Local-Control-Number ("") -- empty fields still contribute their
	// separating dashes.
	REQUIRE(out == "My Title--Jane Doe-");
}

TEST_CASE("SGMLGILS::GetSUTRSRecord element set F is an unimplemented placeholder", "[sgmlgils]") {
	TESTIDBOBJ db;
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.GetSUTRSRecord(r, "F", &out);
	REQUIRE(out == "Full Record");
}

TEST_CASE("SGMLGILS::GetSUTRSRecord element set 'HTML HTML 0' reads a sibling .htm file", "[sgmlgils]") {
	char tmpl[] = "/tmp/isearch2_test_sgmlgils_XXXXXX";
	int fd = mkstemp(tmpl);
	close(fd);
	remove(tmpl);

	STRING sgmlPath = tmpl;
	sgmlPath.Cat(".sgm");
	NamedFile sgmlPlaceholder(sgmlPath, "");

	STRING htmPath = tmpl;
	htmPath.Cat(".htm");
	NamedFile htmFile(htmPath, "<html>rendered content</html>");

	TESTIDBOBJ db;
	SGMLGILS gils(&db);

	RESULT r;
	r.SetPathName("");
	r.SetFileName(sgmlPath);

	STRING out;
	gils.GetSUTRSRecord(r, "HTML HTML 0", &out);
	REQUIRE(out == "<html>rendered content</html>");

	remove(sgmlPath);
}

TEST_CASE("SGMLGILS::GetSUTRSRecord reports an unsupported element set", "[sgmlgils]") {
	TESTIDBOBJ db;
	SGMLGILS gils(&db);

	RESULT r;
	STRING out;
	gils.GetSUTRSRecord(r, "Z", &out);
	REQUIRE(out.Search("Unsupported element set name") != 0);
}
