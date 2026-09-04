/* $Id: mergeunit.cxx,v 1.12 1998/05/12 16:49:12 cnidr Exp $ */
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

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		mergeunit.cxx
Version:	1.00
$Revision: 1.12 $
Description:	Class MERGEUNIT
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

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
#include "mergeunit.hxx"

/// Constructs an empty unit; Initialize() must be called before use.
MERGEUNIT::MERGEUNIT()
{
  sistring="";
  CachePosition=LoadLim=LIM;

  list = new GPTYPE[1];
  Start = new INT[1];
  sistrings = new STRING[1];
  Tag = new CHR[1];
  CacheWritten=FlushWritten=CacheFlush=0;
  ItemsToMerge=TotalLoaded=0;
  // BUGFIX #3 (docs/BUG_CATALOG.md#srcmergeunithxx): Parent/fp/Map/ID/Gp
  // were left indeterminate here; ~MERGEUNIT() already dereferences
  // Parent (via `Parent->ffclose(fp)`) guarded only by `if(fp)`, so a
  // MERGEUNIT destroyed without Initialize() having been called first
  // would read fp/Parent as indeterminate values. Matches the same
  // "indeterminate member" category fixed in nlist.cxx's BUGFIX #2.
  Parent = nullptr;
  fp = nullptr;
  Map = nullptr;
  ID = 0;
  Gp = 0;
}

static void LocalGetSistring(STRING *s, INT Gp, CHR *buf)
{
  CHR *p,Buffer[StringCompLength+1];
  CHR c='\0';
  INT z,len;

  p=&buf[Gp];
  len=strlen(p);
  if(len>=StringCompLength){
    c=p[StringCompLength];
    p[StringCompLength]='\0';
  }
  strcpy(Buffer,p);
  if(len>=StringCompLength)
    p[StringCompLength]=c;
  for (z = 0; Buffer[z]; z++){
    Buffer[z]=tolower(Buffer[z]);
    //    if (!isalnum(Buffer[z]))
    if (!IsAlnum(Buffer[z]))
      Buffer[z] = ' ';
  }
  *s=Buffer;

}

/// Refills the cache (`list`/`Start`/`sistrings`/`Tag`) from `fp`, up to
/// LoadLim entries, then resolves each entry's sort-comparison string.
GDT_BOOLEAN MERGEUNIT::CacheLoad()
{

  INT i,j,k,found=0;
  GPTYPE fe;
  

  STRING Fn;
  int namecount=0;
  FILE *fq;

  CachePosition=0;
#ifdef VERBOSE
  printf("===== Load Unit %i =====\n", ID);
#endif
 
  if(feof(fp)){
    LoadLim=0;
    return(GDT_FALSE);
  }
  for(i=0; i<LoadLim; i++){
#ifndef __SUNPRO_CC
    found+=fread((void*)&fe,sizeof(GPTYPE),1,fp); // explicit cast
#else
    found+=fread((char*)&fe,sizeof(GPTYPE),1,fp); // explicit cast
#endif
    list[i]=fe;
  }
  TotalLoaded+=found;
  if(found==0){
    LoadLim=0;
    return(GDT_FALSE);
  }
  if(found<LoadLim)
    LoadLim=found;
#ifdef VERBOSE
  printf("Build Starting Keys..(got %i)\n", LoadLim);
#endif
  for(i=0; i<LoadLim; i++){
    Tag[i]=0;
    Start[i]=Map->GetKeyByGlobal(list[i]);
  }
#ifdef VERBOSE
  printf("Key Build Complete - Commence Merge Load...\n");
  printf("(%i of %i) Complete\n",
	 TotalLoaded - found, ItemsToMerge);
#endif

  INT size,offset,gp,tmpgp;
  CHR *p;
  STRING x;
  INT q=LoadLim,count=0,ncount=0,LocalStart;
  struct stat sb;

  for(i=0; i<LoadLim; i++){
    if(Tag[i]==0){
      // gp is Global Position of start of record (*not* file)
      gp=Map->GetNameByGlobal(list[i],&Fn,&size,&LocalStart);
    
      p=new CHR[size+1];
      fq=Parent->ffopen(Fn,"rb");
      //      fseek(fq,LocalStart,0);
      fseek(fq, (long)LocalStart, SEEK_SET);
      size=fread(p,1,size, fq);
      p[size]='\0';
      fclose(fq);
      LocalGetSistring(&sistrings[i],list[i]-gp,p);
      Tag[i]=1;
      count++;
      for(j=i; j<LoadLim; j++){ // inside loop - find all later entries 
	// from same record
	if(Tag[j]==0){ // not visited already
	  tmpgp=Start[j];
	  if(tmpgp==gp){		// same record
	    LocalGetSistring(&sistrings[j],list[j]-gp,p);
	    Tag[j]=1;
	    count++;
	    ncount++;
	  }
	}
      }
      if(ncount>(LoadLim/5)){
#ifdef VERBOSE
	printf("Finished %i of %i\n", count, q);
#endif
	ncount=0;
      }
      // BUGFIX #2 (docs/BUG_CATALOG.md#srcmergeunithxx): `p` is
      // allocated via `new CHR[size+1]` above but was freed with scalar
      // `delete` -- undefined behavior, confirmed under ASan as a real
      // alloc-dealloc-mismatch.
      delete [] p;
    }
  }
  return(GDT_TRUE);
}

