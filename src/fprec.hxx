// $Id: fprec.hxx,v 1.4 1998/05/12 16:49:02 cnidr Exp $
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
File:		fprec.hxx
Version:	1.00
$Revision: 1.4 $
Description:	Class FPREC - File Pointer Record
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef FPREC_HXX
#define FPREC_HXX

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"

// One entry in FPT's (src/fpt.hxx) open-file table: a file name, the
// FILE* it's currently open under (if any -- FPREC doesn't open/close
// it itself, just records FPT's bookkeeping), that file's open mode,
// and an LRU-style Priority plus a Closed flag FPT uses to decide which
// file to close when the table is full.
class FPREC {
public:
  FPREC();
  // Copies every field, including Priority and Closed; safe under
  // self-assignment.
  FPREC& operator=(const FPREC& OtherFprec);
  // Stores NewFileName expanded to an absolute path (see
  // common.hxx's ExpandFileSpec).
  void        SetFileName(const STRING& NewFileName);
  void        GetFileName(STRING *StringBuffer) const;
  void        SetFilePointer(FILE* NewFilePointer);
  PFILE       GetFilePointer() const;
  void        SetPriority(const INT NewPriority);
  INT         GetPriority() const;
  void        SetClosed(const GDT_BOOLEAN NewClosed);
  GDT_BOOLEAN GetClosed() const;
  void        SetOpenMode(const STRING& NewOpenMode);
  void        GetOpenMode(STRING *StringBuffer) const;
  ~FPREC();

private:
  STRING      FileName;
  PFILE       FilePointer;
  STRING      OpenMode;
  INT         Priority;
  GDT_BOOLEAN Closed;
};

typedef FPREC* PFPREC;

#endif
