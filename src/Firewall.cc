#ifdef FIREWALLS

#include "Firewall.h"

#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

bool Firewall::_active = true;
bool Firewall::_fatal = false;

void Firewall::trap()
{
    if (_fatal) {
        cerr << " ### FIREWALL: Exiting." << endl;
        exit(1);
    }

    return;
}

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
        vsprintf(buffer, fmt, args);
        va_end(args);
    } else {
        strcpy(buffer,"exception detected");
    }
    
    cerr << " ### FIREWALL [" << file
#ifdef HAS__FUNC__
         << ':' << func 
#endif
         << ':' << line << "]: "
         << buffer << endl;

    trap(); // Put a spot in the debugger
    
    return;
}

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
        vsprintf(buffer, fmt, args);
        va_end(args);
    } else {
        strcpy(buffer,"Assertion failed");
    }

    cerr << " ### FIREWALL [" << file
#ifdef HAS__FUNC__
         << ':' << func 
#endif
         << ':' << line << "]: "
         << buffer << endl;

    trap(); // Put a spot in the debugger
    return;
}

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
