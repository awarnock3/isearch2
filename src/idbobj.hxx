/* $Id: idbobj.hxx,v 1.11 2000/02/04 23:21:40 cnidr Exp $ */
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
File:		idbobj.hxx
Version:	1.00
$Revision: 1.11 $
Description:	Class IDBOBJ: Database object virtual class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef IDBOBJ_HXX
#define IDBOBJ_HXX

/*
#include "defs.hxx"
#include "string.hxx"
#include "mdt.hxx"
#include "dfdt.hxx"
#include "dfd.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "record.hxx"
#include "dtreg.hxx"
*/
#include "hash.hxx"

class IDBOBJ {
  friend class INDEX;
  friend class IRSET;
  friend class NUMERICFLDMGR;
  friend class MERGEUNIT;
  friend class FILEMAP;

public:
  IDBOBJ() { };
  virtual ~IDBOBJ() { };
  virtual UINT4       GetIndexingMemory() const { return 0; };
  virtual void        DfdtAddEntry(const DFD& NewDfd) = 0;
  virtual void        DfdtGetEntry(const INT Index, DFD *DfdRecord) const { };
  virtual INT         DfdtGetTotalEntries() const { return 0; };
  virtual void        DfdtGetFileName(const STRING& FieldName, 
				      STRING *StringBuffer) const { };
  virtual GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
				   const STRING& FieldName, 
				   const STRING& FieldType, 
				   STRING* StringBuffer) const 
    { return GDT_FALSE; };
  virtual GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
				   const STRING& FieldName,
				   STRING *StringBuffer) const
    { return GDT_FALSE; };
  virtual GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
				   const STRING& FieldName, 
				   STRLIST* StrlistBuffer) const 
    { return GDT_FALSE; };
  virtual GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
				   const STRING& FieldName, 
				   DOUBLE* Buffer) const 
    { return GDT_FALSE; };
  virtual void        GetDocTypeOptions(STRLIST *StringListBuffer) const { };
  //	virtual PDOCTYPE GetDocTypePtr(const STRING& DocType) const { };
  
  virtual void        ComposeDbFn(STRING *StringBuffer, 
				  const CHR *Suffix) const { };
  virtual void        DocTypeAddRecord(const RECORD& NewRecord) { };
  virtual FILE       *ffopen(const STRING& FileName, const CHR *Type) 
    { return 0; };
  virtual INT         ffclose(FILE *FilePointer) { return 0; };
  virtual SIZE_T      GpFwrite(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
			       FILE* Stream) const {
    fprintf(stderr, "Bad call to IDBOBJ::GpFwrite()\n");
    fclose(stdout); fclose(stderr); return 0;
  };
  virtual SIZE_T      GpFread(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
			      FILE* Stream) const {
    fprintf(stderr, "Bad call to IDBOBJ::GpFread()\n");
    fclose(stdout); fclose(stderr); return 0;
  };
  //	void GetDbFileStem(PSTRING StringBuffer) const { };
  virtual void        GetDbFileStem(STRING *StringBuffer) const { };
  virtual INT         IsStopWord(CHR* WordStart, INT WordMaximum) const = 0;

  //STRLIST FieldTypes;
  HASH FieldTypes;
  HASH FileNames;
  
protected:
  virtual void        IndexingStatus(const INT StatusMessage,
				     const STRING *FileName, 
				     const INT WordCount) const { };
private:
  virtual MDT        *GetMainMdt() { return 0; };
  virtual DFDT       *GetMainDfdt() { return 0; };
  virtual void        ReplaceWithSpace(RECORD *Record, PCHR data, 
				       INT length){};
  virtual void        ParseFields(RECORD *Record) { };
  virtual GPTYPE      ParseWords(const STRING& Doctype, CHR* DataBuffer, 
				 INT DataLength, INT DataOffset, 
				 GPTYPE* GpBuffer, INT GpLength) = 0;
  //	virtual void SelectRegions(const RECORD& Record, FCT* RegionsPtr) const { };
};

typedef IDBOBJ* PIDBOBJ;

#endif
