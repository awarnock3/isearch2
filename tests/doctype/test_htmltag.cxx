// Tests for doctype/htmltag.hxx / doctype/htmltag.cxx (class
// HTMLTAG). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// HTMLTAG's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too.

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
#include "htmltag.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_htmltag_XXXXXX";
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

TEST_CASE("HTMLTAG::ParseFields extracts the TITLE field", "[htmltag]") {
	const char* content = "<HTML><HEAD><TITLE>My Title</TITLE></HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);

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

TEST_CASE("HTMLTAG::ParseFields extracts META NAME/CONTENT fields", "[htmltag]") {
	const char* content =
		"<HTML><HEAD><TITLE>T</TITLE>"
		"<META NAME=\"Author\" CONTENT=\"Jane Doe\">"
		"</HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);

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

TEST_CASE("HTMLTAG::ParseFields does not stop early on a 0xFF byte in the title text", "[htmltag]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypehtmltagcxx): the original
	// `char ch = (char)fgetc(fp);` narrowed fgetc()'s int result before
	// comparing it to EOF; on a platform where char is signed, the real
	// byte 0xFF narrows to the same value as EOF, so parsing stopped
	// dead the moment it saw that byte anywhere -- including in
	// ordinary (non-tag) text content, like the title here. Same bug
	// as, and fixed the same way as, doctype/eos_guide.cxx's BUGFIX #1.
	std::string content = "<HTML><HEAD><TITLE>A";
	content += (char)0xFF;
	content += "B</TITLE></HEAD><BODY></BODY></HTML>";
	TempFile file(content.c_str(), content.size());
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE(FindField(dft, "TITLE", &titleDf));
}

TEST_CASE("HTMLTAG::ParseFields ignores a META NAME value with a high-bit byte without UB", "[htmltag]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypehtmltagcxx): isalnum()/
	// toupper() are undefined behavior for an argument not
	// representable as unsigned char (or EOF); a NAME value containing
	// a byte >= 0x80 used to pass a negative char straight through on a
	// signed-char platform. Not independently assertable via REQUIRE
	// (UB doesn't necessarily crash), but exercised here so `make
	// tests-asan` (UndefinedBehaviorSanitizer) can catch a regression.
	std::string content = "<HTML><HEAD><TITLE>T</TITLE>"
		"<META NAME=\"Author";
	content += (char)0x80;
	content += "\" CONTENT=\"Jane Doe\">"
		"</HEAD><BODY></BODY></HTML>";
	TempFile file(content.c_str(), content.size());
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);  // Must not trigger UBSan.
	SUCCEED();
}

TEST_CASE("HTMLTAG::ParseFields ignores a stray </TITLE> with no matching <TITLE>", "[htmltag]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypehtmltagcxx): titlePosition
	// used to be read here even when no <TITLE> had actually been seen,
	// reading an uninitialized local. Guarded with sawTitleOpen instead.
	const char* content = "<HTML><HEAD></TITLE>orphan</HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE_FALSE(FindField(dft, "TITLE", &titleDf));
}

TEST_CASE("HTMLTAG::ParseFields adds no TITLE field when there is none", "[htmltag]") {
	const char* content = "<HTML><HEAD></HEAD><BODY></BODY></HTML>";
	TempFile file(content);
	TESTIDBOBJ db;
	HTMLTAG tag(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	tag.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	REQUIRE_FALSE(FindField(dft, "TITLE", &titleDf));
}
