// $Id: common.cxx,v 1.21 2001/01/31 21:15:16 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1994.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*@@@
File:		common.cxx
Version:	$Revision: 1.21 $
Description:	Common functions
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#if (defined(_MSDOS) || defined(_WIN32)) && !defined(UNIX)
#include <direct.h>
#define DIR_SLASH '\\'
#endif

#ifdef UNIX
#include <unistd.h>
#define DIR_SLASH '/'
#endif

#include "common.hxx"

#ifdef HAS__FUNC__
void
panic(const char *filename, const char *func, long line)
#else
void
panic(const char *filename, long line)
#endif
{
  cerr << endl << "?Panic in line " << line
       << " of file " << filename
#ifdef HAS__FUNC__
       << "[" << func << "()]"
#endif
       << endl;
  (void)perror("Unexpected error condition");
  abort();
}


void 
AddTrailingSlash(STRING* PathName) 
{
  STRINGINDEX x;
  if ( ((x=PathName->GetLength()) > 1) &&
       (PathName->GetChr(x) != DIR_SLASH) ) {
    PathName->Cat(DIR_SLASH);
  }
}


void 
RemovePath(STRING* FileName) 
{
  STRINGINDEX x;
  if ((x=FileName->SearchReverse(DIR_SLASH)) != 0) {
    FileName->EraseBefore(x+1);
  }
}


void 
RemoveFileName(STRING* PathName) 
{
  STRINGINDEX x;
  x = PathName->SearchReverse(DIR_SLASH);
  PathName->EraseAfter(x);
}


void
RemoveFileExtension(STRING* PathName)
{
  STRINGINDEX x;
  // BUGFIX #4: SearchReverse returns 0 when there's no '.', and
  // EraseAfter(0) truncates to zero characters -- so a file with no
  // extension had its whole name wiped instead of being left alone,
  // unlike the analogous no-op guard in RemovePath() above.
  if ((x = PathName->SearchReverse('.')) != 0) {
    PathName->EraseAfter(x);
  }
}


off_t
GetFileSize(const STRING FileName) 
{
  struct stat status;
  if (stat(FileName,&status) != 0) {
    return (off_t)-1;
  }
  return status.st_size;
}


off_t
GetFileSize(const CHR* FileName) 
{
  struct stat status;
  if (stat(FileName,&status) != 0) {
    return (off_t)-1;
  }
  return status.st_size;
}


off_t
GetFileSize(FILE* FilePointer) 
{
  /*
    LONG Position, Size;
    Position = ftell(FilePointer);
    fseek(FilePointer, 0L, SEEK_END);
    Size = ftell(FilePointer);
    fseek(FilePointer, Position, SEEK_SET);
    return Size;
  */
  struct stat status;
  if (fstat(fileno(FilePointer),&status) != 0) {
    return (off_t)-1;
  }
  return status.st_size;

}


// Returs GDT_TRUE if it is a regular file
GDT_BOOLEAN
IsFile(const STRING FileName)
{
#if (defined(_MSDOS) || defined(_WIN32)) && !defined(UNIX)
  struct _stat status;
  if (_stat(FileName,&status) != 0) 
#else
  struct stat status;
  if (stat(FileName,&status) != 0) 
#endif
    {
      return GDT_FALSE;
    }

#if (defined(_MSDOS) || defined(_WIN32)) && !defined(UNIX)
  // BUGFIX #1: was `_S_IFREG && status.st_mode` -- a nonzero constant
  // logically ANDed with the mode word, which is true for anything
  // stat() could report (directories included), not just regular
  // files. Needs the actual bitwise AND against the file-type bits.
  if (status.st_mode & _S_IFREG)
#else
  if (S_ISREG(status.st_mode))
#endif
    {
      return GDT_TRUE;
    }

  return GDT_FALSE;
}


GDT_BOOLEAN
IsFile(const CHR* FileName)
{
#if (defined(_MSDOS) || defined(_WIN32)) && !defined(UNIX)
  struct _stat status;
  if (_stat(FileName,&status) != 0) 
#else
  struct stat status;
  if (stat(FileName,&status) != 0) 
#endif
    {
      return GDT_FALSE;
    }

#if (defined(_MSDOS) || defined(_WIN32)) && !defined(UNIX)
  // BUGFIX #1: was `_S_IFREG && status.st_mode` -- a nonzero constant
  // logically ANDed with the mode word, which is true for anything
  // stat() could report (directories included), not just regular
  // files. Needs the actual bitwise AND against the file-type bits.
  if (status.st_mode & _S_IFREG)
#else
  if (S_ISREG(status.st_mode))
#endif
    {
      return GDT_TRUE;
    }

  return GDT_FALSE;
}


