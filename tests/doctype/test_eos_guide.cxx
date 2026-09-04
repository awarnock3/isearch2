// Tests for doctype/eos_guide.hxx / doctype/eos_guide.cxx (class
// EOS_GUIDE). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// EOS_GUIDE's constructor forwards to DOCTYPE(DbParent) after reading
// a "SOURCE" doctype option (IDBOBJ::GetDocTypeOptions()'s default is
// a no-op, so DocSource just stays empty for these tests -- fine,
// since none of them exercise Present()).

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
#include "eos_guide.hxx"

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

	explicit TempFile(const char* Contents, size_t Len) {
		char tmpl[] = "/tmp/isearch2_test_eos_guide_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents, Len);
		close(fd);
		Dir = "/tmp/";
		Base = tmpl + strlen("/tmp/");
	}
	explicit TempFile(const char* Contents) : TempFile(Contents, strlen(Contents)) {}

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

// Finds the DF named `Name` (case-sensitive, matching how ParseFields()
// stores it) among dft's entries, or returns false if none matches.
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

TEST_CASE("EOS_GUIDE::ParseFields extracts the TITLE field", "[eos_guide]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypeeos_guidecxx): titlePosition
	// used to be reset to 0 at the top of every loop iteration,
	// including the one that processes </TITLE> itself -- since <TITLE>
	// and </TITLE> are read in different iterations, this meant title
	// extraction could never succeed for any input at all. Confirmed
	// here by asserting the field actually gets extracted (not just
	// "doesn't crash").
	const char* content = "<HTML><HEAD><TITLE>My Title</TITLE></HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	EOS_GUIDE eg(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	eg.ParseFields(&record);

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

TEST_CASE("EOS_GUIDE::ParseFields extracts META NAME/CONTENT fields", "[eos_guide]") {
	const char* content =
		"<HTML><HEAD><TITLE>T</TITLE>"
		"<META NAME=\"Author\" CONTENT=\"Jane Doe\">"
		"</HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	EOS_GUIDE eg(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	eg.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF authorDf;
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(FindField(dft, "AUTHOR", &authorDf));

	FCT fct;
	authorDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane Doe");
}

TEST_CASE("EOS_GUIDE::ParseFields does not stop early on a 0xFF byte in the title text", "[eos_guide]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeeos_guidecxx): the original
	// `char ch = (char)fgetc(fp);` narrowed fgetc()'s int result before
	// comparing it to EOF; on a platform where char is signed, the real
	// byte 0xFF narrows to the same value as EOF, so parsing stopped
	// dead the moment it saw that byte anywhere -- including in
	// ordinary (non-tag) text content, like the title here.
	std::string content = "<HTML><HEAD><TITLE>A";
	content += (char)0xFF;
	content += "B</TITLE></HEAD><BODY></BODY></HTML>";
	TempFile file(content.c_str(), content.size());
	TESTIDBOBJ db;
	EOS_GUIDE eg(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	eg.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));
}

TEST_CASE("EOS_GUIDE::ParseFields adds no TITLE field when there is none", "[eos_guide]") {
	const char* content = "<HTML><HEAD></HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	EOS_GUIDE eg(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	eg.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE_FALSE(FindField(dft, "TITLE", &titleDf));
}
