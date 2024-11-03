// $Id: fpt.hxx,v 1.4 1998/05/12 16:49:03 cnidr Exp $
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
File:		fpt.hxx
Version:	1.00
$Revision: 1.4 $
Description:	Class FPT - File Pointer Table
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef FPT_HXX
#define FPT_HXX

#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "fprec.hxx"

typedef PFILE* PPFILE;

class FPT {
public:
  FPT();
  FPT(const INT TableSize);
  PFILE ffopen(const STRING& FileName, const CHR *Type);
  INT   ffclose(FILE *FilePointer);
  void  CloseAll();
  ~FPT();

private:
  void   Init(const INT TableSize);
  INT    Lookup(const STRING& FileName);
  INT    Lookup(const FILE *FilePointer);
  void   HighPriority(const INT Index);
  void   LowPriority(const INT Index);
  FPREC *Table;
  INT    TotalEntries;
  INT    MaximumEntries;
};

typedef FPT* PFPT;

#endif
