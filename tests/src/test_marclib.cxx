// Tests for src/marclib.hxx / src/marclib.cxx (free-function MARC
// record/field parsing library). Linked against the real implementation
// via TEST_ENGINE_SRCS in the top-level Makefile, not reimplemented
// here.
//
// marclib.cxx's AllocSafe() calls go through a file-scope
// `extern struct MemBlock *RememberKey;` that's only ever defined in
// src/marc.cxx. src/marc.cxx is now linked into this test binary too
// (added when doctype/mailfolder.cxx's turn needed RememberKey for an
// unrelated reason), so the stub definition that used to live here
// (the same thing every standalone repro used while confirming these
// bugs did, back when marc.cxx wasn't linked) was removed -- keeping
// both would be a duplicate-definition link error.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "marcdefs.hxx"
#include "memcntl.hxx"
#include "marclib.hxx"

#include <cstring>
#include <cstdlib>
#include <cstdio>

TEST_CASE("GetNum converts a fixed-width digit run to a number", "[marclib]") {
	REQUIRE(GetNum(const_cast<char*>("00042"), 5) == 42);
	REQUIRE(GetNum(const_cast<char*>("0000"), 4) == 0);
}

TEST_CASE("tagcmp matches literal tags and X/x wildcards", "[marclib]") {
	REQUIRE(tagcmp("245", "245") == 0);
	REQUIRE(tagcmp("245", "246") == -1);
	REQUIRE(tagcmp("2xx", "245") == 0);
	REQUIRE(tagcmp("2Xx", "299") == 0);
}

TEST_CASE("fieldcopy stops at a field terminator", "[marclib]") {
	char out[32];
	char in[] = "Hello\036garbage";
	int n = fieldcopy(out, in);
	REQUIRE(std::string(out) == "Hello");
	REQUIRE(n == 6);
}

TEST_CASE("codeconvert/charconvert replace delimiter and control chars", "[marclib]") {
	char s[] = "a\037b\036c\035d";
	codeconvert(s);
	REQUIRE(std::string(s) == "a$b+c|d");

	REQUIRE(charconvert(SUBFDELIM) == '$');
	REQUIRE(charconvert(FIELDTERM) == '+');
	REQUIRE(charconvert(RECTERM) == '|');
	REQUIRE(charconvert('Q') == 'Q');
}

TEST_CASE("SetSubF caps subfcodes at 20 entries instead of overflowing", "[marclib]") {
	// BUGFIX #1 coverage: before the fix, a field with more than 20
	// subfields wrote past subfcodes[21] (index 0 is the count, valid
	// data slots are 1..20). Confirmed originally via a standalone ASan
	// repro (UBSan: "index 21 out of bounds for type 'char [21]'").
	MARC_FIELD f{};
	std::string data;
	for (int i = 0; i < 25; i++) {
		data += SUBFDELIM;
		data += char('a' + i);
		data += "x";
	}
	data += FIELDTERM;

	int rc = SetSubF(&f, data.data());
	REQUIRE(rc == 1);
	REQUIRE((unsigned char)f.subfcodes[0] == 20);

	// All 25 subfields are still linked into the list (data preserved),
	// even though only the first 20 are indexable by code.
	int count = 0;
	for (MARC_SUBFIELD* s = f.subfield; s != nullptr; s = s->next)
		count++;
	REQUIRE(count == 25);
}

namespace {

// Builds a minimal, well-formed MARC record: a leader, one directory
// entry for tag "245", and one field with two subfields ('a' and 'b').
// Caller owns the returned buffer (malloc'd) and must free() it.
char* BuildValidRecord(int& reclen) {
	const char* fielddata = "ab\037aHello\037bWorld\036";
	int fdlen = (int)strlen(fielddata);
	int baseaddr = 24 + 12 + 1;
	reclen = baseaddr + fdlen + 1;

	char* record = (char*)malloc(reclen + 1);
	memset(record, ' ', reclen);
	record[reclen] = '\0';

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

	memcpy(record + baseaddr, fielddata, fdlen);
	record[reclen - 1] = RECTERM;
	return record;
}

} // namespace

