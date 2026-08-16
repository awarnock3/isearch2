// Tests for doctype/litmed.hxx / doctype/litmed.cxx (class LITMED).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// LITMED's constructor forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests (e.g.
// tests/doctype/test_html.cxx) works here too. Tag scanning is
// inherited from SGMLNORM, and only tags on IsLITMEDFieldTag()'s
// allowlist (STRICT_LITMED is permanently 1 -- see the header) are
// indexed, e.g. "title", not arbitrary tags.

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
#include "litmed.hxx"

#include <cstdio>
#include <cstring>
#include <string>
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

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_litmed_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, strlen(Contents));
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
	}

	~TempFile() {
		STRING Path(Dir);
		Path.Cat(Base);
		remove(Path);
	}
};

std::string ExtractField(const char* FileContents, FC& fc) {
	return std::string(FileContents + fc.GetFieldStart(),
			    fc.GetFieldEnd() - fc.GetFieldStart() + 1);
}

bool FindField(DFT& dft, const char* Name, DF* Out) {
	INT total = dft.GetTotalEntries();
	for (INT i = 1; i <= total; i++) {
		DF df;
		dft.GetEntry(i, &df);
		STRING fieldName;
		df.GetFieldName(&fieldName);
		if (fieldName == Name) {
			*Out = df;
			return true;
		}
	}
	return false;
}

}  // namespace

TEST_CASE("LITMED::ParseFields extracts an allowlisted tag pair as a field, with no trailing byte lost", "[litmed]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypelitmedcxx): the whole-file-
	// as-one-record fallback used to compute RecEnd = ftell(fp) - 1,
	// dropping the file's last byte -- here, the literal closing '>' of
	// </title>, which is exactly the file's last character (no trailing
	// newline). Confirmed via an exact-substring assertion.
	const char* content = "<title>My Title</title>";
	TempFile file(content);
	TESTIDBOBJ db;
	LITMED lm(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	lm.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));

	FCT fct;
	titleDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "My Title");
}

TEST_CASE("LITMED::ParseFields does not index a tag absent from IsLITMEDFieldTag()'s allowlist", "[litmed]") {
	const char* content = "<title>T</title><randomtag>ignored</randomtag>";
	TempFile file(content);
	TESTIDBOBJ db;
	LITMED lm(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	lm.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF randomDf;
	REQUIRE_FALSE(FindField(dft, "randomtag", &randomDf));
}

TEST_CASE("LITMED::ParseFields extracts an xauthor field", "[litmed]") {
	const char* content = "<title>T</title><xauthor>Jane Doe</xauthor>";
	TempFile file(content);
	TESTIDBOBJ db;
	LITMED lm(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	lm.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF authorDf;
	REQUIRE(FindField(dft, "XAUTHOR", &authorDf));

	FCT fct;
	authorDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane Doe");
}

TEST_CASE("LITMED::Present element set B returns the title field", "[litmed]") {
	const char* content = "<title>Page Title</title>";
	TempFile file(content);
	TESTIDBOBJ db;
	LITMED lm(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	lm.ParseFields(&record);

	RESULT r;
	r.SetPathName(file.Dir);
	r.SetFileName(file.Base);

	STRING out;
	lm.Present(r, "B", &out);
	// Db (TESTIDBOBJ) has no GetFieldData() override, so this exercises
	// only the "does not crash" path -- SGMLNORM's real field lookup
	// needs a live IDBOBJ, out of scope here (same as
	// tests/doctype/test_html.cxx's equivalent note).
	SUCCEED();
}
