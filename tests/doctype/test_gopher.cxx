// Tests for doctype/gopher.hxx / doctype/gopher.cxx (class GOPHER).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS
// in the top-level Makefile, not reimplemented here.
//
// GOPHER's constructor just forwards to DOCTYPE(DbParent), so the same
// minimal TESTIDBOBJ shape used by the other doctype/ tests works here
// too. Present() reads real files off disk (the record itself for "F",
// and a ".cap/<filename>" sidecar for "B"), so these tests build a
// small directory tree under /tmp.

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
#include "gopher.hxx"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace {

class TESTIDBOBJ : public IDBOBJ {
public:
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
};

// Sets up a unique /tmp subdirectory containing a record file and
// (optionally) a ".cap/<name>" sidecar next to it, mirroring the
// directory shape GOPHER::Present() expects.
struct GopherDir {
	STRING Dir;
	STRING RecordName;

	explicit GopherDir(const char* RecordContent) : RecordName("record.txt") {
		char tmpl[] = "/tmp/isearch2_test_gopher_XXXXXX";
		char* d = mkdtemp(tmpl);
		Dir = d;
		Dir.Cat("/");

		STRING recordPath = Dir;
		recordPath.Cat(RecordName);
		FILE* fp = fopen(recordPath, "wb");
		fwrite(RecordContent, 1, strlen(RecordContent), fp);
		fclose(fp);
	}

	void WriteCapFile(const char* CapContent) {
		STRING capDir = Dir;
		capDir.Cat(".cap");
		mkdir(capDir, 0755);
		STRING capPath = capDir;
		capPath.Cat("/");
		capPath.Cat(RecordName);
		FILE* fp = fopen(capPath, "wb");
		fwrite(CapContent, 1, strlen(CapContent), fp);
		fclose(fp);
	}

	void MakeResult(RESULT* r) const {
		r->SetPathName(Dir);
		r->SetFileName(RecordName);
		r->SetRecordStart(0);
	}

	~GopherDir() {
		STRING capPath = Dir;
		capPath.Cat(".cap/");
		capPath.Cat(RecordName);
		remove(capPath);
		STRING capDir = Dir;
		capDir.Cat(".cap");
		rmdir(capDir);
		STRING recordPath = Dir;
		recordPath.Cat(RecordName);
		remove(recordPath);
		rmdir(Dir);
	}
};

}  // namespace

TEST_CASE("GOPHER::Present element set B emits the .cap file's Name= value", "[gopher]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypegophercxx): the file handle
	// opened to read the .cap sidecar was never fclose()'d on this
	// (successfully-opened) path -- a real, always-reachable leak on
	// every "B" present of a record that has one. Not directly
	// observable via a REQUIRE (LeakSanitizer doesn't track file
	// descriptors), but exercised here regardless since the fix touches
	// this exact code path; verified leak-free by inspection and by
	// make tests-asan reporting no other regressions.
	GopherDir dir("irrelevant record body");
	dir.WriteCapFile("Name=A Human-Readable Title\n");
	TESTIDBOBJ db;
	GOPHER gopher(&db);

	RESULT r;
	dir.MakeResult(&r);
	STRING out;
	gopher.Present(r, "B", &out);
	REQUIRE(out == "A Human-Readable Title");
}

TEST_CASE("GOPHER::Present element set B falls back to the filename when there is no .cap file", "[gopher]") {
	GopherDir dir("irrelevant record body");
	TESTIDBOBJ db;
	GOPHER gopher(&db);

	RESULT r;
	dir.MakeResult(&r);
	STRING out;
	gopher.Present(r, "B", &out);
	REQUIRE(out == dir.RecordName);
}

TEST_CASE("GOPHER::Present element set F returns the raw record data", "[gopher]") {
	const char* content = "the actual record contents";
	GopherDir dir(content);
	TESTIDBOBJ db;
	GOPHER gopher(&db);

	RESULT r;
	dir.MakeResult(&r);
	r.SetRecordEnd((GPTYPE)strlen(content));
	STRING out;
	gopher.Present(r, "F", &out);
	REQUIRE(out == content);
}
