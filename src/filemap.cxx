/* $Id: filemap.cxx,v 1.5 1998/05/12 16:49:01 cnidr Exp $ */
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
File:		filemap.cxx
Version:	1.01
Description:	Class FILEMAP
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "common.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "filemap.hxx"

FILEMAP::FILEMAP(const PIDBOBJ DbParent)
{
  INT i;
  Parent=DbParent;
  MDTREC Mdtrec;

  mdt=Parent->GetMainMdt();
  MdtCount=mdt->GetTotalEntries();
  Items= new struct _table[MdtCount];

  for(i=1; i<=MdtCount; i++){
    mdt->GetEntry(i,&Mdtrec);
    Mdtrec.GetFullFileName(&Items[i-1].Path);
    Items[i-1].GpStart=Mdtrec.GetGlobalFileStart();
    Items[i-1].LocalStart=Mdtrec.GetLocalRecordStart();
    Items[i-1].LocalEnd=Mdtrec.GetLocalRecordEnd();
  }
}
static int TableCompareKeys(const void* GpPtr, const void* GpRecPtr) 
{
  GPTYPE LocalValue,Start,End;

  LocalValue=((struct _table*)GpPtr)->GpStart;
  Start= ((struct _table*)GpRecPtr)->GpStart+
    ((struct _table*)GpRecPtr)->LocalStart;

  End= ((struct _table*)GpRecPtr)->GpStart +
    ((struct _table*)GpRecPtr)->LocalEnd;

  if((LocalValue >= Start) && (LocalValue <= End)){

    return 0;
  } else {
    if (LocalValue<Start ) {
      return -1;
    } else {
      return 1;
    }
  }
}
 
GPTYPE FILEMAP::GetKeyByGlobal(GPTYPE gp)
{
  GPTYPE Start;
  struct _table *t,key;
  key.GpStart=gp;
#ifndef __SUNPRO_CC
  t= (struct _table *)bsearch(&key, Items, MdtCount,
			       sizeof(_table), TableCompareKeys);
#else
  t= (struct _table *)bsearch((char*)&key, (char*)Items, MdtCount,
			       sizeof(_table), TableCompareKeys);
#endif

  if(t){
    Start=t->GpStart+t->LocalStart;
  }else{
    Start=0;
    // BUGFIX #2: was "%d" for gp, a GPTYPE (unsigned int) -- the same
    // signed/unsigned printf mismatch already cataloged for
    // src/fc.cxx/src/fct.cxx's Write() functions. A large gp would print
    // as negative.
    printf("Lookup failed for %u\n", gp);
  }

  return(Start);
}

GPTYPE FILEMAP::GetNameByGlobal(GPTYPE gp, PSTRING s, INT *size, INT *LS)
{
  GPTYPE Start;
  struct _table *t,key;
  key.GpStart=gp;
#ifndef __SUNPRO_CC
  t= (struct _table *)bsearch(&key, Items, MdtCount,
			       sizeof(_table), TableCompareKeys);
#else
  t= (struct _table *)bsearch((char*)&key, (char*)Items, MdtCount,
			       sizeof(_table), TableCompareKeys);
#endif
  if(t){
    *size=t->LocalEnd-t->LocalStart+1;
    *LS=t->LocalStart;
    Start=t->GpStart+t->LocalStart;
    *s=t->Path;

  }else{
    Start=0;
    // BUGFIX #2: see GetKeyByGlobal() above.
    printf("Lookup failed for %u\n", gp);
  }

  return(Start);
}
  

FILEMAP::~FILEMAP()
{
  delete [] Items;
}

