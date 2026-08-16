// Tests for doctype/taglist.hxx / doctype/taglist.cxx (class TAGLIST).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// TAGLIST's constructor forwards to SGMLTAG(DbParent), so the same
// minimal TESTIDBOBJ shape used by other doctype/ tests (e.g.
// tests/doctype/test_html.cxx) works here too.

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
#include "taglist.hxx"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
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

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_taglist_XXXXXX";
		int fd = mkstemp(tmpl);
		write(fd, Contents.data(), Contents.size());
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

std::string ExtractField(const std::string& FileContents, FC& fc) {
	return FileContents.substr(fc.GetFieldStart(),
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

TEST_CASE("TAGLIST::ParseFields actually attaches parsed fields to the record", "[taglist]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypetaglistcxx): NewRecord->
	// SetDft(*pdft) was never called -- every field parsed was simply
	// thrown away with the pdft that held it, so TAGLIST never actually
	// indexed anything, for any input, ever. This is the direct
	// regression test: before the fix, this would find zero fields.
	std::string content = "<TITLE>My Title</TITLE>";
	TempFile file(content);
	TESTIDBOBJ db;
	TAGLIST dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF titleDf;
	// DF::SetFieldName() (src/df.cxx) uppercases internally.
	REQUIRE(FindField(dft, "TITLE", &titleDf));

	FCT fct;
	titleDf.GetFct(&fct);
	FC fc;
	fct.GetEntry(1, &fc);
	REQUIRE(ExtractField(content, fc) == "My Title");
}

TEST_CASE("TAGLIST::ParseFields ignores a tag absent from the allowlist", "[taglist]") {
	std::string content = "<TITLE>Kept</TITLE><RANDOMTAG>Dropped</RANDOMTAG>";
	TempFile file(content);
	TESTIDBOBJ db;
	TAGLIST dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);

	dt.ParseFields(&record);

	DFT dft;
	record.GetDft(&dft);
	DF df;
	REQUIRE(FindField(dft, "TITLE", &df));
	REQUIRE_FALSE(FindField(dft, "RANDOMTAG", &df));
}

TEST_CASE("TAGLIST::ReplaceWithSpace does not corrupt memory when keeping an allowlisted tag", "[taglist]") {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypetaglistcxx): the branch for
	// a *kept* tag used to write through `tags_ptr[strlen(*tags_ptr)]`
	// -- an operator-precedence bug parsing as an index N slots ahead
	// in the tags_ptr array itself (N being the tag name's length),
	// then dereferencing whatever garbage or out-of-bounds pointer
	// happened to be there and writing to it. "TITLE" and "H1" are
	// both on the allowlist, so both take this branch here. Confirmed
	// via a before/after test-revert under ASan.
	std::string content = "<TITLE>Hello World</TITLE> plain text <H1>A Header</H1>";
	std::vector<char> buf(content.begin(), content.end());
	buf.push_back('\0');
	TESTIDBOBJ db;
	TAGLIST dt(&db);

	dt.ReplaceWithSpace(buf.data(), (INT)content.size());  // Must not crash.
	SUCCEED();
}

TEST_CASE("TAGLIST::ParseFields does not corrupt memory when called twice (m_TagPos reallocation)", "[taglist]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypetaglistcxx): m_TagPos is
	// always allocated via `new EntryType[numtags]` (array new), but
	// the cleanup right before reallocating it on a *second* call used
	// scalar `delete` instead of `delete []` -- a new[]/delete
	// mismatch, undefined behavior. Only reachable when ParseFields()
	// runs more than once on the same TAGLIST instance (m_TagPos is
	// null the first time), so this test calls it twice.
	std::string content1 = "<TITLE>First</TITLE>";
	std::string content2 = "<TITLE>Second Document</TITLE>";
	TempFile file1(content1);
	TempFile file2(content2);
	TESTIDBOBJ db;
	TAGLIST dt(&db);

	RECORD record1;
	record1.SetPathName(file1.Dir);
	record1.SetFileName(file1.Base);
	record1.SetRecordStart(0);
	record1.SetRecordEnd(0);
	dt.ParseFields(&record1);

	RECORD record2;
	record2.SetPathName(file2.Dir);
	record2.SetFileName(file2.Base);
	record2.SetRecordStart(0);
	record2.SetRecordEnd(0);
	dt.ParseFields(&record2);  // Must not crash (alloc-dealloc-mismatch).

	SUCCEED();
}

TEST_CASE("TAGLIST::ParseWords returns a sentinel instead of exiting the process when GpBuffer is full", "[taglist]") {
	// BUGFIX #4 (docs/BUG_CATALOG.md#doctypetaglistcxx): exit(1) used to
	// terminate the entire process (taking down the rest of this test
	// binary with it) the moment GpListSize reached GpLength. Same bug
	// already fixed in DOCTYPE::ParseWords() (doctype/doctype.cxx,
	// BUGFIX #1): return (GPTYPE)-1 instead, the sentinel
	// INDEX::BuildGpList() (src/index.cxx) already knows how to recover
	// from. "Hello World" has two words; GpLength=1 forces an overflow
	// on the second.
	std::string content = "<TITLE>Hello World</TITLE>";
	TempFile file(content);
	TESTIDBOBJ db;
	TAGLIST dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);
	dt.ParseFields(&record);  // Populates m_TagPos/m_NumPairs.

	GPTYPE gpBuffer[1];
	GPTYPE result = dt.ParseWords((CHR*)content.c_str(), (INT)content.size(),
				       0, gpBuffer, 1);
	REQUIRE(result == (GPTYPE)-1);
}
