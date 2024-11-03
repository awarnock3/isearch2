// -*- C++ -*-

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
