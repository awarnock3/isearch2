// Tests for src/mdtrec.hxx / src/mdtrec.cxx (class MDTREC - Multiple
// Document Table Record).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "mdtrec.hxx"

#include <cstring>

TEST_CASE("MDTREC default-constructs with empty strings and zeroed offsets", "[mdtrec]") {
	MDTREC rec;
	STRING s;
	rec.GetKey(&s);
	REQUIRE(s == "");
	REQUIRE(rec.GetGlobalFileStart() == 0u);
	REQUIRE(rec.GetGlobalFileEnd() == 0u);
	REQUIRE(rec.GetLocalRecordStart() == 0u);
	REQUIRE(rec.GetLocalRecordEnd() == 0u);
	REQUIRE(rec.GetDeleted() == GDT_FALSE);
}

TEST_CASE("MDTREC SetKey/GetKey round-trip", "[mdtrec]") {
	MDTREC rec;
	rec.SetKey(STRING("doc123"));
	STRING s;
	rec.GetKey(&s);
	REQUIRE(s == "doc123");
}

TEST_CASE("MDTREC SetDocumentType/GetDocumentType round-trip", "[mdtrec]") {
	MDTREC rec;
	rec.SetDocumentType(STRING("TEXT"));
	STRING s;
	rec.GetDocumentType(&s);
	REQUIRE(s == "TEXT");
}

TEST_CASE("MDTREC GetFullFileName concatenates PathName and FileName", "[mdtrec]") {
	MDTREC rec;
	rec.SetPathName(STRING("/data/docs/"));
	rec.SetFileName(STRING("report.txt"));
	STRING s;
	rec.GetFullFileName(&s);
	REQUIRE(s == "/data/docs/report.txt");
}

TEST_CASE("MDTREC offset Set/Get round-trip", "[mdtrec]") {
	MDTREC rec;
	rec.SetGlobalFileStart(100u);
	rec.SetGlobalFileEnd(200u);
	rec.SetLocalRecordStart(10u);
	rec.SetLocalRecordEnd(20u);
	REQUIRE(rec.GetGlobalFileStart() == 100u);
	REQUIRE(rec.GetGlobalFileEnd() == 200u);
	REQUIRE(rec.GetLocalRecordStart() == 10u);
	REQUIRE(rec.GetLocalRecordEnd() == 20u);
}

TEST_CASE("MDTREC SetDeleted/GetDeleted round-trip", "[mdtrec]") {
	MDTREC rec;
	rec.SetDeleted(GDT_TRUE);
	REQUIRE(rec.GetDeleted() == GDT_TRUE);
	rec.SetDeleted(GDT_FALSE);
	REQUIRE(rec.GetDeleted() == GDT_FALSE);
}

TEST_CASE("MDTREC FlipBytes round-trips the four offset fields", "[mdtrec]") {
	MDTREC rec;
	rec.SetGlobalFileStart(1u);
	rec.SetGlobalFileEnd(2u);
	rec.SetLocalRecordStart(3u);
	rec.SetLocalRecordEnd(4u);
	rec.FlipBytes();
	rec.FlipBytes();
	REQUIRE(rec.GetGlobalFileStart() == 1u);
	REQUIRE(rec.GetGlobalFileEnd() == 2u);
	REQUIRE(rec.GetLocalRecordStart() == 3u);
	REQUIRE(rec.GetLocalRecordEnd() == 4u);
}

TEST_CASE("MDTREC operator= copies fields independently of the source", "[mdtrec]") {
	MDTREC a;
	a.SetKey(STRING("a-key"));
	a.SetGlobalFileStart(42u);

	MDTREC b;
	b = a;

	a.SetKey(STRING("mutated"));
	a.SetGlobalFileStart(999u);

	STRING s;
	b.GetKey(&s);
	REQUIRE(s == "a-key");
	REQUIRE(b.GetGlobalFileStart() == 42u);
}

TEST_CASE("MDTREC operator= self-assignment leaves fields intact", "[mdtrec]") {
	MDTREC rec;
	rec.SetKey(STRING("keep"));
	rec.SetGlobalFileStart(7u);
	rec = rec;
	STRING s;
	rec.GetKey(&s);
	REQUIRE(s == "keep");
	REQUIRE(rec.GetGlobalFileStart() == 7u);
}

TEST_CASE("MDTREC getters are bounded even when buffers aren't null-terminated", "[mdtrec]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcmdtreccxx): simulates
	// MDT::GetEntry's raw fread() directly into an MDTREC's memory (see
	// src/mdt.cxx) -- a corrupt or truncated on-disk record could leave
	// every fixed buffer filled with non-null bytes and no terminator
	// anywhere. Before the fix, GetKey()'s implicit strlen()-based
	// conversion would read past Key's DocumentKeySize-byte buffer into
	// whatever memory follows it.
	MDTREC rec;
	// Cast to void* first: this memset is deliberately simulating a raw
	// fread() into the object (as MDT::GetEntry does), not accidental
	// misuse, so silence -Wclass-memaccess rather than work around it.
	memset(static_cast<void*>(&rec), 'A', sizeof(MDTREC));

	STRING key, docType, pathName, fileName, fullFileName;
	rec.GetKey(&key);
	rec.GetDocumentType(&docType);
	rec.GetPathName(&pathName);
	rec.GetFileName(&fileName);
	rec.GetFullFileName(&fullFileName);

	REQUIRE(key.GetLength() <= (STRINGINDEX)DocumentKeySize);
	REQUIRE(docType.GetLength() <= (STRINGINDEX)DocumentTypeSize);
	REQUIRE(pathName.GetLength() <= (STRINGINDEX)DocPathNameSize);
	REQUIRE(fileName.GetLength() <= (STRINGINDEX)DocFileNameSize);
	REQUIRE(fullFileName.GetLength() <= (STRINGINDEX)(DocPathNameSize + DocFileNameSize));
}

TEST_CASE("MDTREC operator= is bounded even when the source isn't null-terminated", "[mdtrec]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcmdtreccxx): the old strcpy
	// calls would read past a non-null-terminated source buffer, not
	// just the getters above.
	MDTREC a;
	memset(static_cast<void*>(&a), 'A', sizeof(MDTREC));
	MDTREC b;
	b = a;

	STRING key;
	b.GetKey(&key);
	REQUIRE(key.GetLength() <= (STRINGINDEX)DocumentKeySize);
}
