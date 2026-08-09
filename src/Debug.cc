// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
// NOTE: not built by the top-level Makefile (no TEST_ENGINE_SRCS or
// production rule references this file) and DEBUGCLASS is never
// defined anywhere in this tree, so everything below is dead code in
// the current build -- see the note in Debug.h. Processed anyway; see
// docs/BUG_CATALOG.md for the real bugs found and fixed here.

#ifdef DEBUGCLASS

#include "Debug.h"

#include <iostream>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char * Debug::_environment_setting = 0;
char * Debug::_active_categories[1024];

Debug::Debug(const char *str) : _category(str), _active(false)
{
    _init();
    
    _active = _isActiveCategory(_category);
}

/// Logs a formatted message to cerr, prefixed with category/location,
/// when this instance is active(); a no-op otherwise.
void Debug::out(const char *file, long line,
#ifdef HAS__FUNC__
                const char *func,
#endif
                const char *fmt, ...) const
{
    if (_active) {
        va_list args;
        char buffer[2048];

        va_start(args, fmt);
        // BUGFIX #1 (docs/BUG_CATALOG.md#srcdebugh): vsprintf() writes
        // into `buffer` with no bound, regardless of how long the
        // formatted message actually is -- any caller passing a
        // format/args combination producing 2048+ bytes overflows this
        // fixed stack buffer. Bounded via vsnprintf(); confirmed via a
        // before/after test-revert under ASan.
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        
        // BUGFIX #4 (docs/BUG_CATALOG.md#srcdebugh): `cerr`/`endl` were
        // used unqualified with no `using namespace std;` or `std::`
        // prefix anywhere in this file or its own #includes -- this
        // file would fail to compile the moment DEBUGCLASS is defined,
        // unless some other already-compiled header in the same
        // translation unit happened to pull in `using namespace std;`
        // first (not guaranteed, and not the case when this file is
        // compiled on its own, as this project's tests now do).
        std::cerr << " ### DEBUG_INFO(" << _category << ") ["
             << file
#ifdef HAS__FUNC__
             << ':' << func
#endif
             << ':' << line << "]: "
             << buffer << std::endl;
        
        trap(); // Put a spot in the debugger
    }

    return;
}

// This is only for using from the debugger
void Debug::trap()
{
    return;
}

/// Linear-scans the categories parsed by _init() from DEBUG_OPT for an
/// exact match.
bool Debug::_isActiveCategory(const char * category)
{
    for (int i = 0; _active_categories[i]; ++i) {
        if (!strcmp(_active_categories[i], category)) {
            return true;
        }
        
    }
    
    return false;
}

/// Parses DEBUG_OPT into _active_categories on the first call only
/// (guarded by the static `initialized`); every later call is a no-op.
void Debug::_init()
{
    static bool initialized = false;

    if (initialized) return;
    
    char * c = getenv("DEBUG_OPT");
    
    if (!c) {
        _active_categories[0] = 0;
        initialized = true;
        return;
    }
    
    _environment_setting = strdup(c);
    if (!_environment_setting) {
        std::cerr << " ### DEBUG_INFO: Could not allocate memory for Debug system" << std::endl;
        
        _active_categories[0] = 0;
        initialized = true;
        
        return;
    }
    
    int i = 0;
    char * t = strtok(_environment_setting, ";");

    // BUGFIX #2 (docs/BUG_CATALOG.md#srcdebugh): _active_categories is a
    // fixed 1024-entry array; the loop below had no bound on `i`, so a
    // DEBUG_OPT with 1024+ ';'-separated categories overflowed it.
    // Stopping one slot early leaves room for the terminating 0 below.
    // Confirmed via a before/after test-revert under ASan.
    while (t && i < 1023) {
        _active_categories[i] = t;
        ++i;
        t = strtok(0, ";");
    }

    _active_categories[i] = 0;

    // BUGFIX #3 (docs/BUG_CATALOG.md#srcdebugh): `initialized` was never
    // set on this path, so `_init()` re-ran (re-parsing DEBUG_OPT and
    // leaking the previous `strdup()` into `_environment_setting`) on
    // every single Debug construction instead of just the first, making
    // the `static bool initialized` guard above dead.
    initialized = true;

    return;
}

#endif
// END OF FILE
