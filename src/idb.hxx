/* $Id: idb.hxx,v 1.18 2000/02/04 23:23:57 cnidr Exp $ */
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
File:		idb.hxx
Version:	1.01
$Revision: 1.18 $
Description:	Class IDB
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef IDB_HXX
#define IDB_HXX

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "isearch.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "rcache.hxx"
#include "dtreg.hxx"
#include "index.hxx"
#include "nlist.hxx"
#include "intlist.hxx"
#include "date.hxx"

#ifdef DICTIONARY
#include "dictionary.hxx"
#endif

class IDB 
  : public IDBOBJ {
    friend class INDEX;
    friend class IRSET;
    friend class DOCTYPE;
    friend class VIDB;
#ifdef DICTIONARY
    friend class DICTIONARY;
#endif

public:
    IDB(const STRING& NewPathName, const STRING& NewFileName);
    IDB(const STRING& NewPathName, const STRING& NewFileName, 
	const STRLIST& NewDocTypeOptions);
    void        SetMergeStatus(GDT_BOOLEAN x) { MainIndex->SetMergeStatus(x); }

    void        MergeIndexFiles(INT m) { MainIndex->MergeIndexFiles(m); }
    void        CollapseIndexFiles(INT m) { 
                    MainIndex->CollapseIndexFiles( m); }
    void        Initialize(const STRING& NewPathName, 
			   const STRING& NewFileName,
			   const STRLIST& NewDocTypeOptions);
    GDT_BOOLEAN IsDbCompatible() const;
    GDT_BOOLEAN IsDbVirtual() { return GDT_FALSE;}
    void        GetAllDocTypes(STRLIST *StringListBuffer) const {
                    DocTypeReg->GetDocTypeList(StringListBuffer); }
    void        GetDocTypeOptions(STRLIST *StringListBuffer) const {
                   *StringListBuffer = DocTypeOptions; }
    void        SetDocTypeOptions();
    void        KeyLookup(const STRING& Key, RESULT *ResultBuffer) const;
    void        GetDfdt(DFDT *DfdtBuffer) const { *DfdtBuffer = *MainDfdt; }
    void        GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer) const;
    //	void SetTitle(const STRING& NewTitle);
    //	void GetTitle(PSTRING StringBuffer);
    //	void SetComments(const STRING& NewComments);
    //	void GetComments(PSTRING StringBuffer);
    void        DfdtAddEntry(const DFD& NewDfd) { MainDfdt->AddEntry(NewDfd); }
    void        DfdtGetEntry(const INT Index, DFD *DfdRecord) const { 
                    MainDfdt->GetEntry(Index, DfdRecord); }
    void        FlushFiles();
#ifdef DICTIONARY
    void        CreateDictionary(void);
    void        CreateCentroid(void);
