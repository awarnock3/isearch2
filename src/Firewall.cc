// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
// NOTE: not built by the top-level Makefile and FIREWALLS is never
// defined anywhere in this tree, so everything below is dead code in
// the current build -- see the note in Firewall.h. Processed anyway;
// see docs/BUG_CATALOG.md for the real bugs found and fixed here.

#ifdef FIREWALLS

#include "Firewall.h"

#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
// BUGFIX #3 (docs/BUG_CATALOG.md#srcfirewallh): strcpy()/strcmp() below
// are used with no #include providing their declarations -- this file
// would fail to compile the moment FIREWALLS is defined. Confirmed
// directly: compiling with -DFIREWALLS before this fix fails with
// "'strcpy' was not declared in this scope".
#include <string.h>

bool Firewall::_active = true;
bool Firewall::_fatal = false;

/// Exits the process (via exit(1)) if FIREWALLS=fatal was set at
/// startup; otherwise a no-op, matching its "put a spot in the
/// debugger" call-site comments in hit()/assert().
void Firewall::trap()
{
    if (_fatal) {
        // BUGFIX #2 (docs/BUG_CATALOG.md#srcfirewallh): `cerr`/`endl`
        // used unqualified with no `using namespace std;` or `std::`
        // prefix anywhere in this file or its own #includes -- this
        // file would fail to compile the moment FIREWALLS is defined,
        // the same issue and fix as src/Debug.cc's BUGFIX #4.
        std::cerr << " ### FIREWALL: Exiting." << std::endl;
        exit(1);
    }

    return;
}

/// Logs a formatted message (or "exception detected" if fmt is empty)
/// to cerr when active(), then calls trap().
void Firewall::hit(const char *file, long line,
#ifdef HAS__FUNC__
    const char *func,
#endif
    const char *fmt, ...)
{
    char buffer[2048];

    _init();
    if (!_active) return;
    if (fmt[0]) {
        va_list args;

        va_start(args, fmt);
        // BUGFIX #1 (docs/BUG_CATALOG.md#srcfirewallh): vsprintf()
        // writes into `buffer` with no bound, regardless of how long
        // the formatted message actually is -- the same unbounded-write
        // bug as src/Debug.cc's BUGFIX #1, here at both of this file's
        // two vsprintf() call sites (hit() and assert() below). Bounded
        // via vsnprintf(); confirmed via a before/after test-revert
        // under ASan.
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
    } else {
        strcpy(buffer,"exception detected");
    }

    std::cerr << " ### FIREWALL [" << file
#ifdef HAS__FUNC__
         << ':' << func
#endif
         << ':' << line << "]: "
         << buffer << std::endl;

    trap(); // Put a spot in the debugger

    return;
}

/// If cond is false, logs a formatted message (or "Assertion failed" if
/// fmt is empty) to cerr when active(), then calls trap(); a no-op when
/// cond is true.
void Firewall::assert(
    bool cond, const char *file, long line,
#ifdef HAS__FUNC__
    const char *func,
#endif
    const char *fmt, ...)
{
    char buffer[2048];

    if (cond) return;

    _init();

    if (!_active) return;
    if (fmt[0]) {
        va_list args;

        va_start(args, fmt);
        // BUGFIX #1, second call site -- see hit() above.
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
    } else {
        strcpy(buffer,"Assertion failed");
    }

    std::cerr << " ### FIREWALL [" << file
#ifdef HAS__FUNC__
         << ':' << func
#endif
         << ':' << line << "]: "
         << buffer << std::endl;

    trap(); // Put a spot in the debugger
    return;
}

/// Reads the FIREWALLS environment variable on the first call only
/// (guarded by the static `init`): unset keeps the defaults
/// (_active=true, _fatal=false), "off" clears _active, "fatal" sets
/// _fatal.
void Firewall::_init()
{
    static bool init = true;

    if (init) {
        init = false;
        char *c = getenv("FIREWALLS");
        if (!c) {
            // The defaults: _active = true; _fatal = false;
        } else if (!strcmp(c,"off")) { // Use stricmp, if you have it
            _active = false;
        } else if (!strcmp(c,"fatal")) { // Use stricmp, if you have it
            _fatal = true;
        }
    }

    return;
}

#endif
// END OF FILE
