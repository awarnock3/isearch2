// Tests for src/squery.hxx / src/squery.cxx (class SQUERY).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// OpenThesaurus() constructs a real THESAURUS, but THESAURUS's
// search-time constructor tolerates a nonexistent path/file cleanly
// (its OpenParentsFile()/OpenChildrenFile() just fopen() and return
// early on failure -- see src/thesaurus.cxx), so these tests point it
// at a scratch path that doesn't exist rather than building real
// .syn/.spx/.scx files on disk.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "strlist.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "result.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "termobj.hxx"
#include "sterm.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "operator.hxx"
#include "tokengen.hxx"
#include "thesaurus.hxx"
#include "squery.hxx"

TEST_CASE("SQUERY's copy constructor copies term/KWAQS state but starts with no thesaurus", "[squery]") {
	SQUERY Source;
	Source.SetTerm("spatial");
	STRING Kwaqs = "some kwaqs term";
	Source.SetKWAQSTerm(Kwaqs);
	Source.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db");

	// BUGFIX #1 (docs/BUG_CATALOG.md#srcsqueryhxx): the compiler-
	// generated copy constructor (there was no user-declared one at
	// all) shallow-copied Thesaurus, so both Source and Copy would share
	// one THESAURUS* -- confirmed heap-use-after-free once both called
	// CloseThesaurus(). The real fix starts Copy with no thesaurus of
	// its own, so both CloseThesaurus() calls below must be safe
	// independently.
	SQUERY Copy(Source);

	STRING Term;
	Copy.GetTerm(&Term);
	REQUIRE(Term == "spatial");

	STRING CopiedKwaqs;
	Copy.GetKWAQSTerm(&CopiedKwaqs);
	REQUIRE(CopiedKwaqs == "some kwaqs term");

	Copy.CloseThesaurus();    // must be a safe no-op (Copy's Thesaurus is nullptr)
	Source.CloseThesaurus();  // must still correctly free Source's own real one
	SUCCEED();
}

TEST_CASE("SQUERY's operator= copies term/KWAQS state and does not share Thesaurus", "[squery]") {
	SQUERY Source;
	Source.SetTerm("terrestrial");
	STRING Kwaqs = "another kwaqs term";
	Source.SetKWAQSTerm(Kwaqs);
	Source.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db");

	SQUERY Target;
	Target.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db2");
	// BUGFIX #1, continued: operator= used to do `Thesaurus =
	// OtherSquery.Thesaurus;`, aliasing the source's pointer and leaking
	// Target's own previous Thesaurus in the same stroke. Also used to
	// never copy c_kwaqs_term at all (a separate incomplete-copy bug
	// found while fixing this).
	Target = Source;

	STRING Term;
	Target.GetTerm(&Term);
	REQUIRE(Term == "terrestrial");

	STRING CopiedKwaqs;
	Target.GetKWAQSTerm(&CopiedKwaqs);
	REQUIRE(CopiedKwaqs == "another kwaqs term");

	Target.CloseThesaurus();  // must be a safe no-op (Target's Thesaurus is nullptr post-assignment)
	Source.CloseThesaurus();  // must still correctly free Source's own real one
	SUCCEED();
}

TEST_CASE("SQUERY::OpenThesaurus does not leak when called twice", "[squery]") {
	// BUGFIX #4: OpenThesaurus() used to overwrite Thesaurus
	// unconditionally, leaking whatever it already pointed to.
	SQUERY Q;
	Q.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db1");
	Q.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db2");
	Q.CloseThesaurus();
	SUCCEED();
}

TEST_CASE("SQUERY::CloseThesaurus is safe to call twice", "[squery]") {
	// BUGFIX #3: CloseThesaurus() used to leave Thesaurus dangling
	// (freed but not reset to nullptr) instead of making a second call
	// a safe no-op.
	SQUERY Q;
	Q.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db");
	Q.CloseThesaurus();
	Q.CloseThesaurus();
	SUCCEED();
}

TEST_CASE("SQUERY's destructor frees an open Thesaurus without a matching CloseThesaurus", "[squery]") {
	// BUGFIX #2: ~SQUERY() used to be empty, leaking Thesaurus (and its
	// open file handles) for any SQUERY destroyed without an explicit
	// CloseThesaurus() call first.
	SQUERY Q;
	Q.OpenThesaurus("/tmp/isearch2_test_squery_nonexistent", "db");
	SUCCEED();
	// Q goes out of scope here without CloseThesaurus() ever being called.
}
