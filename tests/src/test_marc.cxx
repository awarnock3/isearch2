// Tests for src/marc.hxx / src/marc.cxx (class MARC).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// MARC's constructor takes a STRING holding a raw, wire-format MARC
// record (leader + directory + field data, delimited by SUBFDELIM/
// FIELDTERM/RECTERM -- see src/marcdefs.hxx), so building test input
// means building that byte layout by hand. BuildValidRecord() below is
// adapted from the same-shaped helper in tests/src/test_marclib.cxx
// (that one builds a raw char* for GetMARC() directly; this one wraps
// it in a STRING for MARC's constructor).

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "marcdefs.hxx"
#include "memcntl.hxx"
#include "marclib.hxx"
#include "marc.hxx"

#include <cstring>
#include <cstdio>
#include <string>

namespace {

// Builds a minimal, well-formed MARC record: a leader, one directory
// entry for tag "245" (Title), and one field with subfields 'a' and
// 'b'. If longBValue is non-null, subfield 'b' holds that (otherwise
// "World"). Same byte layout as test_marclib.cxx's BuildValidRecord(),
// wrapped in a STRING (via the explicit-length constructor, since the
// record contains embedded MARC delimiter bytes, not just text) for
// MARC's own constructor.
STRING BuildValidRecord(const std::string& bValue = "World") {
	std::string fielddata = std::string("ab") + SUBFDELIM + "a" + "Hello" +
				 SUBFDELIM + "b" + bValue + FIELDTERM;
	int fdlen = (int)fielddata.size();
	int baseaddr = 24 + 12 + 1;
	int reclen = baseaddr + fdlen + 1;

	char* record = new char[reclen];
	memset(record, ' ', reclen);

	MARC_LEADER_OVER* leader = (MARC_LEADER_OVER*)record;
	char basebuf[6];
	snprintf(basebuf, sizeof(basebuf), "%05d", baseaddr);
	memcpy(leader->BaseAddr, basebuf, 5);

	MARC_DIRENTRY_OVER* dir = (MARC_DIRENTRY_OVER*)(record + sizeof(MARC_LEADER_OVER));
	memcpy(dir->tag, "245", 3);
	char lenbuf[5], startbuf[6];
	snprintf(lenbuf, sizeof(lenbuf), "%04d", fdlen);
	snprintf(startbuf, sizeof(startbuf), "%05d", 0);
	memcpy(dir->flen, lenbuf, 4);
	memcpy(dir->fstart, startbuf, 5);
	record[24 + 12] = RECTERM;

	memcpy(record + baseaddr, fielddata.data(), fdlen);
	record[reclen - 1] = RECTERM;

	STRING result(record, reclen);
	delete [] record;
	return result;
}

}  // namespace

TEST_CASE("MARC parses a well-formed record and prints its title field", "[marc]") {
	STRING data = BuildValidRecord();
	MARC m(data);
	STRING out;
	m.Print(&out);
	// defaultformat[] maps tag "245" to the "Title:" label.
	CHR* cstr = out.NewCString();
	std::string s(cstr);
	delete [] cstr;
	REQUIRE(s.find("Title:") != std::string::npos);
	REQUIRE(s.find("Hello") != std::string::npos);
	REQUIRE(s.find("World") != std::string::npos);
}

TEST_CASE("MARC does not leak its parsed record", "[marc]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcmarchxx): ~MARC() used to
	// never free c_rec (the comment literally said "FREE THE c_rec!!").
	// Constructing and destroying several MARC objects in a row is what
	// make tests-asan's LeakSanitizer actually verifies against a
	// regression here, not the assertions in this test.
	for (int i = 0; i < 5; i++) {
		STRING data = BuildValidRecord();
		MARC m(data);
		STRING out;
		m.Print(&out);
	}
	SUCCEED();
}

TEST_CASE("MARC handles a malformed record without leaving format state uninitialized", "[marc]") {
	// BUGFIX #2: c_format/c_maxlen used to be set after GetMARC()'s
	// failure check's early return, leaving them uninitialized on a
	// parse failure -- and there's no public way to ask "did
	// construction succeed?", so a caller calling Print() anyway (the
	// only thing it can do) used to read garbage as the wrap width.
	STRING data("not a valid marc record at all");
	MARC m(data);
	STRING out;
	m.Print(&out);  // Must not crash or read uninitialized c_maxlen.
	SUCCEED();
}

TEST_CASE("MARC::Print wraps a long space-less field instead of crashing", "[marc]") {
	// BUGFIX #3 (see docs/BUG_CATALOG.md#srcmarchxx): the word-wrap
	// helpers scanned backward for a space with no lower bound --
	// confirmed a real stack-buffer-underflow with a standalone ASan
	// repro (a single run of 'A's with no spaces, longer than the
	// default 79-column wrap width) before fixing. make tests-asan is
	// what actually re-verifies this, same as the repro did.
	std::string longWord(200, 'A');
	STRING data = BuildValidRecord(longWord);
	MARC m(data);
	STRING out;
	m.Print(&out);  // Must not crash.
	CHR* cstr = out.NewCString();
	std::string s(cstr);
	delete [] cstr;
	REQUIRE(s.find("Hello") != std::string::npos);
}
