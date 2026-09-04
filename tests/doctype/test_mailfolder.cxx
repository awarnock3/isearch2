// Tests for doctype/mailfolder.hxx / doctype/mailfolder.cxx (class
// MAILFOLDER). Linked against the real implementation via
// TEST_ENGINE_DOCTYPE_SRCS in the top-level Makefile, not reimplemented
// here.
//
// MAILFOLDER's constructor just forwards to DOCTYPE(DbParent), so the
// same minimal TESTIDBOBJ shape used by the other doctype/ tests works
// here too. ParseFields() reads the record's bytes back off disk, so
// those tests use a temp file the same way test_colondoc.cxx does.
// parse_tags() itself is private, so it's exercised only indirectly
// through ParseFields() -- including the BUGFIX #1 heap-buffer-overflow
// regression, which needs make tests-asan to actually catch a
// regression, not the assertions in the test itself.

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
#include "mailfolder.hxx"

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

// See tests/doctype/test_sgmlnorm.cxx's TempFile for why Dir/Base are
// kept separate rather than a single combined path.
struct TempFile {
	STRING Dir;
	STRING Base;

	explicit TempFile(const char* Contents) {
		char tmpl[] = "/tmp/isearch2_test_mailfolder_XXXXXX";
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

}  // namespace

TEST_CASE("MAILFOLDER::IsMailFromLine accepts well-formed From lines", "[mailfolder]") {
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);
	REQUIRE(mf.IsMailFromLine("From foo@bar Fri Sep 30 21:15:05 1994\n") == GDT_TRUE);
	REQUIRE(mf.IsMailFromLine("From foo@bar Fri Sep 30 21:15:05 MET DST 1994\n") == GDT_TRUE);
	REQUIRE(mf.IsMailFromLine("Not a from line\n") == GDT_FALSE);
	REQUIRE(mf.IsMailFromLine("Subject: hello\n") == GDT_FALSE);
}

TEST_CASE("MAILFOLDER::IsNewsLine accepts well-formed Article lines", "[mailfolder]") {
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);
	REQUIRE(mf.IsNewsLine("Article 123 of comp.lang.c++\n") == GDT_TRUE);
	REQUIRE(mf.IsNewsLine("Article of comp.lang.c++\n") == GDT_FALSE);  // no number
	REQUIRE(mf.IsNewsLine("From foo@bar\n") == GDT_FALSE);
}

TEST_CASE("MAILFOLDER::NameKey extracts the name part of \"Name <addr>\"", "[mailfolder]") {
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);
	char buf[256];
	strcpy(buf, "John Doe <john@example.com>");
	REQUIRE(STRING(mf.NameKey(buf, GDT_TRUE)) == "John Doe");

	strcpy(buf, "John Doe <john@example.com>");
	REQUIRE(STRING(mf.NameKey(buf, GDT_FALSE)) == "john@example.com");
}

TEST_CASE("MAILFOLDER::accept_tag only accepts the configured keyword list", "[mailfolder]") {
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);
	REQUIRE(mf.accept_tag("Subject") == GDT_TRUE);
	REQUIRE(mf.accept_tag("From") == GDT_TRUE);
	REQUIRE(mf.accept_tag("X-Mailer") == GDT_FALSE);  // not in the list
}

TEST_CASE("MAILFOLDER::ParseFields handles a header-only message with no body", "[mailfolder]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypemailfoldercxx): parse_tags()
	// used to read/write past the end of the buffer whenever the
	// From-line-skip advanced past the actual content without a body
	// following -- this is exactly that shape (a valid From line plus
	// one short header, nothing else). Confirmed a real
	// heap-buffer-overflow with a standalone repro before fixing;
	// running this under make tests-asan is what actually verifies the
	// fix, not the assertions below.
	const char* content = "From foo@bar Fri Sep 30 21:15:05 1994\nSubject: hi\n";
	TempFile file(content);
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	mf.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 1);
	DF df1;
	dft.GetEntry(1, &df1);
	STRING name1;
	df1.GetFieldName(&name1);
	REQUIRE(name1 == "SUBJECT");
}

TEST_CASE("MAILFOLDER::ParseFields extracts headers and the message body, including the last byte of the file", "[mailfolder]") {
	// BUGFIX #2 (RecEnd truncation), #2b (unconditional trailing-newline
	// exclusion), #3 (trailing-whitespace trim off-by-one), #4
	// (SetFieldEnd off-by-one), #5 (RecLength vs ActualLength fallback)
	// -- see docs/BUG_CATALOG.md#doctypemailfoldercxx, all mirroring
	// doctype/colondoc.cxx's bugs of the same shape. No trailing
	// newline on the body deliberately stresses BUGFIX #2/#2b together.
	const char* content =
	    "From foo@bar Fri Sep 30 21:15:05 1994\n"
	    "Subject: Hello World\n"
	    "From: Jane Doe\n"
	    "\n"
	    "This is the body";
	TempFile file(content);
	TESTIDBOBJ db;
	MAILFOLDER mf(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	mf.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	REQUIRE(dft.GetTotalEntries() == 3);

	DF df1, df2, df3;
	dft.GetEntry(1, &df1);
	dft.GetEntry(2, &df2);
	dft.GetEntry(3, &df3);
	STRING name1, name2, name3;
	df1.GetFieldName(&name1);
	df2.GetFieldName(&name2);
	df3.GetFieldName(&name3);
	REQUIRE(name1 == "SUBJECT");
	REQUIRE(name2 == "FROM");
	REQUIRE(name3 == "MESSAGE-BODY");

	FCT fct1, fct2, fct3;
	df1.GetFct(&fct1);
	df2.GetFct(&fct2);
	df3.GetFct(&fct3);
	FC fc1, fc2, fc3;
	fct1.GetEntry(1, &fc1);
	fct2.GetEntry(1, &fc2);
	fct3.GetEntry(1, &fc3);

	REQUIRE(ExtractField(content, fc1) == "Hello World");
	REQUIRE(ExtractField(content, fc2) == "Jane Doe");
	REQUIRE(ExtractField(content, fc3) == "This is the body");
}
