// Tests for doctype/firstline.hxx / doctype/firstline.cxx (class
// FIRSTLINE). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// FIRSTLINE's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too. ParseFields() reads the record's bytes back off disk, so
// these tests use a temp file the same way test_medline.cxx does.

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
#include "firstline.hxx"

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
		char tmpl[] = "/tmp/isearch2_test_firstline_XXXXXX";
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

TEST_CASE("FIRSTLINE::ParseFields extracts exactly the first line, excluding the newline", "[firstline]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypefirstlinecxx): the Headline
	// field's end used to be off by one (SetFieldEnd(val_len) instead
	// of SetFieldEnd(val_len - 1)), which -- since FC's end is
	// inclusive -- pulled the line's trailing '\n' into the indexed
	// field. Confirmed here via an exact-substring assertion (not just
	// "doesn't crash").
	const char* content = "Hello\nWorld";
	TempFile file(content);
	TESTIDBOBJ db;
	FIRSTLINE fl(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	fl.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF headlineDf;
	REQUIRE(FindField(dft, "HEADLINE", &headlineDf));

	FCT fct;
	headlineDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Hello");
}

TEST_CASE("FIRSTLINE::ParseFields extracts the whole file when there is no newline", "[firstline]") {
	// Same off-by-one bug, but here it would have pointed one byte past
	// EOF instead of just including a delimiter -- confirmed via an
	// exact-substring assertion against the whole (newline-less) file.
	const char* content = "No newline here";
	TempFile file(content);
	TESTIDBOBJ db;
	FIRSTLINE fl(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	fl.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF headlineDf;
	REQUIRE(FindField(dft, "HEADLINE", &headlineDf));

	FCT fct;
	headlineDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "No newline here");
}

TEST_CASE("FIRSTLINE::ParseFields adds no Headline field for an empty file", "[firstline]") {
	TempFile file("");
	TESTIDBOBJ db;
	FIRSTLINE fl(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	fl.ParseFields(&record);  // Must not crash.

	DFT dft;
	record.GetDft(&dft);
	DF headlineDf;
	REQUIRE_FALSE(FindField(dft, "HEADLINE", &headlineDf));
}