#endif
    INT         DfdtGetTotalEntries() const { 
                    return MainDfdt->GetTotalEntries(); };
    DOCTYPE    *GetDocTypePtr(const STRING& DocType) const;
    GDT_BOOLEAN ValidateDocType(const STRING& DocType) const;
    //	void AddRecordList(const RECLIST& NewRecordList);
    INT         GetTotalRecords() const { return MainMdt->GetTotalEntries(); }
    void        SetIndexingMemory(const UINT4 MemorySize);
    UINT4       GetIndexingMemory() const { return IndexingMemory; }
    IRSET      *AndSearch(const SQUERY& SearchQuery); // temporary!
    IRSET      *Search(const SQUERY& SearchQuery);
    void        DfdtGetFileName(const STRING& FieldName, 
				STRING *StringBuffer) const;
    //	void GetRecordData(const RESULT& ResultRecord, PSTRING StringBuffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     const STRING& FieldType,
			     STRING* StringBuffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     STRLIST* StrlistBuffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     STRING* StringBuffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     DOUBLE* Buffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     DATERANGE* Buffer) const;
    GDT_BOOLEAN GetFieldData(const RESULT& ResultRecord, 
			     const STRING& FieldName,
			     SRCH_DATE* Buffer) const;
    void        Present(const RESULT& ResultRecord, const STRING& ElementSet,
			const STRING& RecordSyntax,
			GDT_BOOLEAN HighlightTerms,
			STRING *StringBuffer) const;
	// deprecated
    void        Present(const RESULT& ResultRecord, const STRING& ElementSet,
			const STRING& RecordSyntax, 
			STRING *StringBuffer) const;
	// deprecated
    void        Present(const RESULT& ResultRecord, const STRING& ElementSet,
			STRING *StringBuffer) const;
    //	void GenerateKeys();
    void        DebugModeOn() { DebugMode = 1; }
    void        DebugModeOff() { DebugMode = 0; }
    void        SetDebugSkip(const INT Skip) { DebugSkip = Skip; }
    INT         IsSystemFile(const STRING& FileName);
    void        KillAll();
    void        ComposeDbFn(STRING *StringBuffer, const CHR *Suffix) const;
    void        GetDbFileStem(STRING *StringBuffer) const;
    void        GetDbVersionNumber(STRING *StringBuffer) const;
    void        GetIsearchVersionNumber(STRING *StringBuffer) const {
                     *StringBuffer = IsearchVersion; }
    void        AddRecord(const RECORD& NewRecord);
    void        DocTypeAddRecord(const RECORD& NewRecord);
    void        SetDbState(const INT4 DbState);
    INT4        GetDbState();
    void        Index();
  void          ReplaceWithSpace(RECORD *Record, PCHR data, INT length);
    void        ParseFields(RECORD *Record);
    //@ManMemo: This method called during indexing to build GP list for document.
    GPTYPE      ParseWords(const STRING& Doctype, CHR* DataBuffer, 
			   INT DataLength, INT DataOffset,
			   GPTYPE* GpBuffer, INT GpLength);
    INT         IsStopWord(CHR* WordStart, INT WordMaximum) const;
    //      void SelectRegions(const RECORD& Record, FCT* RegionsPtr) const;
    FILE       *ffopen(const STRING& FileName, const CHR *Type) {
                     return fopen(FileName, Type); }
    INT         ffclose(FILE *FilePointer) { return fclose(FilePointer); }
    GDT_BOOLEAN IsWrongEndian() const { return WrongEndian; }
    void        SetWrongEndian();
    void        SetDocumentInfo(const INT Index, const RECORD& Record);
    void        GetDocumentInfo(const INT Index, RECORD *RecordBuffer) const;
    SIZE_T      GpFwrite(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
			 FILE* Stream) const;
    SIZE_T      GpFread(GPTYPE* Ptr, SIZE_T Size, SIZE_T NumElements, 
			FILE* Stream) const;
    GDT_BOOLEAN GetDocumentDeleted(const INT Index) const;
    INT         DeleteByKey(const STRING& Key);
    INT         UndeleteByKey(const STRING& Key);
    SIZE_T      CleanupDb();
    void        SetGlobalDocType(const STRING& NewGlobalDocType);
    void        GetGlobalDocType(STRING *StringBuffer) const;
    void        BeginRsetPresent(const STRING& RecordSyntax);
    void        EndRsetPresent(const STRING& RecordSyntax);
    void        WriteCentroid(FILE* fp);

    MDT*        GetMainMdt() { return MainMdt; }
    DFDT*       GetMainDfdt() { return MainDfdt; }

    INDEX      *MainIndex;
    DTREG      *DocTypeReg;

    ~IDB();

protected:
    void IndexingStatus(const INT StatusMessage, const STRING *FileName,
			const INT Count) const {};
    //    INDEX      *MainIndex;
    //    DTREG      *DocTypeReg;
    STRLIST     DocTypeOptions;
    MDT        *MainMdt;

private:
    //    MDT*        GetMainMdt() { return MainMdt; }
    //    DFDT*       GetMainDfdt() { return MainDfdt; }

    STRING      DbPathName, DbFileName;
    STRING      Title, Comments;
    DFDT       *MainDfdt;
    UINT4       IndexingMemory;
    INT         DebugMode, DebugSkip;
    FPT         MainFpt;
    INT         TotalRecordsQueued;
    REGISTRY   *MainRegistry;
    GDT_BOOLEAN DbInfoChanged;
    GDT_BOOLEAN WrongEndian;
    //	STRLIST FieldTypes;
};

typedef IDB* PIDB;

void
MakeDbGilsRec(IDB *IdbPtr, STRING& PathName, STRING& FileName, STRING* buffer);

#endif
