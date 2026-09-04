// Tests for src/record.hxx / src/record.cxx (class RECORD - Database Record).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "record.hxx"

#include <cstdio>

TEST_CASE("RECORD default-constructs with empty fields and zeroed range", "[record]") {
	RECORD r;
	STRING s;
	r.GetKey(&s);
	REQUIRE(s.GetLength() == 0);
	r.GetPathName(&s);
	REQUIRE(s.GetLength() == 0);
	r.GetFileName(&s);
	REQUIRE(s.GetLength() == 0);
	REQUIRE(r.GetRecordStart() == 0);
	REQUIRE(r.GetRecordEnd() == 0);
}

TEST_CASE("RECORD(PathName, FileName) constructor adds a trailing slash and strips FileName's path", "[record]") {
	// Absolute paths (starting with '/') so ExpandFileSpec() returns them
	// unmodified instead of resolving against the test runner's cwd.
	STRING PathName("/tmp/isearch2_test_dir");
	STRING FileName("/tmp/isearch2_test_dir/myfile.txt");
	RECORD r(PathName, FileName);

	STRING s;
	r.GetPathName(&s);
	REQUIRE(s == "/tmp/isearch2_test_dir/");
	r.GetFileName(&s);
	REQUIRE(s == "myfile.txt");

	// BUGFIX #2 coverage: RecordStart/RecordEnd used to be left
	// indeterminate by this constructor (unlike the default one).
	REQUIRE(r.GetRecordStart() == 0);
	REQUIRE(r.GetRecordEnd() == 0);
}

TEST_CASE("RECORD SetKey/GetKey round-trips", "[record]") {
	RECORD r;
	r.SetKey(STRING("some-key"));
	STRING s;
	r.GetKey(&s);
	REQUIRE(s == "some-key");
}

TEST_CASE("RECORD SetPathName adds a trailing slash", "[record]") {
	RECORD r;
	r.SetPathName(STRING("/tmp/isearch2_test_dir"));
	STRING s;
	r.GetPathName(&s);
	REQUIRE(s == "/tmp/isearch2_test_dir/");
}

TEST_CASE("RECORD SetFileName strips any directory prefix", "[record]") {
	RECORD r;
	r.SetFileName(STRING("/some/dir/doc.txt"));
	STRING s;
	r.GetFileName(&s);
	REQUIRE(s == "doc.txt");
}

TEST_CASE("RECORD GetFullFileName concatenates PathName and FileName", "[record]") {
	RECORD r;
	r.SetPathName(STRING("/tmp/isearch2_test_dir"));
	r.SetFileName(STRING("doc.txt"));
	STRING s;
	r.GetFullFileName(&s);
	REQUIRE(s == "/tmp/isearch2_test_dir/doc.txt");
}

TEST_CASE("RECORD SetRecordStart/SetRecordEnd round-trip", "[record]") {
	RECORD r;
	r.SetRecordStart(100u);
	r.SetRecordEnd(200u);
	REQUIRE(r.GetRecordStart() == 100u);
	REQUIRE(r.GetRecordEnd() == 200u);
}

TEST_CASE("RECORD SetDocumentType upper-cases", "[record]") {
	RECORD r;
	r.SetDocumentType(STRING("html"));
	STRING s;
	r.GetDocumentType(&s);
	REQUIRE(s == "HTML");
}

TEST_CASE("RECORD SetDft/GetDft round-trips the field table", "[record]") {
	DFT dft;
	DF df;
	df.SetFieldName(STRING("TITLE"));
	dft.AddEntry(df);

	RECORD r;
	r.SetDft(dft);

	DFT out;
	r.GetDft(&out);
	REQUIRE(out.GetTotalEntries() == 1);
}

TEST_CASE("RECORD operator= deep-copies independently of the source", "[record]") {
	RECORD original;
	original.SetKey(STRING("original-key"));
	original.SetRecordStart(5u);

	RECORD copy;
	copy = original;
	copy.SetKey(STRING("copy-key"));

	STRING s;
	original.GetKey(&s);
	REQUIRE(s == "original-key");
	copy.GetKey(&s);
	REQUIRE(s == "copy-key");
	REQUIRE(copy.GetRecordStart() == 5u);
}

TEST_CASE("RECORD Write/Read round-trips every field through a file", "[record]") {
	RECORD original;
	original.SetKey(STRING("test-key"));
	original.SetPathName(STRING("/tmp/isearch2_test_dir"));
	original.SetFileName(STRING("doc.txt"));
	original.SetRecordStart(10u);
	original.SetRecordEnd(20u);
	original.SetDocumentType(STRING("html"));

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	RECORD restored;
	restored.Read(fp);
	fclose(fp);

	STRING s;
	restored.GetKey(&s);
	REQUIRE(s == "test-key");
	restored.GetPathName(&s);
	REQUIRE(s == "/tmp/isearch2_test_dir/");
	restored.GetFileName(&s);
	REQUIRE(s == "doc.txt");
	REQUIRE(restored.GetRecordStart() == 10u);
	REQUIRE(restored.GetRecordEnd() == 20u);
	restored.GetDocumentType(&s);
	REQUIRE(s == "HTML");
}

TEST_CASE("RECORD Write/Read round-trips a GPTYPE above INT_MAX", "[record]") {
	// BUGFIX #1 coverage: Write() used %d (signed) on RecordStart/
	// RecordEnd (GPTYPE, unsigned); confirmed via a standalone repro
	// that the original %d/GetInt() pairing already round-tripped
	// correctly on this platform (complementary two's-complement
	// reinterpretation), the same "works today only by platform
	// coincidence" case already found in src/fc.cxx -- fixed to %u/
	// GetLong() anyway for portability, not because this was a
	// demonstrated failure.
	RECORD original;
	original.SetRecordStart(3000000000u);
	original.SetRecordEnd(4000000000u);

	FILE* fp = tmpfile();
	REQUIRE(fp != nullptr);
	original.Write(fp);
	rewind(fp);

	RECORD restored;
	restored.Read(fp);
	fclose(fp);

	REQUIRE(restored.GetRecordStart() == 3000000000u);
	REQUIRE(restored.GetRecordEnd() == 4000000000u);
}
