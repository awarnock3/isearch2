// Tests for src/Firewall.h / src/Firewall.cc (class Firewall).
//
// Same situation as src/Debug.h/src/Debug.cc (see tests/src/test_debug.cxx's
// header comment): neither file is included anywhere else in src/,
// doctype/, or Isearch-cgi/, and FIREWALLS is never defined by the
// top-level Makefile, so both are dead code in the current build.
// Firewall.cc is NOT added to TEST_ENGINE_SRCS for the same reason
// Debug.cc isn't: with the ambient build flags it would compile to an
// empty translation unit. Instead this file privately compiles its own
// copy of the real implementation by defining FIREWALLS before
// including Firewall.cc directly.
#define FIREWALLS 1
#include "Firewall.cc"

#include "catch_amalgamated.hpp"

// catch_amalgamated.hpp pulls in <cassert>, whose function-like `assert`
// macro would otherwise rewrite every `Firewall::assert(...)` call below
// before the compiler ever sees the `Firewall::` qualifier.
#undef assert

#include <string>

TEST_CASE("Firewall::hit and Firewall::assert survive an oversized formatted message", "[firewall]") {
	// FIREWALLS is left unset, so Firewall::_init() keeps its defaults
	// (_active=true, _fatal=false) -- hit()/assert() log but never call
	// exit(1) via trap(), which would otherwise kill this test binary.
	REQUIRE(Firewall::active());

	std::string longMsg(4096, 'x');

	// BUGFIX #1 (docs/BUG_CATALOG.md#srcfirewallh): hit()'s vsprintf()
	// used to write into a fixed 2048-byte stack buffer with no bound on
	// the formatted message's length.
	Firewall::hit(__HERE__, "%s", longMsg.c_str());

	// BUGFIX #1, second call site: assert()'s vsprintf() had the same
	// unbounded write. cond=false so the logging path actually runs.
	Firewall::assert(false, __HERE__, "%s", longMsg.c_str());

	SUCCEED();
}
