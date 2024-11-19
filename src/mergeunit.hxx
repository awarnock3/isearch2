// $Id: mergeunit.hxx,v 1.5 1998/05/12 16:49:13 cnidr Exp $
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
File:		mergeunit.hxx
Version:	1.00
$Revision: 1.5 $
Description:	Class MERGEUNIT
Author:		Jim Fullton, MCNC/CNIDR
@@@*/



#ifndef MERGEUNIT_HXX
#define MERGEUNIT_HXX

#include "defs.hxx"
#include "filemap.hxx"
#include "idbobj.hxx"

#define LIM 10000

class MERGEUNIT{
	
 public:
  MERGEUNIT();
  GDT_BOOLEAN Smallest(STRING *small);
  GDT_BOOLEAN Initialize(STRING& FileName,const PIDBOBJ DbParent,
			 FILEMAP *map, INT IDValue); // true = success
  GDT_BOOLEAN Empty();				     // true = no more entries
  GPTYPE      GetGp(); // returns the GP
  void        GetSistring (STRING *a);
  GDT_BOOLEAN Flush(FILE *fout);
  GDT_BOOLEAN Load();		// get more items
  void        Write(FILE *fout);
  GDT_BOOLEAN CacheLoad();	// Load Cache From Disk
  void        SetLoadLimit(INT lim);
  
  ~MERGEUNIT();
  
private:
  GDT_BOOLEAN CacheEmpty();
  GDT_BOOLEAN GetIndirectBuffer(const GPTYPE Gp, STRING *Buffer); 
  PIDBOBJ     Parent;
  FILE       *fp;			// file of GP's to read from
  STRING      sistring;
  GPTYPE      Gp;
  FILEMAP    *Map;
  INT         CachePosition;
  INT         LoadLim,ID,CacheWritten,FlushWritten,CacheFlush;
  INT         ItemsToMerge,TotalLoaded;
  
  GPTYPE     *list;
  INT        *Start;
  
  STRING     *sistrings;
  // STRING *names;
  CHR        *Tag;
};



typedef MERGEUNIT *PMERGEUNIT;
#endif
