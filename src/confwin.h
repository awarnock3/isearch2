/* src/confwin.h */
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
//
// Windows/DOS counterpart to src/conf.h (which configure generates for
// everyone else) -- gdt.h only includes this file at all when _MSDOS
// or _WIN32 is defined, so the two branches below are NOT "Windows vs.
// everyone else"; they're two Windows/DOS-family data models:
//   #ifndef _WIN32 -- _MSDOS defined without _WIN32: classic 16-bit
//     real-mode MS-DOS (e.g. Borland/Turbo C++ for DOS), where `int`
//     genuinely was 16 bits.
//   #else -- _WIN32 defined: Win32/Win64, the LLP64 data model, where
//     `int` and `long` both stay 32 bits even on 64-bit Windows
//     (unlike Unix's LP64, where `long` is 64 bits -- see conf.h).
// Both branches' values are correct for their respective targets; this
// isn't a swapped-condition bug (an easy first impression without the
// context above -- this file had no comments before).
#ifndef CONFWIN_H
#define CONFWIN_H

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

#endif /* !CONFWIN_H */
