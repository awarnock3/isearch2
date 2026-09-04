// Tests for doctype/uspat.hxx / doctype/uspat.cxx (class USPAT, class GB).
// Linked against the real implementation via TEST_ENGINE_DOCTYPE_SRCS in
// the top-level Makefile, not reimplemented here.
//
// This file's audit was deliberately scoped to USPAT::ParseFields(),
// USPAT::Present(), and the GB column-tagged field parser (GB::GetFieldName()
// via the public GB::BuildDft() entry point) rather than the ~40 pato_*
// presentation-formatting helpers (~4500 lines) -- see the
// docs/BUG_CATALOG.md entry for this file for the full rationale.
//
// BUGFIX #2 (Present()'s PreferredRecordSyntax leak) and BUGFIX #4
// (pato_ReadPatent()'s memccpy() overread) are NOT covered by a test here:
// pato_ReadPatent() is a private method and the header-signature freeze
// rules out adding a test-only friend declaration to expose it, and
// Present()'s only public entry into that code path,
// RESULT::GetHighlightedRecord(), is a no-op in this build (DO_HIGHLIGHTING
// is never defined anywhere in this tree -- see src/result.cxx), so
// StringBuffer never carries real record content through Present() here.
// Both fixes were verified by inspection and are described in
// docs/BUG_CATALOG.md; see that file for details.

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
#include "uspat.hxx"

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

	explicit TempFile(const std::string& Contents) {
		char tmpl[] = "/tmp/isearch2_test_uspat_XXXXXX";
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

}  // namespace

TEST_CASE("USPAT::ParseFields treats an empty file as a zero-length record instead of underflowing", "[uspat]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeuspatcxx): this is the same
	// `ftell(fp) - 1` underflow pattern already fixed (and ASan-confirmed)
	// in doctype/colondoc.cxx BUGFIX #1, doctype/marcdump.cxx BUGFIX #2,
	// doctype/memodoc.cxx BUGFIX #1, and doctype/referbib.cxx BUGFIX #2.
	// RecEnd is GPTYPE (unsigned UINT4), so for an empty file
	// ftell(fp)==0 and the old `- 1` wrapped to UINT_MAX, which would
	// make `new CHR[RecLength + 1]` attempt a huge allocation.
	TempFile file("");
	TESTIDBOBJ db;
	USPAT dt(&db);

	RECORD record;
	record.SetPathName(file.Dir);
	record.SetFileName(file.Base);
	record.SetRecordStart(0);
	record.SetRecordEnd(0);  // 0 => ParseFields reads to EOF itself

	REQUIRE_NOTHROW(dt.ParseFields(&record));
}

TEST_CASE("GB::BuildDft does not read out of bounds for a zero-length record", "[uspat]") {
	// BUGFIX #3 (docs/BUG_CATALOG.md#doctypeuspatcxx), first half: without
	// the Start+4 > c_length guard at the top of GetFieldName(), the
	// unconditional `memcpy(tmp, c_record+Start, 4)` read 4 bytes from a
	// buffer with zero real content -- reachable any time a record's
	// entire text is empty (Start=0, c_length=0).
	CHR* recBuf = new CHR[1];
	recBuf[0] = '\0';
	GB gb(recBuf, 0);

	STRLIST fieldNames, hierarchies;
	PDFT pdft = gb.BuildDft(nullptr, fieldNames, hierarchies);
	REQUIRE(pdft != nullptr);
	delete pdft;
	delete [] recBuf;
}

TEST_CASE("GB::BuildDft does not read out of bounds when the last field's line ends exactly at EOF", "[uspat]") {
	// BUGFIX #3, second half: a record whose last field's line ends in a
	// newline right at end-of-buffer used to leave Done at 0 after the
	// blank-line-skipping loop ran off the end of c_record, so
	// GB::BuildDft() invoked GetFieldName() again with an out-of-bounds
	// Start -- straight into the same memcpy() as above. This is the
	// common case (any record ending in a newline), not an edge case:
	// confirmed as a real heap-buffer-overflow via a before/after
	// test-revert under ASan (see docs/BUG_CATALOG.md).
	//
	// Two 4-char-tagged, newline-terminated field lines; neither tag is
	// in FIELD_NAMES/HIERARCHIES (both lists passed empty below), so
	// GetFieldName()'s bounds handling is exercised without also
	// exercising InsertField()/Db.
	std::string content = "AAAA1111\nBBBB2222\n";
	CHR* recBuf = new CHR[content.size() + 1];
	memcpy(recBuf, content.data(), content.size());
	recBuf[content.size()] = '\0';
	GB gb(recBuf, (INT4)content.size());

	STRLIST fieldNames, hierarchies;
	PDFT pdft = gb.BuildDft(nullptr, fieldNames, hierarchies);
	REQUIRE(pdft != nullptr);
	delete pdft;
	delete [] recBuf;
}