// flush entire unit to file
GDT_BOOLEAN MERGEUNIT::Flush(FILE *fout)
{ 
  INT i;
#ifdef VERBOSE
  printf("Flush..\n");
#endif
//  Parent->GpFwrite(&Gp,1,sizeof(GPTYPE),fout);
//  ++FlushWritten;
  // flush cache here
  for(i=CachePosition; i<LoadLim; i++){
    Parent->GpFwrite(&list[i],1,sizeof(GPTYPE),fout);
    ++CacheFlush;
  }
  while(Parent->GpFread(&Gp,1,sizeof(GPTYPE),fp)){
    //  cout << Gp <<endl;
    ++FlushWritten;
    Parent->GpFwrite(&Gp,1,sizeof(GPTYPE),fout);
  }
  CachePosition=LoadLim;
  return GDT_TRUE;
}

// write item in Gp to disk
void MERGEUNIT::Write(FILE *fout)
{
  // cout <<"|"<<sistring<<"|"<<endl;
  ++CacheWritten;
  Parent->GpFwrite(&list[CachePosition++],1,sizeof(GPTYPE),fout);
  Load();
  
}


GDT_BOOLEAN MERGEUNIT::Smallest(STRING *Current)
{
  CHR a[256],b[256];
  Current->GetCString(a,256);
  sistrings[CachePosition].GetCString(b,256);
  if(strcmp(a,b)<=0)
    return(GDT_TRUE); 
  *Current=b;
  return(GDT_FALSE);
}


// signify whether entire unit is empty

GDT_BOOLEAN MERGEUNIT::Empty()
{

  if(feof(fp)&&(CacheEmpty()==GDT_TRUE)){
    return(GDT_TRUE);
  }
  else
    return(GDT_FALSE);

}

GPTYPE MERGEUNIT::GetGp()
{
  return(list[CachePosition]);
}

void MERGEUNIT::GetSistring(STRING *a)
{
  *a=sistrings[CachePosition];
}

GDT_BOOLEAN MERGEUNIT::CacheEmpty()
{
  // note - we have a good value in Gp!!
  if(CachePosition==LoadLim)
    return(GDT_TRUE);
  else
    return(GDT_FALSE);
}

void MERGEUNIT::SetLoadLimit(INT v)
{
  CachePosition=LoadLim=v;

  delete [] list;
  delete [] Start;
  delete [] sistrings;
  delete [] Tag;
  
  list = new GPTYPE[LoadLim+1];
  Start = new INT[LoadLim+1];
  sistrings = new STRING[LoadLim+1];
  Tag = new CHR[LoadLim+1];
}


// get top item in cache and put in Gp/sistring
// reload cache if necessary
// returns TRUE if cache is empty (nothing left)

GDT_BOOLEAN MERGEUNIT::Load()
{
  GDT_BOOLEAN val=GDT_TRUE;
  
  if( CacheEmpty()==GDT_TRUE)
    val=CacheLoad();
  if (val==GDT_FALSE)
    return GDT_TRUE;
 // Gp=list[CachePosition];
//  sistring=sistrings[CachePosition++];
  return(GDT_FALSE);
  
}


GDT_BOOLEAN MERGEUNIT::Initialize(STRING& FileName,const PIDBOBJ DbParent, FILEMAP *m, INT value)
{
  CHR Tmp[256];
  GDT_BOOLEAN val;
  struct stat sb;
  
  FileName.GetCString(Tmp,256);
  stat(Tmp,&sb);
  ItemsToMerge=sb.st_size/sizeof(GPTYPE);
  Parent = DbParent;
  ID=value;
  Map=m;
  fp=Parent->ffopen(FileName,"rb");
  if(!fp)
    return GDT_FALSE;
  val=Load();
  // Gp=list[CachePosition];
  //  sistring=sistrings[CachePosition++];
  return(val);
  
}



/// Closes the source file (if Initialize() was called) and frees the
/// four owned arrays.
MERGEUNIT::~MERGEUNIT()
{

  if(fp)
    Parent->ffclose(fp);
  //  delete names;
  // BUGFIX #2 (docs/BUG_CATALOG.md#srcmergeunithxx): `list`, `Tag`, and
  // `Start` are each allocated with array `new` (see the constructor
  // and SetLoadLimit()) but were freed here with scalar `delete` --
  // undefined behavior, confirmed under ASan as a real
  // alloc-dealloc-mismatch, firing on the most basic construct-then-
  // destroy usage of this class (e.g. `MERGEUNIT A[2];`). Only
  // `sistrings` was already correct.
  delete [] list;
  delete [] sistrings;
  delete [] Tag;
  delete [] Start;
#ifdef VERBOSE
  printf("=== Unit %i ===\n", ID);
  printf("Written From Cache: %i\n", CacheWritten);
  printf("Remaining Items Flushed From Cache: %i\n", CacheFlush);
  printf("Remaining Items Flushed From Disk: %i\n", FlushWritten);
#endif  
}
