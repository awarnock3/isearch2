/* $Id: index.hxx,v 1.20 1999/04/22 18:19:53 cnidr Exp $ */
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
File:		index.hxx
Version:	1.01
$Revision: 1.20 $
Description:	Class INDEX
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef INDEX_HXX
#define INDEX_HXX

#include "defs.hxx"
#include "idbobj.hxx"
#include "string.hxx"
#include "mdt.hxx"
#include "squery.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "rcache.hxx"
#include "intlist.hxx"
#include "date.hxx"
#include "thesaurus.hxx"

// Not self-contained without this: DocTypePtr below is only ever used
// as an opaque pointer here (the full definition comes from
// doctype/doctype.hxx via dtreg.hxx, included by every real translation
// unit before this header). Forward-declaring it is not a signature
// change -- just what lets this header compile standalone, e.g. for
// tests/src/test_index.cxx, without pulling in the whole doctype/ tree.
class DOCTYPE;

// Owns and searches one physical on-disk inverted index for an IDB: the
// global-position (GPTYPE) postings files built during indexing, plus
// the per-field, numeric, and date search paths that read them back.
// Only ever heap-allocated and held by pointer (see IDB::MainIndex in
// idb.cxx) -- never copied or assigned -- so the raw-pointer members
// below (SetCache, DocTypePtr, TheThesaurus, Dict) don't need copy/move
// control; they're initialized in the constructor to keep them safe
// regardless.
class INDEX {
  friend class IDB;
#ifdef DICTIONARY
  friend class DICTIONARY;
#endif

public:
  INDEX(const PIDBOBJ DbParent, const STRING& NewFileName);
//int MemIndexCompare(PGPTYPE x, PGPTYPE y);
//void AddRecord(const RECORD& NewRecord);
  void        SortNumericFieldData();
  void        WriteFieldData(const RECORD& Record, const GPTYPE GpOffset);
#ifdef DICTIONARY
  void        CreateDictionary(void);
  void        CreateCentroid(void);
#endif
  void        AddRecordList(PFILE RecordListFp);
  GDT_BOOLEAN ValidateInField(const GPTYPE HitGp, FILE *fp, INT Entries,
			      INT Disk, GPTYPE *Cache, INT CacheSize, 
			      INT CacheBase );
  GDT_BOOLEAN DiskValidateInField(const GPTYPE HitGp, FILE *Fp, INT Total);
  PIRSET      RsetOr(const OPOBJ& Set1, const OPOBJ& Set2) const;
  PIRSET      Search(const SQUERY& SearchQuery);
  PIRSET      AndSearch(const SQUERY& SearchQuery);
  //  PRSET AndSearch(const SQUERY& SearchQuery);
  //  PRSET OrSearch(const SQUERY& SearchQuery);
  PIRSET      SoundexSearch(const STRING& SearchTerm, const STRING& FieldName);