void 
ExpandFileSpec(STRING* FileSpec) 
{
#if defined(_MSDOS) || defined(_WIN32)
  if ( FileSpec->GetLength() >= 3 &&
       isalpha(FileSpec->GetChr(1)) &&
       FileSpec->GetChr(2) == ':' &&
       FileSpec->GetChr(3) == DIR_SLASH) {
    return;
  }
#endif
  if (FileSpec->GetChr(1) == DIR_SLASH) {
    return;
  }
  STRING OldFileSpec;
  STRING NewFileSpec;
  STRINGINDEX p, p2;
  STRING s;
  INT Special;
  CHR Cwd[1024];
  if (!getcwd(Cwd, 1022)) {
    strcpy(Cwd, ".");
  }
  NewFileSpec = Cwd;
  AddTrailingSlash(&NewFileSpec);
  OldFileSpec = *FileSpec;
  while ( (p=OldFileSpec.Search(DIR_SLASH)) != 0) {
    Special = 0;
    s = OldFileSpec;
    s.EraseAfter(p);

#if defined (_MSDOS) || defined (_WIN32)
    if ( (s.Equals(".\\")) || (s.Equals("\\")) ) 
#else
    if ( (s.Equals("./")) || (s.Equals("/")) ) 
#endif
      {
	Special = 1;
      }

#if defined (_MSDOS) || defined (_WIN32)
    if (s.Equals("..\\")) 
#else
    if (s.Equals("../")) 
#endif
      {
	Special = 1;
	p2 = NewFileSpec.SearchReverse(DIR_SLASH);
	if (p2 > 1) {
	  NewFileSpec.EraseAfter(p2-1);
	}
	p2 = NewFileSpec.SearchReverse(DIR_SLASH);
	NewFileSpec.EraseAfter(p2);
      }
    if (!Special) {
      NewFileSpec.Cat(s);
    }
    OldFileSpec.EraseBefore(p+1);
  }
  NewFileSpec.Cat(OldFileSpec);
  *FileSpec = NewFileSpec;
}


void
GpSwab(PGPTYPE GpPtr)
{
  // BUGFIX #2: the old body read an uninitialized `GPTYPE Gp;` into
  // the word-swap below whenever CROSS_PLATFORM isn't defined -- true
  // everywhere in this build (see docs/BUG_CATALOG.md#srccommoncxx) --
  // producing stack garbage instead of a byte-swapped value. Simply
  // seeding Gp from *GpPtr isn't enough to fix it either: the
  // word-swap alone only reverses GpPtr's two 16-bit halves, not its
  // 4 individual bytes. A full reversal (the behavior every real
  // caller relies on -- see FC::FlipBytes()/MDTREC::FlipBytes(), only
  // ever invoked when the on-disk data's endianness doesn't match the
  // host's) only happened when CROSS_PLATFORM was defined, because
  // swab() additionally byte-swapped each half before the word-swap
  // ran. Replaced with a direct, unconditional 4-byte reversal that's
  // correct regardless of CROSS_PLATFORM or swab() availability.
  PUCHR p = (PUCHR)GpPtr;
  UCHR tmp;
  tmp = p[0]; p[0] = p[3]; p[3] = tmp;
  tmp = p[1]; p[1] = p[2]; p[2] = tmp;
}


GDT_BOOLEAN 
IsBigEndian() 
{
  UINT2 Test = 1;
  return (*((PUCHR)(&Test)) == 0) ? GDT_TRUE : GDT_FALSE;
}


GDT_BOOLEAN 
DBExists(const STRING FileSpec) 
{
  struct stat info;
  GDT_BOOLEAN exists=GDT_FALSE;
  CHR *CheckName1;
  CHR *CheckName2;
  STRING IndexFile;
  STRING VirtualFile;

  IndexFile = FileSpec;
  IndexFile.Cat(".dbi");
  CheckName1 = IndexFile.NewCString();

  VirtualFile = FileSpec;
  VirtualFile.Cat(".vdb");
  CheckName2 = VirtualFile.NewCString();

  if (stat(CheckName1, &info) ==0) {
    exists = GDT_TRUE;
  } else if (stat(CheckName2, &info) ==0) {
    exists = GDT_TRUE;
  }

  delete [] CheckName1;
  delete [] CheckName2;
  
  return(exists);

}


