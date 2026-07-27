/* $Id: multiterm.cxx,v 1.14 1999/03/26 00:27:22 cnidr Exp $ */
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
File:		multiterm.cxx
Version:	1.01
$Revision: 1.14 $
Description:	Class INDEX
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

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
#include "mergeunit.hxx"
#include "filemap.hxx"
#ifdef DICTIONARY
#include "dictionary.hxx"
#endif
#define CACHELIMIT 50000	// 50,000 entries, at 8 bytes each

static int gpcomp(const void* x, const void* y) {
  return(*((GPTYPE *)x)-*((GPTYPE *)y));
}


PIRSET INDEX::MultiTermSearch(const STRING& QueryTerm, 
	const STRING& FieldName, INT4 Relation) 
{				// binary search
  
  
  STRING FieldType, CheckName;
  INT w,kk;
  FILE *fx=(FILE*)NULL;
  CHR buf[256];
  STRING TmpIndexFileName;
  GPTYPE gp;
  INT ip, oip, maxip, low, high;
  INT x, z, TermLength, OrigTermLength;
  INT first, last;
  INT match, nomatch;
  CHR OrigTerm[StringCompLength+1], *Term;
  INT done;
  PIRSET pirset = new IRSET(Parent);
  
  // Make the name of the file holding the number of chunks
  Parent->ComposeDbFn(&CheckName, ".num");

  // Open the file and find out how many pieces we have to search
  // Note - this should always succeed because TermSearch calls this
  // routine when it find that the *.num file exists
  fx=Parent->ffopen(CheckName,"r");
  if (!fgets(buf,256,fx)) {
    fclose(fx);
    delete pirset;
    return NULL;
  }
  fclose(fx);
  IndexNum=atoi(buf);

  // Check to see if we are searching a text field.  If not, bail out
  // to the numeric searching code (which does `not need the *.inx* files)
  Parent->FieldTypes.GetValue(FieldName,&FieldType);

  if (FieldType.GetLength() == 0)
    FieldType = "TEXT";

  if (FieldType!="TEXT"){
    DOUBLE fKey;
    CHR TmpBuf[256];
    QueryTerm.GetCString(TmpBuf,256);
    fKey=atof(TmpBuf);
    return(NumericSearch(fKey,FieldName,Relation));
  }

  // Search over each index chunk
  for(kk=1; kk<=IndexNum; kk++){

    //    cout << "Searching in chunk " << kk << endl;

    // Replace the *.inx file name with the name of the chunk
    TmpIndexFileName=IndexFileName;
    if (IndexNum > 1) {
      sprintf(buf,".%d",kk);
      TmpIndexFileName.Cat(buf);
    }

    // Open the index file
    PFILE fpi = Parent->ffopen(TmpIndexFileName, "rb");
    if (!fpi) {
      perror(TmpIndexFileName);
      EXIT_ERROR;
    }

    // Set up the searching variables
    done = 0;
    fseek(fpi, 0L, SEEK_END);
    maxip = (ftell(fpi) / sizeof(GPTYPE)) - 1;
    high = maxip;
    ip = high / 2;
    low = 0;
    GDT_BOOLEAN hit;
    z = 0;
    
    QueryTerm.GetCString(OrigTerm, sizeof(OrigTerm));
    //    OrigTermLength = QueryTerm.GetLength();
    // Fixed overflow of string length.  Ultimately, this has to be
    // modified to handle strings of arbitrary length, rather than the
    // sistring length.
    OrigTermLength = strlen(OrigTerm);
    
    // Because of sorting unpleasantness, we must convert non alnums 
    // in phrases to spaces.  For phrase searches, we need to look past 
    // all stop words, and start with the first indexed word. 
    // Later we check backwards in the data.
    INT PhraseEnd = OrigTermLength;
    INT n, PhraseBeg = 0, FoundBeg=0;

    if (OrigTerm[OrigTermLength - 1] == '*')
      PhraseEnd--;

    for (n=0; n < PhraseEnd; n++) {
      if (!IsAlnum(OrigTerm[n])) {
	OrigTerm[n] = ' ';
	if (!FoundBeg && IsStopWord(OrigTerm+PhraseBeg, n - PhraseBeg)) 
	  PhraseBeg = n + 1;
	else
	  FoundBeg = 1;
      }
    }
    
    if (PhraseBeg >= OrigTermLength) {
      // only stop words. return an empty IRSET.
      PIRSET pirset = new IRSET(Parent);
      return pirset;
    }

    Term = OrigTerm + PhraseBeg; 
    TermLength = OrigTermLength - PhraseBeg;
    
    // Now, do the binary search on the pointers
    do {
      hit = GDT_FALSE;
      oip = ip;
      fseek(fpi, (long)(ip * sizeof(GPTYPE)), SEEK_SET);
      x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
      if (x) {
	z = Match(Term, TermLength, gp);
	if (z == 0) {
	  done = 1;
	  hit = GDT_TRUE;
	}
	else if (z < 0) {
	  high = ip-1;
	}
	else if (z > 0) {
	  low = ip + 1;
	}
	ip = (low + high) / 2;
	if (ip < 0) {
	  ip = 0;
	}
	if (ip > maxip) {
	  ip = maxip;
	}
      } else {
	ip = 0;
	done = 1;
      }
    } while ( (!done) && (high >= low) );
    
    
    if (!hit && kk==IndexNum) {
      // no hits - return an empty irset
      return pirset;
    }
    
    // bracket hits
    if (hit) {
    
      // find first matching sistring
      low = 0;
      high = ip;
      first = high / 2;
      match = ip;
      nomatch = 0;
      do {
	fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
	x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
	if (x) 
	  z = Match(Term, TermLength, gp);
	if (z == 0) {
	  match = first;
	  high = first;
	} else {
	  nomatch = first;
	  low = first + 1;
	}
	first = (low + high) / 2;
	if (first < 0) {
	  first = 0;
	} else {
	  if (first > ip) {
	    first = ip;
	  }
	}
      } while ( (match - nomatch) > 5 );
      first = match;
      do {
	if (first > 0) {
	  first--;
	}
	fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
	x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
	if (x) 
	  z = Match(Term, TermLength, gp);
      } while ( (z == 0) && (first > 0) );

      if ( (z != 0) || (first > 0) ) {
	first++;
      }
    
    
      // find last matching sistring
      low = ip;
      high = maxip;
      last = (high + low) / 2;
      match = ip;
      nomatch = maxip;

      do {
	fseek(fpi, (long)(last * sizeof(GPTYPE)), SEEK_SET);
	x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
	if (x) 
	  z = Match(Term, TermLength, gp);
	if (z == 0) {
	  match = last;
	  low = last + 1;
	} else {
	  nomatch = last;
	  high = last;
	}
	last = (low + high) / 2;
	if (last < ip) {
	  last = ip;
	} else {
	  if (last > maxip) {
	    last = maxip;
	  }
	}
      } while ( (nomatch - match) > 5 );

      last = match;

      do {
	if (last < maxip) {
	  last++;
	}

	fseek(fpi, (long)(last * sizeof(GPTYPE)), SEEK_SET);
	x = Parent->GpFread(&gp, 1, sizeof(GPTYPE), fpi);
	if (x) 
	  z = Match(Term, TermLength, gp);
      } while ( (z == 0) && (last < maxip) );

      if ( (z != 0) || (last < maxip) ) {
	last--;
      }
    
      // Build result set
      IRESULT iresult;
      MDT*    ThisMdt;  
      MDTREC  mdtrec;
      GPTYPE  GlobalRecEnd;
      PGPTYPE gplist = new GPTYPE[last-first+1];
      INT OK;
      PFCT Pfct;
      FC Fc;

      fseek(fpi, (long)(first * sizeof(GPTYPE)), SEEK_SET);
      x = Parent->GpFread(gplist, 1, (last-first+1) * sizeof(GPTYPE), 
			  fpi) / sizeof(GPTYPE);
      fclose(fpi);
    
      // sort gplist
      qsort(gplist, (last-first)+1,sizeof(GPTYPE),gpcomp);

#ifdef VERBOSE
      printf("Got %i entries in pass %i\n", x, kk);
#endif

      INT Offset = TermLength - OrigTermLength;
      INT TermLenNoStar;
    
      if (QueryTerm.GetChr(OrigTermLength) == '*') {
	TermLenNoStar = OrigTermLength - 1; // ignore "*" at end
      } else {
	TermLenNoStar = OrigTermLength;
      }
    
      Pfct = new FCT();
      INT CheckField;
      INT Total=0;
      INT Disk=0;
      INT CacheSize=0;
      GPTYPE *Cache=(GPTYPE*)NULL;
      FILE *fpf=(FILE*)NULL;
    
      if (FieldName.Equals("") || FieldName.GetLength()==0) {
	CheckField=0;
	Total=0;
      } else {
	STRING Fn;
	CheckField=1;
	Parent->DfdtGetFileName(FieldName, &Fn);
	InCache=OutCache=Accesses=0;

	fpf = Parent->ffopen(Fn, "rb");
	if (fpf) {
	  fseek(fpf, 0L, SEEK_END);
	  Total = ftell(fpf) / ( sizeof(GPTYPE) * 2 );
	  rewind(fpf);
	  CacheSize=CACHELIMIT;
	  Cache=new GPTYPE[CacheSize*2];
	  Parent->GpFread(Cache,sizeof(GPTYPE),CacheSize*2,fpf);
	  Disk=0;
	} else {
	  // field file not found - return an empty irset
	  cerr << "Field " << FieldName << " not present in this index." 
	       << endl;
	  return pirset;
	}
      }
    
      for (ip=0; ip<x; ip++) {
	OK = 0;
	
	// Here is new code with rcache from Jim
	if (TermLength != OrigTermLength) {
	  if ( (Match(OrigTerm, OrigTermLength, gplist[ip], Offset)) == 0)  
	    gplist[ip] += Offset;
	  else
	    continue;
	}
	  
	if (CheckField==1) {
	  if (Disk==1) {
	    if (DiskValidateInField(gplist[ip], fpf, Total)) {
	      OK=1;
	    }
	  } else if (ValidateInField(gplist[ip], fpf, Total, Disk, 
				     Cache, CacheSize,0)) { 
	    OK = 1;
	  }
	} else
	  OK = 1;
	  
	if (OK) {
	  // make sure that phrases do not go past the end of the local record.
	  ThisMdt = Parent->GetMainMdt();
          w = Parent->GetMainMdt()->GetMdtRecord(gplist[ip], &mdtrec);
	  GlobalRecEnd = mdtrec.GetGlobalFileStart() + 
	    mdtrec.GetLocalRecordEnd();

	  if  ( !((GlobalRecEnd - gplist[ip]) >= (TermLenNoStar - 1)) )
	    continue;

	  // Skip deleted records
	  if (mdtrec.GetDeleted() == GDT_TRUE)
	    continue;
	  iresult.SetMdtIndex(w);
	  iresult.SetHitCount(1);
	  iresult.SetScore(0);
	  iresult.SetMdt(*ThisMdt);
	  Fc.SetFieldStart(gplist[ip]);
	  //      Fc.SetFieldEnd(gplist[ip] + QueryLength - 1);
	  Fc.SetFieldEnd(gplist[ip] + TermLength - 1);
	  Pfct->Clear();
	  Pfct->AddEntry(Fc);
#ifdef DO_HIGHLIGHTING  
	  iresult.SetHitTable(*Pfct);
#endif
	  pirset->FastAddEntry(iresult, 1);
	}
      }
      if(CheckField==1 && fpf!=NULL){
	//  printf("%d Accesses, %d InCache, %d OutCache (%f Efficiency)\n",
	//  Accesses,InCache,OutCache,(InCache/Accesses)*100);
	fclose(fpf);
      }
      delete Pfct;
      if(CacheSize>0)
	delete Cache;
      delete [] gplist;
      pirset->SortByIndex();
      pirset->MergeEntries(1);
    }
  }
  //  pirset->SortByIndex();
  //  pirset->MergeEntries(1);
  return pirset;
}
