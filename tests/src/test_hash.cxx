// Tests for src/hash.hxx / src/hash.cxx (class HASH - open-addressed,
// quadratic-probing hash table). Linked against the real implementation
// via TEST_ENGINE_SRCS in the top-level Makefile, not reimplemented here.

#include "catch_amalgamated.hpp"

#include "hash.hxx"

#include <cstdlib>
#include <string>

TEST_CASE("hash.hxx is self-contained", "[hash]") {
	// BUGFIX #1 (see docs/BUG_CATALOG.md#srchashhxx): this header used
	// INT/CHR/STRING without including their definitions. Merely
	// compiling this translation unit -- which includes only hash.hxx
	// among the project's own headers -- is the regression test for
	// that: it wouldn't have compiled beforehand.
	HASH h;
	SUCCEED();
}

TEST_CASE("HASH Insert then Find round-trips a value", "[hash]") {
	HASH h;
	Item_type item;
	item.Key = 42;
	item.Block = strdup("hello");

	REQUIRE(h.Insert(item) == 0);

	Item_type* found = h.Find(42);
	REQUIRE(found != nullptr);
	REQUIRE(std::string(static_cast<CHR*>(found->Block)) == "hello");
	delete found;

	REQUIRE(h.Find(99) == nullptr);
	// item.Block is now owned by h's table (freed by ~HASH()); don't
	// free it here too.
}

TEST_CASE("HASH Check reflects presence without allocating", "[hash]") {
	HASH h;
	Item_type item;
	item.Key = 7;
	item.Block = nullptr;

	REQUIRE(h.Check(7) == 0);
	h.Insert(item);
	REQUIRE(h.Check(7) == 1);
}

TEST_CASE("HASH Insert reports a duplicate key", "[hash]") {
	HASH h;
	Item_type item;
	item.Key = 5;
	item.Block = nullptr;

	REQUIRE(h.Insert(item) == 0);
	REQUIRE(h.Insert(item) == 1);
}

TEST_CASE("HASH(0) falls back to the documented default instead of dividing by zero", "[hash]") {
	// BUGFIX #2 coverage: before the fix, TableSize==0 made
	// HashFunction's `s % TableSize` a division by zero (SIGFPE).
	HASH h(0);
	Item_type item;
	item.Key = 1;
	item.Block = nullptr;
	REQUIRE(h.Insert(item) == 0);
	REQUIRE(h.Check(1) == 1);
}

TEST_CASE("HASH::AddEntry/GetValue round-trip a name=value entry", "[hash]") {
	HASH h;
	h.AddEntry(STRING("greeting=hello world"));

	STRING out;
	h.GetValue(STRING("greeting"), &out);
	REQUIRE(out == STRING("hello world"));
}

TEST_CASE("HASH::GetValue on a missing key returns an empty string", "[hash]") {
	HASH h;
	STRING out("untouched");
	h.GetValue(STRING("no-such-key"), &out);
	REQUIRE(out == STRING(""));
}

TEST_CASE("HASH::AddEntry with no '=' is a no-op instead of crashing", "[hash]") {
	// BUGFIX #3 coverage: before the fix, strchr returning nullptr for
	// a missing '=' was dereferenced unconditionally (*p='\0') and
	// crashed. Confirmed originally via a standalone ASan repro
	// (SEGV: store to null pointer).
	HASH h;
	h.AddEntry(STRING("no_equals_sign_here"));
	SUCCEED();
}

TEST_CASE("HASH::AddEntry on a duplicate key doesn't leak the discarded value", "[hash]") {
	// BUGFIX #5 (see docs/BUG_CATALOG.md#srchashcxx): r.Block used to be
	// strdup()'d before checking whether the key already existed, and
	// the duplicate-key branch (Check() == true) never freed it --
	// a guaranteed leak on every duplicate AddEntry() call. Confirmed
	// originally via a standalone repro under ASan's LeakSanitizer.
	HASH h;
	h.AddEntry(STRING("greeting=hello"));
	h.AddEntry(STRING("greeting=world"));  // same key, discarded value

	STRING out;
	h.GetValue(STRING("greeting"), &out);
	REQUIRE(out == STRING("hello"));  // first value wins, unchanged
}

TEST_CASE("HASH::AddEntry with an over-long name/value doesn't overflow its buffers", "[hash]") {
	// BUGFIX #4 coverage: before the fix, name/Value (CHR[256]) were
	// filled via unbounded strcpy from a CHR[513] source, so a name or
	// value over 255 characters smashed the stack. Confirmed originally
	// via a standalone ASan repro (stack-buffer-overflow in strcpy).
	STRING LongName(std::string(300, 'A').c_str());
	STRING Entry;
	Entry = LongName;
	Entry += "=short-value";
	HASH h;
	h.AddEntry(Entry);
	SUCCEED();
}
