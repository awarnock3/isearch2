// Tests for src/glist.hxx / src/glist.cxx (class GLIST).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.
//
// GLIST has no destructor (see the class comment in glist.hxx), so
// every cell inserted in these tests is explicitly Delete()d before
// the GLIST goes out of scope -- letting one leak here would just be
// noise on top of the documented, not-fixed-this-turn leak itself.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "glist.hxx"

TEST_CASE("GLIST starts empty", "[glist]") {
	GLIST list;
	REQUIRE(list.IsEmpty() == GDT_TRUE);
	REQUIRE(list.GetLength() == 0);
	REQUIRE(list.First() == nullptr);
	REQUIRE(list.Last() == nullptr);
}

TEST_CASE("GLIST InsertAfter builds a forward-traversable list", "[glist]") {
	GLIST list;
	int a = 1, b = 2, c = 3;
	list.InsertAfter(nullptr, &a);          // list: a
	GPOSITION* pa = list.First();
	list.InsertAfter(pa, &b);               // list: a, b
	GPOSITION* pb = list.Next(pa);
	list.InsertAfter(pb, &c);               // list: a, b, c
	GPOSITION* pc = list.Next(pb);

	REQUIRE(list.GetLength() == 3);
	REQUIRE(list.IsEmpty() == GDT_FALSE);
	REQUIRE(list.First() == pa);
	REQUIRE(list.Last() == pc);
	REQUIRE(*(int*)list.Retrieve(pa) == 1);
	REQUIRE(*(int*)list.Retrieve(pb) == 2);
	REQUIRE(*(int*)list.Retrieve(pc) == 3);
	REQUIRE(list.Next(pc) == nullptr);
	REQUIRE(list.Prev(pa) == nullptr);
	REQUIRE(list.Prev(pc) == pb);
	REQUIRE(list.Next(pa) == pb);

	list.Delete(pa);
	list.Delete(pb);
	list.Delete(pc);
}

TEST_CASE("GLIST Delete from an interior position relinks both neighbors", "[glist]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srcglisthxx): Delete() on an
	// interior cell used to leave the *following* cell's Prev pointer
	// dangling (pointing at the just-freed cell) instead of relinking it
	// to the deleted cell's predecessor. This test's assertions confirm
	// the relinked structure; the use-after-free itself (reading through
	// the stale Prev) is what `make tests-asan` actually catches on a
	// regression, not a plain REQUIRE here.
	GLIST list;
	int a = 1, b = 2, c = 3;
	list.InsertAfter(nullptr, &a);
	GPOSITION* pa = list.First();
	list.InsertAfter(pa, &b);
	GPOSITION* pb = list.Next(pa);
	list.InsertAfter(pb, &c);
	GPOSITION* pc = list.Next(pb);

	list.Delete(pb);  // delete the interior cell

	REQUIRE(list.GetLength() == 2);
	REQUIRE(list.Next(pa) == pc);
	REQUIRE(list.Prev(pc) == pa);  // the relinked pointer
	REQUIRE(*(int*)list.Retrieve(list.Prev(pc)) == 1);

	list.Delete(pa);
	list.Delete(pc);
}

TEST_CASE("GLIST Delete handles head, tail, and singleton cases", "[glist]") {
	GLIST list;
	int a = 1, b = 2, c = 3;
	list.InsertAfter(nullptr, &a);
	GPOSITION* pa = list.First();
	list.InsertAfter(pa, &b);
	GPOSITION* pb = list.Next(pa);
	list.InsertAfter(pb, &c);
	GPOSITION* pc = list.Next(pb);

	list.Delete(pa);  // head
	REQUIRE(list.First() == pb);
	REQUIRE(list.Prev(pb) == nullptr);

	list.Delete(pc);  // tail
	REQUIRE(list.Last() == pb);
	REQUIRE(list.Next(pb) == nullptr);

	list.Delete(pb);  // singleton
	REQUIRE(list.IsEmpty() == GDT_TRUE);
	REQUIRE(list.First() == nullptr);
}

TEST_CASE("GLIST InsertBefore keeps each atom paired with its own type tag", "[glist]") {
	// BUGFIX #2 (see docs/BUG_CATALOG.md#srcglisthxx): InsertBefore()'s
	// insert-after-then-swap implementation used to swap only the Atom
	// field, leaving each cell's Type tag on the wrong atom after the
	// swap.
	GLIST list;
	int a = 1, b = 2;
	list.InsertAfter(nullptr, &a, 42);
	GPOSITION* pa = list.First();
	list.InsertBefore(pa, &b, 99);

	GPOSITION* p1 = list.First();
	GPOSITION* p2 = list.Next(p1);
	REQUIRE(*(int*)list.Retrieve(p1) == 2);
	REQUIRE(list.DataType(p1) == 99);
	REQUIRE(*(int*)list.Retrieve(p2) == 1);
	REQUIRE(list.DataType(p2) == 42);

	list.Delete(p1);
	list.Delete(p2);
}

TEST_CASE("GLIST InsertBefore into an empty list behaves like InsertAfter(nullptr, ...)", "[glist]") {
	GLIST list;
	int a = 1;
	list.InsertBefore(nullptr, &a, LIST_INT);
	REQUIRE(list.GetLength() == 1);
	REQUIRE(list.DataType(list.First()) == LIST_INT);
	list.Delete(list.First());
}

TEST_CASE("GLIST Update replaces an atom without changing its type tag", "[glist]") {
	GLIST list;
	int a = 1, b = 2;
	list.InsertAfter(nullptr, &a, LIST_INT);
	GPOSITION* pa = list.First();
	REQUIRE(list.Update(pa, &b) == GDT_TRUE);
	REQUIRE(*(int*)list.Retrieve(pa) == 2);
	REQUIRE(list.DataType(pa) == LIST_INT);
	REQUIRE(list.Update(pa, nullptr) == GDT_FALSE);
	list.Delete(pa);
}
