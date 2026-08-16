// -*- C++ -*-

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
// NOTE: same situation as src/Debug.h (see that file's note) -- neither
// this header nor its implementation (src/Firewall.cc) is included
// anywhere else in src/, doctype/, or Isearch-cgi/, and FIREWALLS is
// never defined by the top-level Makefile, so the stub class below (not
// the real one further down) is what any includer would actually get.
// Processed anyway since it carries a PROCESSING_STATUS.md row; see
// docs/BUG_CATALOG.md for the real bugs found and fixed in Firewall.cc.

#ifndef Firewall_h
#define Firewall_h

//
// If we are compiling using gcc, use __FUNCTION__ builtin to get the function name
//
#ifdef __GNUC__
#ifndef HAS__FUNC__
#define HAS__FUNC__
#endif
#endif

//
// __HERE__ has to be a preprocessor macro
//
#ifdef HAS__FUNC__
#define __HERE__ __FILE__, __LINE__, __FUNCTION__
#else
#define __HERE__ __FILE__, __LINE__
#endif

#ifndef FIREWALLS
/// No-op stand-in used whenever FIREWALLS isn't defined (the case
/// everywhere in this tree's current build) -- every call compiles away
/// to nothing.
class Firewall
{
//
// The production version
//
public:
    static bool active() { return false; }
    static void hit(const char *, ...) { return; }
    static void assert(bool cond, ...) { return; }
};

#else
/// Runtime assertion/trap facility, gated by the FIREWALLS environment
/// variable at first use ("off" disables it, "fatal" makes a hit
/// exit(1) via trap() -- see Firewall::_init() in Firewall.cc).
class Firewall
{
//
// The debugging version
//
public:
    static bool active() { return _active; }
    static void trap();

#ifdef HAS__FUNC__
    static void hit(const char *file, long line, const char *func, const char *fmt = "", ...);
#else
    static void hit(const char *file, long line, const char *fmt = "", ...);
#endif
    
#ifdef HAS__FUNC__
    static void assert(
        bool cond,
        const char *file, long line, const char *func,
        const char *fmt = "", ...
        );
#else
    static void assert(bool cond, const char *file, long line, const char *fmt = "", ...);
#endif

private:
    static bool _active;
    static bool _fatal;

    static void _init();
};
    
#endif

#endif

// END OF FILE
