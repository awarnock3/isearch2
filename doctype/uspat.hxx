// $Id: uspat.hxx,v 1.2 1998/05/12 16:48:38 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1995.

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
File:		uspat.hxx
Version:	1.00
$Revision: 1.2 $
Description:	Class USPAT - Greenbook-style US patent Text
Author:		Jim Fullton - Jim.Fullton@cnidr.org
@@@*/

#ifndef USPAT_HXX
#define USPAT_HXX

#include "defs.hxx"
#include "isearch.hxx"
#include "doctype.hxx"
#include "reclist.hxx"

/* add miscellaneous PATO defs here.  clean up later */


#define PATO_MAXSTR 256
#define RS_SUTRS "1.2.840.10003.5.101"
#define RS_HTML "1.2.840.10003.5.1000.34.1"


typedef struct tagPATO_FIELD {
	char ID[6];
	char Data[PATO_MAXSTR];
	struct tagPATO_FIELD* Next;
} PATO_FIELD;

typedef struct tagPATO_GROUP {
	char ID[6];
	PATO_FIELD* Fields;
	struct tagPATO_GROUP* Next;
} PATO_GROUP;

typedef struct tagPATO_PATENT {
	PATO_GROUP* Groups;
} PATO_PATENT;

typedef struct tagPATO_BUFFER {
	int Index;
	char* Data;
	int Size;
} PATO_BUFFER;

typedef struct tagPATO_OUTPUT {
	int Type; /* 0:stdout, 1:file, 2:buffer */
	FILE* fp;
	PATO_BUFFER* Buffer;
} PATO_OUTPUT;


class USPAT : public DOCTYPE {
public:
	char pato_HostName[256];
	char pato_DBName[256];
	//char RecBuffer[2000000];
	PATO_PATENT *Patent;
	USPAT(IDBOBJ* DbParent);
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(RECORD* NewRecord)  ;
	void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
		const STRING &Rs, STRING* StringBuffer);
	
	~USPAT();
private:
	
	void pato_Cleanup(PCHR s) const;
	PCHR pato_item(PCHR S, INT X, PCHR Item, CHR Delim)const ;
	INT pato_itemCount(PCHR S, CHR Delim) const;
	PCHR RmTrailBlanks(PCHR S)const ;
	PATO_PATENT* pato_ReadPatent(PCHR Buffer, INT PatentSize)const ;
	PCHR pato_Get_TI(PATO_PATENT* Patent, PCHR S, INT SSize)const ;
	void CleanTextString(PSTRING StringBuffer)const ;
	PATO_GROUP* pato_FindGroup(PATO_PATENT* Patent, char* Group)const ;
	PATO_FIELD* pato_FindField(PATO_GROUP* Group, PCHR Field)const ;
	PCHR pato_Get_PN(PATO_PATENT* Patent, PCHR S, INT SSize)const ;
	void pato_DisposePatent(PATO_PATENT* Patent) const;
	void pato_OutputBuffer(PATO_OUTPUT* Output, PATO_BUFFER* Buffer)const ;
	void pato_InitBuffer(PATO_BUFFER* Buffer, INT Size) const;
	void pato_FreeBuffer(PATO_BUFFER* Buffer) const;
	PCHR pato_GetField(PATO_GROUP* Group, PCHR Field, PCHR S) const;
	//void InsertField(PDFT pdft,PCHR Field, INT Start, INT End) const;
	void AddCommas(PCHR S1, PCHR S2) const;
	void Semi_to_Comma(PCHR S1) const;
	void pato_Code(PCHR S, INT Code, INT Size, INT Flag) const;
	void pato_ElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PSTRING ElementSet, PCHR Format) const;
	void pato_TextElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR Element) const;
	void pato_HtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR Element) const;
  	void pato_TextHtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR Element) const;
	void PatentPath(PCHR fn, PCHR path)const ;
	void space(CHR* S, INT length, PATO_OUTPUT* Output) const;
	void pato_Write(PATO_OUTPUT* Output, const CHR* S) const;
	void pato_HtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR ElementSet)const ; 
   	void pato_TextHtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR ElementSet)const ; 
	void pato_TextElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PCHR ElementSet)const ;
	void pato_ClassFixup(PCHR S)const ;
	int pato_WriteBuffer(PATO_BUFFER* Buffer, const CHR* S)const ;
	INT pato_GoodClass(PCHR Sub)const ;
	void pato_html_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group)const ;
	void pato_text_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group)const ;
	void pato_html_FigLinks(PCHR Data, PCHR Buffer)const ;
	void pato_html_Special(PCHR Data, PCHR Buffer)const ;
	void pato_html_CloseFormat(PATO_OUTPUT* Output, PCHR Format)const ;
	void pato_ExpandDate(PCHR S, PCHR Buffer) const;
	void pato_html_Continue(PATO_OUTPUT* Output, PATO_FIELD* Field) const;
	PCHR pato_GetMonth(INT x, PCHR S) const;
	INT pato_PA(PCHR ID)const ;
	void pato_html_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
		INT MaxNumNames)const ;
	void pato_text_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart)const ;
	void pato_NameCount(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
			    INT MaxNumNames) const;
};

typedef USPAT* PUSPAT;


#include <string.h>
#include "string.hxx"
#include "strlist.hxx"
#include "dft.hxx"
#include "dfd.hxx"
#include "idbobj.hxx"
#include "gdt.h"

class GB {
	STRLIST c_field_name_list;
	INT4 	c_field_name_count,
		c_length;
	CHR 	*c_record;
	INT GetFieldName(const INT4 Start, STRING *Name, INT4 *NextFieldStart);
	void InsertField(PIDBOBJ Db, PDFT pdft, const STRING& Field, INT Start, 
		INT End) const;
public:
	GB(CHR *Record, const INT4 Length);
	~GB();

	PDFT BuildDft(PIDBOBJ Db, STRLIST& FieldNames, STRLIST& Hierarchies);
};

#endif

























