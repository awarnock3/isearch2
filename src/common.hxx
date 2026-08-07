// $Id: common.hxx,v 1.13 2000/04/01 23:38:14 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of noteworthy
uses of this software. 

3. The names of MCNC and Clearinghouse for Networked Information Discovery
and Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

/*@@@
File:		common.hxx
Version:	$Revision: 1.13 $
Description:	Common functions
Author:		Nassib Nassar, nrn@cnidr.org
Notes:          Added RemoveFileExtension - aw3
                Added macros for max and min - aw3
		Moved rename(STRING, STRING) here
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef COMMON_HXX
#define COMMON_HXX

#include "defs.hxx"
#include "string.hxx"

// Free-function grab bag used tree-wide: filesystem path helpers
// (AddTrailingSlash/RemovePath/RemoveFileName/RemoveFileExtension/
// ExpandFileSpec), file/db existence and size checks (GetFileSize/
// IsFile/DBExists), endianness helpers (GpSwab/IsBigEndian), and a
// handful of string/date utilities (IsAlnum/trimWhitespace/
// ParseIsoDate). No class here, no state -- every function is
// independently callable given just its arguments.

// #ifndef max
// #define max(a,b) (((a) > (b)) ? (a) : (b))
// #endif
//
// #ifndef min
// #define min(a,b) (((a) < (b)) ? (a) : (b))
// #endif

// PANIC is a combination of the definitions from Firewall class and
// the macro from "POSIX Programmer's Guide" by Lewin
#ifdef __GNUC__
#ifndef HAS__FUNC__
#define HAS__FUNC__
#endif
#endif

//
// __HERE__ has to be a preprocessor macro
//
#ifndef __HERE__
#ifdef HAS__FUNC__
#define __HERE__ __FILE__, __LINE__, __FUNCTION__
#else
#define __HERE__ __FILE__, __LINE__
#endif
#endif

#ifndef PANIC
#define PANIC panic(__HERE__);
#endif

#ifdef HAS__FUNC__
void        panic(const char *filename, const char *func, long line);
#else
void        panic(const char *filename, long line);
#endif

// Appends DIR_SLASH ('/' on UNIX) unless PathName is empty or already
// ends in one.
void        AddTrailingSlash(STRING* PathName);
// Keeps only the last path component: erases everything up to and
// including the last DIR_SLASH. A no-op if there's no DIR_SLASH.
void        RemovePath(STRING* FileName);
// Keeps only the directory prefix (through the last DIR_SLASH,
// inclusive): the complement of RemovePath(). No DIR_SLASH means no
// directory, so the result is the empty string, not the input
// unchanged -- see RemovePath() for the case that IS a no-op.
void        RemoveFileName(STRING* PathName);
// Erases everything after the last '.' (the dot itself is kept). A
// no-op if PathName has no '.'.
void        RemoveFileExtension(STRING* PathName);

#ifdef _WIN32
typedef long off_t;
#define true 1
#endif

off_t       GetFileSize(const STRING FileName);
off_t       GetFileSize(const CHR* FileName);
off_t       GetFileSize(FILE* FilePointer);
// GDT_TRUE iff FileName exists and stat() reports it as a regular file
// (not a directory, device, etc).
GDT_BOOLEAN IsFile(const STRING FileName);
GDT_BOOLEAN IsFile(const CHR* FileName);
// Rewrites a relative FileSpec into an absolute path against the
// current working directory, resolving "./" and "../" components.
// A no-op if FileSpec is already absolute.
void        ExpandFileSpec(STRING* FileSpec);
// Byte-swaps a GPTYPE (UINT4) in place for endianness conversion.
void        GpSwab(PGPTYPE GpPtr);
GDT_BOOLEAN IsBigEndian();
// GDT_TRUE iff FileSpec has a matching ".dbi" or ".vdb" file on disk.
GDT_BOOLEAN DBExists(const STRING FileSpec);
// Shadows the C library's ::rename(const char*, const char*); unlike
// it, first removes an existing To on MSDOS/WIN32, where rename()
// alone wouldn't replace it.
INT         rename(const STRING From, const STRING To);
// GDT_TRUE for any character that isn't whitespace, control, or
// punctuation, plus '_' -- a Unicode-tolerant alternative to isalnum().
GDT_BOOLEAN IsAlnum(int c);
// Trims leading and trailing whitespace from s in place.
void        trimWhitespace(char* s);
// Parses an ISO-8601 date/time (YYYY-MM-DD, optionally "T"hh:mm:ss"Z")
// into a fractional-day DOUBLE (integer part is the date, fractional
// part is the time of day). Returns 0.0 for an empty DateString and
// -99999999.0 if the date portion isn't digits.
DOUBLE      ParseIsoDate(const STRING DateString);

#endif
