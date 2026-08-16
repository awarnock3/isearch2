// Tests for src/Debug.h / src/Debug.cc (class Debug).
//
// Neither file is included anywhere else in src/, doctype/, or
// Isearch-cgi/, and DEBUGCLASS is never defined by the top-level
// Makefile -- both files are effectively dead code in the current
// build (see the note at the top of Debug.h). Debug.cc is therefore
// NOT added to TEST_ENGINE_SRCS; compiling it there with the ambient
// build flags would still produce an empty translation unit (the whole
// file's content is guarded by `#ifdef DEBUGCLASS`). Instead, this file
// privately compiles its own copy of the real implementation by
// defining DEBUGCLASS before including Debug.cc directly, giving this
// one translation unit exclusive ownership of Debug's static state
// (_active_categories, _environment_setting, and _init()'s "only run
// once" latch) without affecting anything else in the suite.
#define DEBUGCLASS 1
#include "Debug.cc"

#include "catch_amalgamated.hpp"

#include <cstdlib>
#include <string>

TEST_CASE("Debug survives an oversized DEBUG_OPT category list and an oversized out() message", "[debug]") {
	// Debug::_init() only ever runs once per process (a `static bool
	// initialized` guard -- see BUGFIX #3), reading DEBUG_OPT on the
	// first Debug object constructed anywhere in this translation unit.
	// This is the only test in this file, so it owns that first call.

	// BUGFIX #2: _active_categories is a fixed 1024-entry array; build a
	// DEBUG_OPT with more entries than that to exercise the bound that
	// used to be missing from the ';'-token-parsing loop in _init().
	std::string opt;
	for (int i = 0; i < 1030; ++i) {
		if (i) opt += ";";
		opt += "cat" + std::to_string(i);
	}
	setenv("DEBUG_OPT", opt.c_str(), 1);

	Debug d("cat0");
	REQUIRE(d.active());

	// A category never listed in DEBUG_OPT must not be active.
	Debug inactive("not-a-listed-category");
	REQUIRE_FALSE(inactive.active());

	// BUGFIX #1: out() used to vsprintf() into a fixed 2048-byte stack
	// buffer with no bound on the formatted message's length.
	std::string longMsg(4096, 'x');
	d.out(__HERE__, "%s", longMsg.c_str());

	SUCCEED();
}
