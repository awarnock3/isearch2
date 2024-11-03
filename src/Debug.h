// -*- C++ -*-

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
class Debug
{
public:
    Debug(const char *) {}
    static bool active() { return false; }
    static void out(const char *, ...) { return; }
};
#else
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
