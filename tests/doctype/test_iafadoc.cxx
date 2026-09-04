// Tests for doctype/iafadoc.hxx / doctype/iafadoc.cxx (class
// IAFADOC). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// IAFADOC's constructor just forwards to COLONDOC(DbParent). Present()
// looks up several candidate field names via the inherited
// DOCTYPE::Present(), which for anything but "F"/"B" delegates to
// Db->GetFieldData(ResultRecord, FieldName, STRING*) -- the 3-arg
// overload -- so TESTIDBOBJ overrides exactly that, backed by a small
// field map the individual tests populate.

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
#include "colondoc.hxx"
#include "iafadoc.hxx"

#include <map>
#include <string>

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

}  // namespace

TEST_CASE("IAFADOC::Present element set B combines Title and Author", "[iafadoc]") {
	TESTIDBOBJ db;
	db.Fields["Title"] = "My FTP Archive Entry";
	db.Fields["Author"] = "Jane Doe";
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);
	REQUIRE(out == "My FTP Archive Entry (Jane Doe) ");
}

TEST_CASE("IAFADOC::Present element set B uses just the Title when there is no Author", "[iafadoc]") {
	TESTIDBOBJ db;
	db.Fields["Title"] = "My FTP Archive Entry";
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);
	REQUIRE(out == "My FTP Archive Entry");
}

TEST_CASE("IAFADOC::Present element set B falls back to Package-Name when there is no Title", "[iafadoc]") {
	TESTIDBOBJ db;
	db.Fields["Package-Name"] = "gnu-tools";
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);
	REQUIRE(out == "Package: gnu-tools");
}

TEST_CASE("IAFADOC::Present element set B falls all the way to Newsgroup-Name", "[iafadoc]") {
	TESTIDBOBJ db;
	db.Fields["Newsgroup-Name"] = "comp.archives.test";
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);
	REQUIRE(out == "Newsgroup: comp.archives.test");
}

TEST_CASE("IAFADOC::Present element set B falls back to a truncated Description when no name field exists", "[iafadoc]") {
	TESTIDBOBJ db;
	db.Fields["Description"] = "First line of description\nSecond line, never shown";
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);
	REQUIRE(out == "Description: First line of description...");
}

TEST_CASE("IAFADOC::Present element set B does not crash when nothing at all is found", "[iafadoc]") {
	TESTIDBOBJ db;
	IAFADOC doc(&db);

	RESULT r;
	STRING out;
	doc.Present(r, "B", &out);  // Must not crash.
	SUCCEED();
}