TEST_CASE("GetMARC parses a well-formed record and its subfields", "[marclib]") {
	int reclen = 0;
	char* record = BuildValidRecord(reclen);

	MARC_REC* m = GetMARC(record, reclen, 0);
	REQUIRE(m != nullptr);
	REQUIRE(m->nfields == 1);

	MARC_FIELD* f = m->fields;
	REQUIRE(std::string(f->tag) == "245");
	REQUIRE(f->indicator1 == 'a');
	REQUIRE(f->indicator2 == 'b');

	char buf[64];
	REQUIRE(GetSubf(f, buf, 'a') != nullptr);
	REQUIRE(std::string(buf) == "Hello");
	REQUIRE(GetSubf(f, buf, 'b') != nullptr);
	REQUIRE(std::string(buf) == "World");
	REQUIRE(GetSubf(f, buf, 'z') == nullptr);

	free(record);
}

TEST_CASE("GetMARC rejects a field whose directory offset points outside the record", "[marclib]") {
	// BUGFIX #2 coverage: before the fix, a directory entry claiming a
	// field starts far beyond the record buffer made SetField read out
	// of bounds. Confirmed originally via a standalone ASan repro
	// (heap-buffer-overflow READ at the indicator1 line).
	const int reclen = 24 + 12 + 1;
	char* record = (char*)malloc(reclen);
	memset(record, '0', reclen);
	record[reclen - 1] = FIELDTERM;

	MARC_LEADER_OVER* leader = (MARC_LEADER_OVER*)record;
	memcpy(leader->BaseAddr, "00036", 5);

	MARC_DIRENTRY_OVER* dir = (MARC_DIRENTRY_OVER*)(record + sizeof(MARC_LEADER_OVER));
	memcpy(dir->tag, "245", 3);
	memcpy(dir->flen, "0010", 4);
	memcpy(dir->fstart, "09000", 5); // wildly out of range for a 37-byte record

	MARC_REC* m = GetMARC(record, reclen, 0);
	REQUIRE(m == nullptr);
	free(record);
}

TEST_CASE("GetMARC rejects a record too short to hold a leader", "[marclib]") {
	// BUGFIX #4 coverage: before the fix, a record shorter than
	// MARC_LEADER_OVER (24 bytes) still had its leader's BaseAddr field
	// (offset 12-16) read unconditionally, walking past the buffer.
	// Confirmed originally via a standalone ASan repro (heap-buffer-
	// overflow READ inside GetNum(), called from GetMARC()).
	char record[10];
	memset(record, '0', sizeof(record));

	MARC_REC* m = GetMARC(record, sizeof(record), 0);
	REQUIRE(m == nullptr);
}

TEST_CASE("GetMARC rejects a leader-only record with no directory terminator", "[marclib]") {
	// BUGFIX #4 coverage: a record just long enough to pass the leader
	// check above, but truncated before any directory terminator,
	// walked the directory-scan loop's `dir` pointer past the buffer
	// reading dir->tag[0]. Confirmed originally via a standalone ASan
	// repro (heap-buffer-overflow READ at the directory scan).
	char record[24];
	memset(record, '0', sizeof(record));

	MARC_REC* m = GetMARC(record, sizeof(record), 0);
	REQUIRE(m != nullptr);
	REQUIRE(m->nfields == 0);
}

TEST_CASE("normalize formats a call number and rejects malformed input", "[marclib]") {
	char in[] = "QA76.73 .C153";
	char out[64];
	char* result = normalize(in, out);
	REQUIRE(result == out);
	REQUIRE(std::string(out).substr(0, 3) == "QA ");

	char badin[] = "123 no leading letters";
	char badout[64];
	REQUIRE(normalize(badin, badout) == nullptr);
}
