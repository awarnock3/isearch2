// $Id: doctype.cxx,v 1.13 2000/02/04 22:50:08 cnidr Exp $
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
File:		doctype.cxx
Version:	1.02
$Revision: 1.13 $
Description:	Class DOCTYPE - Document Type
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:  Archie Warnock (warnock@clark.net)
@@@*/

#include <string.h>
#include <ctype.h>
#include "isearch.hxx"
#include "doctype.hxx"

DOCTYPE::DOCTYPE(IDBOBJ* DbParent) {
	Db = DbParent;
}


void 
DOCTYPE::ParseRecords(const RECORD& FileRecord) {
	Db->DocTypeAddRecord(FileRecord);
}


void
DOCTYPE::ReplaceWithSpace(PCHR data, INT length)
{
  PCHR    p;
  
  for (p = data; p < (data + length); p++) 
    {
      *p = tolower(*p);
      if (!IsAlnum(*p)) 
	{
	  *p = ' ';
	}       
    }
  *p = '\0';                      // Add a NULL to terminate the record
}


GPTYPE 
DOCTYPE::ParseWords(
		    //@ManMemo: Pointer to document text buffer.
		    CHR* DataBuffer,
		    //@ManMemo: Length of document text buffer in # of characters.
		    INT DataLength,
		    //@ManMemo: Offset that must be added to all GP positions because GP space is shared with other documents.
		    INT DataOffset,
		    //@ManMemo: Pointer to document (word-beginning) GP buffer.
		    GPTYPE* GpBuffer,
		    //@ManMemo: Length of document GP buffer in # of GPTYPE elements, i.e. sizeof(GPTYPE).
		    INT GpLength
		    ) 
{     // This code began life as INDEX::BuildGpList().
  INT GpListSize = 0;
  INT Position = 0;
  while (Position < DataLength) {
    while ( (Position < DataLength) &&
	   (!IsAlnum(DataBuffer[Position])) ) {
      //	   (!isalnum(DataBuffer[Position])) ) {
      Position++;
    }
    if ( (Position < DataLength) &&
	(!(Db->IsStopWord(DataBuffer + Position,
			  DataLength - Position))) ) {
      if (GpListSize >= GpLength) {
         cout << "GpListSize >= GpLength" << endl;
         exit(1);
      }
      GpBuffer[GpListSize++] = DataOffset + Position;
    }
    while ( (Position < DataLength) &&
	   (IsAlnum(DataBuffer[Position])) ) {
      //	   (isalnum(DataBuffer[Position])) ) {
      Position++;
    }
  }
  return GpListSize;
}     // Return # of GPs added to GpBuffer


DOUBLE 
DOCTYPE::ParseNumeric(const CHR *Buffer){
  STRING Hold;
  Hold = Buffer;
  if (Hold.IsNumber())
    return(Hold.GetFloat());
  else
    return 0;
}


void 
DOCTYPE::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 STRING* StringBufferPtr) {
  STRING FieldName;
  GDT_BOOLEAN Status;
  *StringBufferPtr = "";
  if (ElementSet.Equals("F")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;
  } else if (ElementSet.Equals("B")) {
    ResultRecord.GetFileName(StringBufferPtr);
  } else {
    Status = Db->GetFieldData(ResultRecord, ElementSet, StringBufferPtr);
  }
/*
  *StringBufferPtr = "";
  if (ElementSet.Equals("F")) {
    ResultRecord.GetRecordData(StringBufferPtr);
    return;
  }
  if (Db->DfdtGetTotalEntries() == 0) {
    return;
  }
  STRING FieldName;
  if (ElementSet.Equals("B")) {
    DFD Dfd;
    Db->DfdtGetEntry(1, &Dfd);
    Dfd.GetFieldName(&FieldName);
  } else {
    FieldName = ElementSet;
  }
  STRLIST Strlist;
  GDT_BOOLEAN Status;
  Status = Db->GetFieldData(ResultRecord, FieldName, &Strlist);
  if (Status)
    Strlist.Join("\n", StringBufferPtr);
  else
    *StringBufferPtr = "";
*/
}


void 
DOCTYPE::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING* StringBufferPtr) {
  Present(ResultRecord, ElementSet, StringBufferPtr);
}


void 
DOCTYPE::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, GDT_BOOLEAN HighlightTerms,
		 STRING* StringBufferPtr) {
  if ( (HighlightTerms) && (ElementSet ^= "F") ) {
    if (RecordSyntax == HtmlRecordSyntax) {
      ResultRecord.GetHighlightedRecord("<B>", "</B>", StringBufferPtr);
    }
    else if (RecordSyntax == SutrsRecordSyntax) {
      ResultRecord.GetHighlightedRecord("***", "***", StringBufferPtr);
    } else {
      Present(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
    }
  } else {
    Present(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
  }
}


DOCTYPE::~DOCTYPE() {
}


REGISTRY* 
DOCTYPE::GetMetadata(const RECORD& record, const STRING&
		     mdType, const REGISTRY* defaults) {
  REGISTRY* meta = defaults->clone();
  RESULT result;
  STRING s;
  record.GetKey(&s);
  result.SetKey(s);
  record.GetPathName(&s);
  result.SetPathName(s);
  record.GetFileName(&s);
  result.SetFileName(s);
  record.GetDocumentType(&s);
  result.SetDocumentType(s);
  result.SetRecordStart(record.GetRecordStart());
  result.SetRecordEnd(record.GetRecordEnd());

  STRLIST position, value;
	
  // add the title
       Present(result, "title", &s);
  position.AddEntry("locator");
  position.AddEntry("title");
  value.AddEntry(s);
  meta->SetData(position, value);

  return meta;
}

