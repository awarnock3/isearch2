// Tests for src/merge.hxx / src/merge.cxx (heapsort utilities used for
// external/internal merge sorting). Linked against the real
// implementation via TEST_ENGINE_SRCS in the top-level Makefile, not
// reimplemented here.
//
// Note: MERGE::AddChunk is declared in merge.hxx but has no definition
// anywhere in this tree and nothing calls it -- not exercised here,
// same as it's not exercised (or implemented) anywhere else.

#include "catch_amalgamated.hpp"

#include "gdt.h"
#include "defs.hxx"
#include "merge.hxx"

#include <vector>

namespace {
int GpCompare(const void *a, const void *b) {
	GPTYPE x = *(const GPTYPE*)a;
	GPTYPE y = *(const GPTYPE*)b;
	if (x < y) return -1;
	if (x > y) return 1;
	return 0;
}

int IntCompare(const void *a, const void *b) {
	int x = *(const int*)a;
	int y = *(const int*)b;
	if (x < y) return -1;
	if (x > y) return 1;
	return 0;
}
} // namespace

TEST_CASE("GpHsort sorts a descending array into ascending order", "[merge]") {
	std::vector<GPTYPE> data = {8, 7, 6, 5, 4, 3, 2, 1};
	GpHsort(data.data(), data.size(), GpCompare);
	for (size_t i = 1; i < data.size(); i++) {
		REQUIRE(data[i-1] <= data[i]);
	}
	REQUIRE(data.front() == 1);
	REQUIRE(data.back() == 8);
}

TEST_CASE("GpHsort is correct across a range of sizes including the off-by-one boundary", "[merge]") {
	// BUGFIX #1 coverage: buildGpHeap() guarded its data[childpos+1]
	// read with `childpos < heapsize` instead of `childpos + 1 <
	// heapsize`. n=8 is the smallest size where GpHsort's first
	// (full-heapsize) pass reaches a childpos exactly at heapsize-1 --
	// confirmed originally via a standalone ASan repro
	// (heap-buffer-overflow read).
	for (size_t n = 1; n <= 20; n++) {
		std::vector<GPTYPE> data(n);
		for (size_t i = 0; i < n; i++) {
			data[i] = (GPTYPE)(n - i);
		}
		GpHsort(data.data(), n, GpCompare);
		for (size_t i = 1; i < n; i++) {
			REQUIRE(data[i-1] <= data[i]);
		}
	}
}

TEST_CASE("hsort sorts a descending int array into ascending order", "[merge]") {
	std::vector<int> data = {8, 7, 6, 5, 4, 3, 2, 1};
	hsort(data.data(), data.size(), sizeof(int), IntCompare);
	for (size_t i = 1; i < data.size(); i++) {
		REQUIRE(data[i-1] <= data[i]);
	}
}

TEST_CASE("hsort is correct across a range of sizes", "[merge]") {
	for (size_t n = 1; n <= 20; n++) {
		std::vector<int> data(n);
		for (size_t i = 0; i < n; i++) {
			data[i] = (int)(n - i);
		}
		hsort(data.data(), n, sizeof(int), IntCompare);
		for (size_t i = 1; i < n; i++) {
			REQUIRE(data[i-1] <= data[i]);
		}
	}
}
