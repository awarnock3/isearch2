// Tests for src/mergeunit.hxx / src/mergeunit.cxx (class MERGEUNIT).
// Linked against the real implementation via TEST_ENGINE_SRCS in the
// top-level Makefile, not reimplemented here.
//
// MERGEUNIT::Initialize() requires a real PIDBOBJ, FILEMAP, and backing
// file -- a disproportionate fixture for this file's turn (matching the
// judgment call already made for src/Iindex.cxx's AddFile(), see
// docs/BUG_CATALOG.md#srciindexcxx). The bugs fixed here are both
// exercised by plain construction/destruction instead, which needs no
// such fixture: BUGFIX #2's delete/delete[] mismatch fires on the
// constructor's own initial array allocations, and BUGFIX #3's
// uninitialized Parent/fp/Map/ID/Gp are precisely the members left
// untouched by *not* calling Initialize().

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
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "filemap.hxx"
#include "mergeunit.hxx"

#include <type_traits>

TEST_CASE("MERGEUNIT is non-copyable", "[mergeunit]") {
	// BUGFIX #1 (docs/BUG_CATALOG.md#srcmergeunithxx): MERGEUNIT owns
	// four heap-allocated arrays with no correct copy semantics; rather
	// than implement deep-copy semantics nothing in the tree needs, the
	// copy constructor and operator= are both deleted.
	STATIC_REQUIRE_FALSE(std::is_copy_constructible<MERGEUNIT>::value);
	STATIC_REQUIRE_FALSE(std::is_copy_assignable<MERGEUNIT>::value);
}

TEST_CASE("MERGEUNIT constructs and destroys without a delete/delete[] mismatch", "[mergeunit]") {
	// BUGFIX #2 (most severe finding in this file): list/Tag/Start were
	// freed with scalar `delete` in ~MERGEUNIT() despite being allocated
	// via array `new` in the constructor -- confirmed under ASan
	// (alloc-dealloc-mismatch) on exactly this basic construct-then-
	// destroy usage, matching the live `MERGEUNIT A[2];` at
	// src/index.cxx:2438. Also exercises BUGFIX #3 (Parent/fp/Map/ID/Gp
	// left indeterminate): since Initialize() is never called here, the
	// destructor's `if(fp)` check runs against whatever the constructor
	// left fp as -- must not crash.
	MERGEUNIT Unit;
	SUCCEED();
}

TEST_CASE("MERGEUNIT array construction/destruction does not trigger a delete/delete[] mismatch", "[mergeunit]") {
	// Same as above, but via array new/delete (MERGEUNIT's actual live
	// usage pattern at src/index.cxx:2438: `MERGEUNIT A[2];`).
	MERGEUNIT* Units = new MERGEUNIT[2];
	delete [] Units;
	SUCCEED();
}
