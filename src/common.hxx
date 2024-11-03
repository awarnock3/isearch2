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

#ifndef COMMON_HXX
#define COMMON_HXX

#include "defs.hxx"
#include "string.hxx"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

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

void        AddTrailingSlash(STRING* PathName);
void        RemovePath(STRING* FileName);
void        RemoveFileName(STRING* PathName);
void        RemoveFileExtension(STRING* PathName);

#ifdef _WIN32
typedef long off_t;
#define true 1
#endif

off_t       GetFileSize(const STRING FileName);
off_t       GetFileSize(const CHR* FileName);
off_t       GetFileSize(FILE* FilePointer);
GDT_BOOLEAN IsFile(const STRING FileName);
GDT_BOOLEAN IsFile(const CHR* FileName);
void        ExpandFileSpec(STRING* FileSpec);
void        GpSwab(PGPTYPE GpPtr);
GDT_BOOLEAN IsBigEndian();
GDT_BOOLEAN DBExists(const STRING FileSpec);
INT         rename(const STRING From, const STRING To);
GDT_BOOLEAN IsAlnum(int c);
void        trimWhitespace(char* s);
DOUBLE      ParseIsoDate(const STRING DateString);

#endif