  // Date Searching Routines
  PIRSET DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
		      INT4 Relation, INT4 Structure);
  PIRSET DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
		      INT4 Relation, INT4 Structure, GDT_BOOLEAN Strict);
  PIRSET DateRangeSearch(const STRING& QueryTerm, const STRING& FieldName, 
			 INT4 Relation, GDT_BOOLEAN Strict);
  PIRSET SingleDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
			  INT4 Relation, GDT_BOOLEAN Strict);
  PIRSET SingleDateSearchBefore(const SRCH_DATE& QueryDate, 
				const STRING& FieldName, 
				IntBlock FindBlock,
				GDT_BOOLEAN EndpointFlag);
  PIRSET YSearchBefore(const SRCH_DATE& DateY, const STRING& FieldName,
		       IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET YMSearchBefore(const SRCH_DATE& DateYM, const STRING& FieldName,
			IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET YMDSearchBefore(const SRCH_DATE& QueryDate, const STRING& FieldName,
			 IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET SingleDateSearchAfter(const SRCH_DATE& QueryDate, 
			       const STRING& FieldName, 
			       IntBlock FindBlock,
			       GDT_BOOLEAN EndpointFlag);
  PIRSET YMDSearchAfter(const SRCH_DATE& DateYMD, const STRING& FieldName,
			IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET YMSearchAfter(const SRCH_DATE& DateYM, const STRING& FieldName,
		       IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET YSearchAfter(const SRCH_DATE& DateY, const STRING& FieldName,
		      IntBlock FindBlock, GDT_BOOLEAN EndpointFlag);
  PIRSET DateRangeSearchContains(const DATERANGE& QueryDate, 
				 const STRING& FieldName,
				 IntBlock FindBlock,
				 GDT_BOOLEAN EndpointFlag);
  // This searches indexes of intervals
  PIRSET DateSearch(const DOUBLE fKey, const STRING& FieldName, 
			INT4 Relation, IntBlock FindBlock);


  // Text Searching
  PIRSET TermSearch(const STRING& SearchTerm, const STRING& FieldName);
  PIRSET TermSearch(const STRING& SearchTerm, const STRING& FieldName,
		    INT4 Relation);
  PIRSET MultiTermSearch(const STRING& SearchTerm, 
			 const STRING& FieldName, INT4 Relation);
  PIRSET TermSearch(DOUBLE QueryTerm, const STRING& FieldName);
  PIRSET TermSearch(DOUBLE QueryTerm, const STRING& FieldName, 
		    INT4 Relation);
  INT    Match(const CHR *QueryTerm, const INT TermLength, const GPTYPE gp, 
	       const INT4 Offset=0);

  // Numeric Searching
  // This searches indexes of single numeric values
  PIRSET NumericSearch(const DOUBLE fKey, const STRING& FieldName, 
		       INT4 Relation);
  // This searches indexes of intervals
  /*  PIRSET NumericSearch(const DOUBLE fKey, const STRING& FieldName, 
      INT4 Relation, IntBlock FindBlock);
  */
  // These two support geo-spatial searching
  PIRSET Interval(DOUBLE WestLongitude, DOUBLE EastLongitude,
		  DOUBLE SouthLatitude, DOUBLE NorthLatitude);
  PIRSET BoundingRectangle(DOUBLE NorthBC,
			   DOUBLE SouthBC,
			   DOUBLE WestBC,
			   DOUBLE EastBC);

  void   SetMergeStatus(GDT_BOOLEAN a) { MergeStatus=a; }
  void   DumpIndex(INT DebugSkip);
  void   WriteCentroid(FILE* fp);
  INT    IsStopWord(CHR* WordStart, INT WordMaximum) const;
  void   SetDocTypePtr(DOCTYPE* NewDocTypePtr) { 
                          DocTypePtr = NewDocTypePtr; }
  DOCTYPE *GetDocTypePtr() { return DocTypePtr; }
  ~INDEX();

private:
  GDT_BOOLEAN MergeStatus;
  GDT_BOOLEAN GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer);
  GDT_BOOLEAN GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer, 
				const INT len);
  INT GetIndirectBuffer(const GPTYPE Gp, CHR *Buffer, 
			const INT len, const INT BufferLen);
  //  INT IsStopWord(PCHR WordStart, INT WordMaximum);
  //  GPTYPE BuildGpList(INT StartingPosition, PCHR MemoryData,
  //		     INT MemoryDataLength, PGPTYPE MemoryIndex,
  //		     INT MemoryIndexLength);
  //@ManMemo: Calls IDB::ParseWords()
  GPTYPE  BuildGpList(const STRING& Doctype, INT StartingPosition,
		      CHR *MemoryData, INT MemoryDataLength, 
		      GPTYPE *MemoryIndex, INT MemoryIndexLength);
  //  void MergeIndex(PCHR MemoryData, INT MemoryDataLength,
  void    FlushIndexFiles(CHR *MemoryData, INT MemoryDataLength,
			  GPTYPE *MemoryIndex, INT MemoryIndexLength,
			  GPTYPE GlobalStart);
  void    MergeIndexFiles(INT MemMB);
  void    CollapseIndexFiles(INT MemMB);
  PFILE   GetFilePointer(const GPTYPE gp) const;
  STRING  IndexFileName;
  PIDBOBJ Parent;
  INT     IndexNum; // count of indexes to merge
//  UINT4 DataMemorySize, IndexMemorySize;
//  PGPTYPE MemoryIndex;
//  PMDT MemoryMdt;
#ifdef DICTIONARY
  DICTIONARY *Dict;
#endif
  RCACHE     *SetCache;
  INT         Accesses, InCache, OutCache;
  DOCTYPE    *DocTypePtr;
  THESAURUS  *TheThesaurus;
};

typedef INDEX* PINDEX;

#endif
