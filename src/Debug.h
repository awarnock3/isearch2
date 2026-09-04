// -*- C++ -*-

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
// NOTE: neither this file nor its implementation (src/Debug.cc) is
// included anywhere else in src/, doctype/, or Isearch-cgi/, and
// DEBUGCLASS is never defined by the top-level Makefile -- both files
// are effectively orphaned/dead in the current build (the stub Debug
// class below is what every translation unit would actually get if it
// ever did include this header). Processed anyway since it carries a
// PROCESSING_STATUS.md row and its bugs are real, reachable the moment
// DEBUGCLASS is defined; see docs/BUG_CATALOG.md for the full note.

#ifndef DebugInfo_h
#define DebugInfo_h

#ifdef __GNUC__
#ifndef HAS__FUNC__
#define HAS__FUNC__ (1)
#endif
#endif

#ifdef HAS__FUNC__
#define __HERE__ __FILE__, __LINE__, __FUNCTION__
#else
#define __HERE__ __FILE__, __LINE__
#endif

#ifndef DEBUGCLASS
/// No-op stand-in used whenever DEBUGCLASS isn't defined (the case
/// everywhere in this tree's current build) -- every call compiles away
/// to nothing.
class Debug
{
public:
    Debug(const char *) {}
    static bool active() { return false; }
    static void out(const char *, ...) { return; }
};
#else
/// Category-gated debug logger: active() is true only when this
/// instance's category string was listed in the ';'-separated
/// DEBUG_OPT environment variable at first use (see Debug::_init() in
/// Debug.cc); out() logs to cerr only when active().
class Debug
{
public:
    Debug(const char *);

    bool active() const { return _active; }
    void out(const char *file, long line,
#ifdef HAS__FUNC__
             const char *func,
#endif
             const char *, ...) const;

    static void trap();

private:
    const char *_category; // Not a string, so no overhead until used
    bool _active;

    static void _init();
    static bool _isActiveCategory(const char * category);

    static char * _environment_setting;
    static char * _active_categories[1024]; // Avoid using STL, for now
};
#endif


#endif

// END OF FILE
