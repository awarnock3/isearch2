// Tests for doctype/iknowdoc.hxx / doctype/iknowdoc.cxx (class
// IKNOWDOC). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// IKNOWDOC's constructor forwards to COLONDOC(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too. ~IKNOWDOC() writes a "<db>.tpt" sidecar via
// Db->GetDbFileStem() (a plain virtual call on IDBOBJ itself after
// BUGFIX #4 removed an unnecessary and unsafe (IDB*) downcast), so
// every test that constructs an IKNOWDOC implicitly exercises that
// destructor path too -- TESTIDBOBJ's default (no-op) GetDbFileStem()
// makes it write to a predictable ".tpt" file in the current
// directory, cleaned up by TESTIDBOBJ's own destructor.
//
// COLONDOC::UnifiedName() (inherited, not overridden here) is the
// identity function, so any tag name used below becomes a real field
// name -- no allowlist to work around.

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
#include "iknowdoc.hxx"

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

	~TESTIDBOBJ() {
		// IDBOBJ::GetDbFileStem() defaults to a no-op, leaving
		// STRING temp empty in ~IKNOWDOC(), which then Cat()s
		// ".tpt" onto it -- so every IKNOWDOC built against a
		// plain TESTIDBOBJ writes to "./.tpt". Clean it up.
		remove(".tpt");
	}
};

struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_iknowdoc_XXXXXX";
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

TEST_CASE("IKNOWDOC::ParseFields extracts Template/Handle and a data field", "[iknowdoc]") {
	const char* content = "Template: Person\nHandle: p001\nName: Jane Doe\n";
	TempFile file(content);
	TESTIDBOBJ db;
	IKNOWDOC doc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	doc.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF nameDf;
	REQUIRE(FindField(dft, "NAME", &nameDf));

	FCT fct;
	nameDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane Doe");

	// Every tagged field is also duplicated into "Value-only".
	DF valueOnlyDf;
	REQUIRE(FindField(dft, "VALUE-ONLY", &valueOnlyDf));
}

TEST_CASE("IKNOWDOC::ParseFields tracks distinct Template values", "[iknowdoc]") {
	const char* content = "Template: Person\nHandle: p001\nName: Jane Doe\n";
	TempFile file(content);
	TESTIDBOBJ db;
	IKNOWDOC doc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	doc.ParseFields(&record);

	REQUIRE(doc.TemplateTypes.GetTotalEntries() == 1);
	STRING seen;
	doc.TemplateTypes.GetEntry(1, &seen);
	REQUIRE(seen == "Person");
}

TEST_CASE("IKNOWDOC::ParseFields does not include the last byte of a record ending without a newline", "[iknowdoc]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypeiknowdoccxx): RecEnd used to
	// be computed as `ftell(fp) - 1`, one byte short of the real file
	// size, so a final field with no trailing newline lost its last
	// character. Confirmed here via an exact-substring assertion (not
	// just "doesn't crash").
	const char* content = "Template: Person\nHandle: p001\nName: Jane";
	TempFile file(content);
	TESTIDBOBJ db;
	IKNOWDOC doc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	doc.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF nameDf;
	REQUIRE(FindField(dft, "NAME", &nameDf));

	FCT fct;
	nameDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "Jane");
}

TEST_CASE("IKNOWDOC::ParseFields skips (not aborts) a record whose first field is not Template", "[iknowdoc]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeiknowdoccxx): this used to
	// call EXIT_ERROR (exit(1)) on a malformed record, which would kill
	// this entire test binary -- not just fail this one assertion. The
	// fact that later TEST_CASEs in this file (and every other test
	// file linked into the same binary) still get to run at all is
	// itself part of what this regression test is confirming.
	const char* content = "NotATemplate: oops\nHandle: p001\n";
	TempFile file(content);
	TESTIDBOBJ db;
	IKNOWDOC doc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	doc.ParseFields(&record);  // Must not exit(1).

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 0);
}

TEST_CASE("IKNOWDOC::ParseFields skips (not aborts) a record whose second field is not Handle", "[iknowdoc]") {
	const char* content = "Template: Person\nNotHandle: p001\n";
	TempFile file(content);
	TESTIDBOBJ db;
	IKNOWDOC doc(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);

	doc.ParseFields(&record);  // Must not exit(1).

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 0);
}
