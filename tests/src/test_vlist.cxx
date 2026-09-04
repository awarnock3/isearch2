// Tests for src/vlist.hxx / src/vlist.cxx (class VLIST - Doubly Linked
// Circular List Base Class). Linked against the real implementation via
// TEST_ENGINE_OBJS in the top-level Makefile, not reimplemented here.
//
// AddNode is protected, so multi-node-circle tests go through a minimal
// subclass that exposes it -- same pattern as test_opobj.cxx's TestOp.
//
// IMPORTANT: ~VLIST() unconditionally cascades `delete Next` through
// the rest of the circle (see docs/BUG_CATALOG.md#srcvlisthxx, "Found
// but out of scope"). That's only safe when every attached node is
// heap-allocated and torn down deliberately (Clear(), or a single
// well-chosen delete) -- never via ordinary stack unwind of locals
// still linked to each other. Every multi-node circle built below uses
// heap-allocated nodes for anything attached via AddNode, and calls
// Clear() on the (stack-allocated) anchor before it goes out of scope.

#include "catch_amalgamated.hpp"

#include "vlist.hxx"

namespace {

class TestNode : public VLIST {
public:
	// Without this, TestNode's compiler-generated operator=(const
	// TestNode&) would hide VLIST::operator=(const VLIST&) from lookup
	// entirely (ordinary C++ name-hiding, same reason FCT declares its
	// own operator= instead of relying on VLIST's).
	using VLIST::operator=;
	void PublicAddNode(VLIST* NewEntryPtr) { AddNode(NewEntryPtr); }
};

}  // namespace

TEST_CASE("VLIST default-constructs as a circle of one", "[vlist]") {
	VLIST a;
	REQUIRE(a.GetTotalEntries() == 0);
}

TEST_CASE("VLIST AddNode links nodes into a circle", "[vlist]") {
	TestNode a;
	a.PublicAddNode(new VLIST());
	a.PublicAddNode(new VLIST());
	REQUIRE(a.GetTotalEntries() == 2);
	a.Clear();  // deletes the two heap nodes, leaves `a` a solo circle
}

TEST_CASE("VLIST copy constructor produces a standalone new circle", "[vlist]") {
	TestNode a;
	a.PublicAddNode(new VLIST());
	VLIST* b = new VLIST();
	a.PublicAddNode(b);
	REQUIRE(a.GetTotalEntries() == 2);

	VLIST copy(*b);
	REQUIRE(copy.GetTotalEntries() == 0);
	// The source circle must be untouched by copy-construction.
	REQUIRE(a.GetTotalEntries() == 2);

	a.Clear();
}

TEST_CASE("VLIST operator= detaches from its old circle and becomes standalone", "[vlist]") {
	TestNode a;
	VLIST* b = new VLIST();
	a.PublicAddNode(b);
	a.PublicAddNode(new VLIST());
	REQUIRE(a.GetTotalEntries() == 2);

	VLIST other;
	*b = other;
	// b is now a standalone circle of one...
	REQUIRE(b->GetTotalEntries() == 0);
	// ...and a's circle must still be valid without b.
	REQUIRE(a.GetTotalEntries() == 1);

	delete b;  // safe: b is solo now, no cascade
	a.Clear();
}

TEST_CASE("VLIST operator= self-assignment does not corrupt the circle", "[vlist]") {
	TestNode a;
	VLIST* b = new VLIST();
	a.PublicAddNode(b);
	a.PublicAddNode(new VLIST());

	*b = *b;
	REQUIRE(b->GetTotalEntries() == 0);
	REQUIRE(a.GetTotalEntries() == 1);

	delete b;
	a.Clear();
}

TEST_CASE("VLIST copies and their sources survive independent destruction", "[vlist]") {
	// Same shape of concern as the original bug (see
	// docs/AUTOPILOT_LOG.md#srcvlisthxx): copy-construct one node from
	// another, then destroy both. Prior to the fix this corrupted the
	// circle and double-freed under ASan/UBSan.
	VLIST* a = new VLIST();
	VLIST* b = new VLIST(*a);
	delete b;
	delete a;
	SUCCEED("no ASan/UBSan failure on independent destruction");
}