INT 
rename(const STRING From, const STRING To) {

#if defined(_MSDOS) || defined(_WIN32)

  // MSDOS / WIN32 rename does not remove an existing file so
  // we have to do it ourselves.
  remove(To);
#endif

  // BUGFIX #3: `rename(From, To)` here was an exact-match call to this
  // very overload -- STRING needs a user-defined conversion to reach
  // the C library's rename(const char*, const char*), and overload
  // resolution always prefers an exact match, so every call recursed
  // into itself until the stack overflowed. Casting to const char*
  // first makes the C library overload the exact match instead.
  return rename((const char*)From, (const char*)To);
}

// From Ahti "Ade" Nevalainen <c72092@UWasa.Fi> to handle non-latin chars
GDT_BOOLEAN
IsAlnum(int c)
{
  if ( !(isspace(c)) && !(iscntrl(c)) && !(ispunct(c) )) { 
    return GDT_TRUE;
  } else if (c == '_') {
    return GDT_TRUE;
  }
  return GDT_FALSE;
}
  /*
    INT 
    IsAlnum(int c)
    {
    if ( !(isspace(c)) && !(iscntrl(c)) && !(ispunct(c) )) { 
    return 1;
    return 0;
    }
    */

void 
trimWhitespace(char* s) {
  // trim off end space
  char* p = s + strlen(s) - 1;
  while ( (p >= s) && (isspace(*p)) ) {
    p--;
  }
  *(p+1) = '\0';
  // trim off beginning space
       p = s;
  while ( (*p != '\0') && (isspace(*p)) ) {
    p++;
  }
  memmove(s, p, strlen(p) + 1);
}


DOUBLE
ParseIsoDate(const STRING DateString) {
  // Now, parse out the ISO-8601 date and convert to fractional day
  STRINGINDEX ptr;
  STRING TmpDate,TmpTime;
  INT len;
  INT YYYY,MM,DD;
  INT hh,mm;
  FLOAT ss;
  // BUGFIX #5: both left uninitialized and read by the final `return`
  // below whenever the input doesn't match the sub-format each is set
  // in -- no '-' in the date portion, or a 'T' present but no ':' in
  // the time portion.
  DOUBLE date_val = 0.0, time_val = 0.0;

  TmpDate = DateString;

  if (TmpDate.GetLength() == 0)
    return 0.0;

  ptr = TmpDate.Search('T');
  if (ptr) {
    TmpDate.EraseAfter(ptr-1);
    TmpTime = DateString;
    TmpTime.EraseBefore(ptr+1);
    len = TmpTime.Search('Z');
    TmpTime.EraseAfter(len-1);

    //    cout << "Split " << DateString << " into " << TmpDate << " and " << TmpTime << endl;
  } else {
    TmpDate = DateString;
    time_val = 0.0;
    //    cout << "Date is " << TmpDate << endl;
  }

  // Now, we have the date and time in separate strings, so convert them
  // each to numeric values
  //
  // First, the date
  ptr = TmpDate.Search('-');
  if (ptr) {
    // The format is YYYY-MM-DD
    CHR *tDate;
    tDate = TmpDate.NewCString();
    if (isdigit(tDate[0])) {
      // We might have to look explictly for - if the formatting is bad
      sscanf(tDate,"%4d-%2d-%2d",&YYYY,&MM,&DD);
      snprintf(tDate, TmpDate.GetLength() + 1, "%04d%02d%02d", YYYY, MM, DD);
      TmpDate = tDate;
      date_val = TmpDate.GetFloat();
    } else {
      // BUGFIX #6: this early return used to skip the delete [] below,
      // leaking tDate on every non-digit date. Confirmed with
      // LeakSanitizer via tests/src/test_common.cxx's non-digit-date
      // test.
      delete [] tDate;
      return -99999999.0;
    }
    delete [] tDate;
  }

  ptr = TmpTime.Search(':');
  if (ptr) {
    CHR *tTime;
    tTime = TmpTime.NewCString();
    
    // We might have to look explictly for : if the formatting is bad
    sscanf(tTime,"%2d:%2d:%f",&hh,&mm,&ss);
    time_val = (((ss/60.0) + mm)/60.0 + hh)/24.0;
    delete [] tTime;
  }
  
  return (date_val + time_val);
}
