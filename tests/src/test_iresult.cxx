// Tests for src/iresult.hxx / src/iresult.cxx (class IRESULT - a
// search hit as tracked internally during scoring/merging).
// Linked against the real implementation via TEST_ENGINE_OBJS in the
// top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "defs.hxx"
#include "mdt.hxx"
#include "iresult.hxx"

#include <cstdio>
#include <unistd.h>

namespace {

// Owns a real, on-disk MDT (mirrors tests/src/test_filemap.cxx's
// TempMdt) purely so SetMdt()/GetMdt() have a real object to point at.
struct TempMdt {
	STRING Stem;
	MDT* Mdt;

	TempMdt() {
		char tmpl[] = "/tmp/isearch2_test_iresult_XXXXXX";
		int fd = mkstemp(tmpl);
		close(fd);
		remove(tmpl);
		Stem = tmpl;
		Mdt = new MDT(Stem, GDT_FALSE);
	}

	~TempMdt() {
		delete Mdt;
		STRING Fn;
		Fn = Stem; Fn.Cat(".mdt"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdg"); remove(Fn);
		Fn = Stem; Fn.Cat(".mdk"); remove(Fn);
	}
};

}  // namespace

TEST_CASE("IRESULT default-constructs with zeroed fields", "[iresult]") {
	IRESULT r;
	REQUIRE(r.GetMdtIndex() == 0);
	REQUIRE(r.GetHitCount() == 0);
	REQUIRE(r.GetScore() == 0.0);
	REQUIRE(r.GetDbNum() == 0);
	REQUIRE(r.GetMdt() == nullptr);
}

TEST_CASE("IRESULT SetMdtIndex/GetMdtIndex round-trip", "[iresult]") {
	IRESULT r;
	r.SetMdtIndex(42);
	REQUIRE(r.GetMdtIndex() == 42);
}

TEST_CASE("IRESULT SetHitCount/GetHitCount round-trip", "[iresult]") {
	IRESULT r;
	r.SetHitCount(5);
	REQUIRE(r.GetHitCount() == 5);
}

TEST_CASE("IRESULT IncHitCount() increments by one", "[iresult]") {
	IRESULT r;
	r.SetHitCount(5);
	r.IncHitCount();
	REQUIRE(r.GetHitCount() == 6);
}

TEST_CASE("IRESULT IncHitCount(N) increments by N", "[iresult]") {
	IRESULT r;
	r.SetHitCount(5);
	r.IncHitCount(10);
	REQUIRE(r.GetHitCount() == 15);
}

TEST_CASE("IRESULT SetScore/GetScore round-trip", "[iresult]") {
	IRESULT r;
	r.SetScore(1.5);
	REQUIRE(r.GetScore() == 1.5);
}

TEST_CASE("IRESULT IncScore adds to the running score", "[iresult]") {
	IRESULT r;
	r.SetScore(1.0);
	r.IncScore(0.5);
	REQUIRE(r.GetScore() == 1.5);
}

TEST_CASE("IRESULT SetDbNum/GetDbNum round-trip", "[iresult]") {
	IRESULT r;
	r.SetDbNum(3);
	REQUIRE(r.GetDbNum() == 3);
}

TEST_CASE("IRESULT SetMdt/GetMdt round-trip", "[iresult]") {
	TempMdt tmp;
	IRESULT r;
	r.SetMdt(*tmp.Mdt);
	REQUIRE(r.GetMdt() == tmp.Mdt);
}

TEST_CASE("IRESULT operator= copies every field, including Mdt", "[iresult]") {
	TempMdt tmp;
	IRESULT a;
	a.SetMdtIndex(7);
	a.SetHitCount(3);
	a.SetScore(2.5);
	a.SetDbNum(1);
	a.SetMdt(*tmp.Mdt);

	IRESULT b;
	b = a;

	REQUIRE(b.GetMdtIndex() == 7);
	REQUIRE(b.GetHitCount() == 3);
	REQUIRE(b.GetScore() == 2.5);
	REQUIRE(b.GetDbNum() == 1);
	REQUIRE(b.GetMdt() == tmp.Mdt);
}

TEST_CASE("IRESULT operator= self-assignment leaves fields intact", "[iresult]") {
	TempMdt tmp;
	IRESULT r;
	r.SetMdtIndex(9);
	r.SetMdt(*tmp.Mdt);
	r = r;
	REQUIRE(r.GetMdtIndex() == 9);
	REQUIRE(r.GetMdt() == tmp.Mdt);
}
