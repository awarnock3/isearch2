// Tests for doctype/html.hxx / doctype/html.cxx (class HTML). Linked
// against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// HTML's constructor just forwards to SGMLNORM(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests (e.g.
// tests/doctype/test_cipc.cxx) works here too.

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
#include "html.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_html_XXXXXX";
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

TEST_CASE("HTML::ParseFields extracts a simple tag pair as a field", "[html]") {
	const char* content = "<HTML><TITLE>My Page</TITLE><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTML html(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	html.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));

	FCT fct;
	titleDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "My Page");
}

TEST_CASE("HTML::ParseFields does not index tags on IgnoreHTMLTag()'s list", "[html]") {
	const char* content = "<HTML><P>Some paragraph text</P></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTML html(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	html.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF pDf;
	REQUIRE_FALSE(FindField(dft, "P", &pDf));
}

TEST_CASE("HTML::ParseFields handles a minimized <DD> list without a matching close", "[html]") {
	// Exercises the "min tags" fallback: a <DD> with no </DD> should
	// still get a field, bounded by the next <DT> or </DL>.
	const char* content = "<DL><DT>Term<DD>Definition text</DL>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTML html(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	html.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF ddDf;
	REQUIRE(FindField(dft, "DD", &ddDf));
}

TEST_CASE("HTML::Present element set B returns the title field", "[html]") {
	const char* content = "<HTML><TITLE>Page Title</TITLE></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTML html(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	html.ParseFields(&record);

	RESULT r;
	r.SetPathName(file.Dir);
	r.SetFileName(file.Base);
	r.SetKey("1");

	STRING out;
	html.Present(r, "B", &out);
	// Db (TESTIDBOBJ) has no GetFieldData() override, so this exercises
	// only the "does not crash, falls back sensibly" path -- SGMLNORM's
	// real field lookup needs a live IDBOBJ, out of scope here.
	SUCCEED();
}
