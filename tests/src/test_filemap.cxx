// Tests for src/filemap.hxx / src/filemap.cxx (class FILEMAP).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// FILEMAP's constructor unconditionally dereferences Parent->GetMainMdt(),
// so (unlike IRSET/RSET, whose constructors tolerate a nullptr parent)
// it needs a real IDBOBJ backed by a real MDT. IDBOBJ itself has only 3
// pure virtuals (none of which FILEMAP calls), so a minimal test-only
// subclass stands in for the real IDB class.

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
#include "filemap.hxx"

#include <cstdio>
#include <unistd.h>

class TESTIDBOBJ : public IDBOBJ {
public:
	explicit TESTIDBOBJ(MDT* TheMdt) : TheMdt(TheMdt) {}
	MDT* GetMainMdt() override { return TheMdt; }
	void DfdtAddEntry(const DFD&) override {}
	INT IsStopWord(CHR*, INT) const override { return 0; }
	GPTYPE ParseWords(const STRING&, CHR*, INT, INT, GPTYPE*, INT) override { return 0; }
private:
	MDT* TheMdt;
};

namespace {

// Owns the temp MDT files (<stem>.mdt/.mdg/.mdk) and cleans them up.
struct TempMdt {
	STRING Stem;
	MDT* Mdt;

	TempMdt() {
		char tmpl[] = "/tmp/isearch2_test_filemap_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		remove(tmpl);  // MDT creates its own <stem>.mdt/.mdg/.mdk instead
		Stem = tmpl;
		Mdt = new MDT(Stem, GDT_FALSE);
	}

	void AddDoc(GPTYPE GpStart, GPTYPE LocalStart, GPTYPE LocalEnd,
		    const CHR* PathName, const CHR* FileName) {
		MDTREC rec;
		rec.SetPathName(STRING(PathName));
		rec.SetFileName(STRING(FileName));
		rec.SetGlobalFileStart(GpStart);
		rec.SetLocalRecordStart(LocalStart);
		rec.SetLocalRecordEnd(LocalEnd);
		Mdt->AddEntry(rec);
	}

	~TempMdt() {
		delete Mdt;
		STRING Fn;
		Fn = Stem; Fn.Cat(".mdt"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdg"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdk"); remove(Fn);
	}
};

}  // namespace

TEST_CASE("FILEMAP GetKeyByGlobal finds the containing file's global start", "[filemap]") {
	TempMdt Tmp;
	// Two documents, back to back: doc1 spans local [0,99] at global
	// offset 0; doc2 spans local [0,49] at global offset 100.
	Tmp.AddDoc(0, 0, 99, "/tmp/", "doc1.txt");
	Tmp.AddDoc(100, 0, 49, "/tmp/", "doc2.txt");

	TESTIDBOBJ Idb(Tmp.Mdt);
	FILEMAP Map(&Idb);

	REQUIRE(Map.GetKeyByGlobal(0) == 0);
	REQUIRE(Map.GetKeyByGlobal(50) == 0);
	REQUIRE(Map.GetKeyByGlobal(99) == 0);
	REQUIRE(Map.GetKeyByGlobal(100) == 100);
	REQUIRE(Map.GetKeyByGlobal(149) == 100);
}

TEST_CASE("FILEMAP GetKeyByGlobal returns 0 for a gp outside every range", "[filemap]") {
	TempMdt Tmp;
	Tmp.AddDoc(0, 0, 99, "/tmp/", "doc1.txt");

	TESTIDBOBJ Idb(Tmp.Mdt);
	FILEMAP Map(&Idb);

	REQUIRE(Map.GetKeyByGlobal(1000) == 0);
}

TEST_CASE("FILEMAP GetNameByGlobal returns the file's path, size, and local start", "[filemap]") {
	TempMdt Tmp;
	Tmp.AddDoc(0, 0, 99, "/tmp/", "doc1.txt");
	Tmp.AddDoc(100, 0, 49, "/tmp/", "doc2.txt");

	TESTIDBOBJ Idb(Tmp.Mdt);
	FILEMAP Map(&Idb);

	STRING Path;
	INT Size, LocalStart;
	GPTYPE Start = Map.GetNameByGlobal(120, &Path, &Size, &LocalStart);

	REQUIRE(Start == 100);
	REQUIRE(Path == "/tmp/doc2.txt");
	REQUIRE(Size == 50);
	REQUIRE(LocalStart == 0);
}
