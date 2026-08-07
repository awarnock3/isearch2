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
File:		doctype.hxx
Version:	1.00
Description:	Class DOCTYPE - Document Type
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef DOCTYPE_HXX
#define DOCTYPE_HXX

#include "defs.hxx"
#include "idbobj.hxx"
#include "result.hxx"
#include "squery.hxx"
#include "rset.hxx"
#include "registry.hxx"

// Base class for every document-type parser (see doctype/*.cxx for the
// ~60 concrete formats: HTML, MARC, GILS, etc.). Db is a non-owning
// back-pointer to the parent IDBOBJ, set once at construction. Most
// methods here are deliberately no-op defaults (bodies inlined as
// `{}`), meant to be overridden per format; parameter names are kept
// on those even though unused, for self-documentation of the interface
// each override implements.
class DOCTYPE {
public:
  DOCTYPE(IDBOBJ* DbParent);
  virtual void   BeforeIndexing() {};
  virtual void   LoadFieldTable() {};
  virtual void   AddFieldDefs() {};
  virtual void   ParseRecords(const RECORD& FileRecord);
  //@ManMemo: This method called during indexing to build GP list for document.
  // Returns the number of GPs written to GpBuffer, or (GPTYPE)-1 if
  // GpBuffer (GpLength entries) filled up before the document did --
  // callers (see INDEX::BuildGpList, src/index.cxx) check for that
  // sentinel and flush/stop rather than treating it as a valid count.
  virtual GPTYPE ParseWords(CHR* DataBuffer, INT DataLength,
			    INT DataOffset, GPTYPE* GpBuffer,
			    INT GpLength);
  //	virtual void SelectRegions(const RECORD& Document, FCT* FctPtr);
  // Lowercases DataBuffer in place, turning non-alphanumeric bytes
  // into spaces. Writes a '\0' at data[length] to terminate the
  // result as a C string -- callers must size data with at least
  // length+1 bytes of capacity.
  virtual void   ReplaceWithSpace(PCHR data, INT length);
  virtual void   ParseFields(RECORD* NewRecordPtr) {};
  virtual void   ParseDate(const CHR *Buffer, DOUBLE* fStart, 
			   DOUBLE* fEnd) {};
  virtual void   ParseDate(const STRING& Buffer, DOUBLE* fStart, 
			   DOUBLE* fEnd) {};
  virtual void   ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
				DOUBLE* fEnd) {};
  virtual void   ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
				DOUBLE* fEnd) {};
  virtual void   ParseRange(const CHR *Buffer, DOUBLE* fStart, 
				DOUBLE* fEnd) {};
  virtual void   ParseRange(const STRING& Buffer, DOUBLE* fStart, 
				DOUBLE* fEnd) {};
  virtual DOUBLE ParseNumeric(const CHR *Buffer);
  virtual INT    ParseGPoly(const CHR *Buffer, DOUBLE Vertices[]) { return 0; }
  virtual DOUBLE ParseComputed(const STRING& FieldName, const CHR *Buffer) 
    {return 0.0;}
  //  virtual void GetMetadata(const RECORD& record, const STRING&
  //			   mdType, STRING* buffer);
  virtual REGISTRY* GetMetadata(const RECORD& record, const STRING&
			   mdType, const REGISTRY* defaults);
  virtual void   AfterIndexing() {};
  virtual void   BeforeSearching(SQUERY* SearchQueryPtr) {};
  virtual PIRSET AfterSearching(IRSET* ResultSetPtr) { return ResultSetPtr;};
  virtual void   BeforeRset(const STRING& RecordSyntax) {};
  virtual void   AfterRset(const STRING& RecordSyntax) {};
  virtual void   Present(const RESULT& ResultRecord, 
			 const STRING& ElementSet,
			 STRING* StringBufferPtr);
  virtual void   Present(const RESULT& ResultRecord, 
			 const STRING& ElementSet,
			 const STRING& RecordSyntax, 
			 STRING* StringBufferPtr);
  virtual void   Present(const RESULT& ResultRecord, 
			 const STRING& ElementSet,
			 const STRING& RecordSyntax,
			 GDT_BOOLEAN HighlightTerms,
			 STRING* StringBufferPtr);
  virtual GDT_BOOLEAN UsefulSearchField(const STRING& Field) 
                         { return GDT_TRUE;} ;
  virtual ~DOCTYPE();
	
protected:
  IDBOBJ* Db;
	
};

typedef DOCTYPE* PDOCTYPE;

#endif
