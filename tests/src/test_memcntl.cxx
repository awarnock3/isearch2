// Tests for src/memcntl.hxx / src/memcntl.cxx (AllocSafe/FreeSafe, a
// malloc-style linked-list memory tracker used by src/marclib.cxx).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "memcntl.hxx"

TEST_CASE("AllocSafe returns usable memory and links it into *base", "[memcntl]") {
	struct MemBlock* base = nullptr;
	char* p = AllocSafe(&base, 16, 0, GENERALMEM);
	REQUIRE(p != nullptr);
	REQUIRE(base != nullptr);
	REQUIRE(base->data == p);
	REQUIRE(base->memsize == 16);

	FreeSafe(&base, nullptr, 1);
}

TEST_CASE("AllocSafe with MEMF_CLEAR zeroes the returned memory", "[memcntl]") {
	struct MemBlock* base = nullptr;
	char* p = AllocSafe(&base, 8, MEMF_CLEAR, GENERALMEM);
	REQUIRE(p != nullptr);
	for (int i = 0; i < 8; i++) {
		REQUIRE(p[i] == '\0');
	}
	FreeSafe(&base, nullptr, 1);
}

TEST_CASE("FreeSafe(flag=0) removes exactly the named block, leaving others intact", "[memcntl]") {
	struct MemBlock* base = nullptr;
	char* p1 = AllocSafe(&base, 8, 0, GENERALMEM);
	char* p2 = AllocSafe(&base, 8, 0, GENERALMEM);

	FreeSafe(&base, p1, 0);

	// p2's block should still be reachable and intact.
	bool found = false;
	for (struct MemBlock* b = base; b; b = b->nextmem) {
		if (b->data == p2) found = true;
	}
	REQUIRE(found);

	FreeSafe(&base, nullptr, 1);
}

TEST_CASE("FreeSafe(flag=1) frees every block and leaves *base null", "[memcntl]") {
	// BUGFIX #3 coverage: before the fix, *base still pointed at the
	// first (already-deleted) block after a free-all, instead of
	// nullptr. Confirmed real with a standalone repro: free-all,
	// allocate again (silently absorbing the dangling pointer into the
	// new block's ->nextmem), then free-all a second time -- the second
	// pass reached the dangling pointer and aborted under ASan with
	// heap-use-after-free, then would have double-freed it. This test
	// exercises exactly that sequence; passing under make tests-asan is
	// the point.
	struct MemBlock* base = nullptr;
	AllocSafe(&base, 8, 0, GENERALMEM);
	AllocSafe(&base, 8, 0, GENERALMEM);

	FreeSafe(&base, nullptr, 1);
	REQUIRE(base == nullptr);

	// Reusing the same (now properly empty) list must work cleanly.
	char* p = AllocSafe(&base, 8, 0, GENERALMEM);
	REQUIRE(p != nullptr);
	REQUIRE(base != nullptr);

	FreeSafe(&base, nullptr, 1);
	REQUIRE(base == nullptr);
}

TEST_CASE("FreeSafe on an empty list is a no-op", "[memcntl]") {
	struct MemBlock* base = nullptr;
	REQUIRE(FreeSafe(&base, nullptr, 1) == 0);
	REQUIRE(FreeSafe(&base, (char*)1, 0) == 0);
}
