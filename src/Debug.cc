#ifdef DEBUGCLASS

#include "Debug.h"

#include <iostream.h>

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
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        cerr << " ### DEBUG_INFO(" << _category << ") ["
             << file
#ifdef HAS__FUNC__
             << ':' << func 
#endif
             << ':' << line << "]: "
             << buffer << endl;
        
        trap(); // Put a spot in the debugger
    }

    return;
}

// This is only for using from the debugger
void Debug::trap()
{
    return;
}

bool Debug::_isActiveCategory(const char * category)
{
    for (int i = 0; _active_categories[i]; ++i) {
        if (!strcmp(_active_categories[i], category)) {
            return true;
        }
        
    }
    
    return false;
}

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
        cerr << " ### DEBUG_INFO: Could not allocate memory for Debug system" << endl;
        
        _active_categories[0] = 0;
        initialized = true;
        
        return;
    }
    
    int i = 0;
    char * t = strtok(_environment_setting, ";");
    
    while (t) {
        _active_categories[i] = t;
        ++i;
        t = strtok(0, ";");
    }
    
    _active_categories[i] = 0;

    return;
}

#endif
// END OF FILE
