/* src/confwin.h */
#ifndef _WIN32
#define SIZEOF_SHORT_INT 2
#define SIZEOF_INT 2
#define SIZEOF_LONG_INT 4
#define SIZEOF_LONG_LONG_INT 8
#else
#define SIZEOF_SHORT_INT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG_INT 4
#define SIZEOF_LONG_LONG_INT 8
#endif
