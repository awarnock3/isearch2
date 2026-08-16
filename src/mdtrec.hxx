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
File:		mdtrec.hxx
Version:	1.00
Description:	Class MDTREC - Multiple Document Table Record
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef MDTREC_HXX
#define MDTREC_HXX

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
//#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"

// A Multiple Document Table record: one indexed document's key,
// doctype, path/file name, and its [GlobalFileStart,GlobalFileEnd) /
// [LocalRecordStart,LocalRecordEnd) byte-offset spans within the
// corpus. All string fields are fixed-size CHR buffers (not STRING),
// matching this record's on-disk, fixed-length layout (see
// MDT::GetEntry/AddEntry, src/mdt.cxx, which fread()/fwrite() it as a
// raw block) -- Set* accessors always null-terminate within bounds
// (via STRING::GetCString); Get* accessors never assume the buffer
// already is, since a corrupt or truncated on-disk record can leave
// one that isn't (see BUGFIX #1/#2 in source).
class MDTREC {
public:
  MDTREC();
  // Deep-copies every field; safe under self-assignment. Bounded even
  // if OtherMdtRec's buffers aren't null-terminated.
  MDTREC& operator=(const MDTREC& OtherMdtRec);
  void SetKey(const STRING& NewKey);
  void GetKey(STRING* StringBuffer) const;
  void SetDocumentType(const STRING& NewDocumentType);
  void GetDocumentType(STRING* StringBuffer) const;
  void SetPathName(const STRING& NewPathName);
  void GetPathName(STRING* StringBuffer) const;
  void SetFileName(const STRING& NewFileName);
  void GetFileName(STRING* StringBuffer) const;
  // PathName immediately followed by FileName, concatenated.
  void GetFullFileName(STRING* StringBuffer) const;
  void SetGlobalFileStart(const GPTYPE NewGlobalFileStart);
  GPTYPE GetGlobalFileStart() const;
  void SetGlobalFileEnd(const GPTYPE NewGlobalFileEnd);
  GPTYPE GetGlobalFileEnd() const;
  void SetLocalRecordStart(const GPTYPE NewLocalRecordStart);
  GPTYPE GetLocalRecordStart() const;
  void SetLocalRecordEnd(const GPTYPE NewLocalRecordEnd);
  GPTYPE GetLocalRecordEnd() const;
  void SetDeleted(const GDT_BOOLEAN Flag);
  GDT_BOOLEAN GetDeleted() const;
  // Byte-swaps the four GPTYPE offset fields in place, for cross-endian
  // file I/O.
  void FlipBytes();
  ~MDTREC();

private:
  CHR Key[DocumentKeySize];
  CHR DocumentType[DocumentTypeSize];
  CHR PathName[DocPathNameSize];
  CHR FileName[DocFileNameSize];
  GPTYPE GlobalFileStart;
  GPTYPE GlobalFileEnd;
  GPTYPE LocalRecordStart;
  GPTYPE LocalRecordEnd;
  CHR Deleted;
};

typedef MDTREC* PMDTREC;

#endif
