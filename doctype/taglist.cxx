// $Id: taglist.cxx,v 1.3 2000/10/12 20:55:25 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included i
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to retur
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
File:           taglist.cxx
Version:        1.0
$Revision: 1.3 $
Description:    Class TAGLIST - SGML-like Text
			Derived from SGMLTAG. Uses a list of valid tags to verify
			that the inputed tag should be excepted. Used so that you ca
			create an index containing only specific information(such as
			headings)
Author:         Richard Shiels
Changes:

TODO:
			Modify to read valid tags from an input file
			It may be worth implementing this functionality for all tag
				based indexes.
@@@*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "taglist.hxx"

TAGLIST::TAGLIST(PIDBOBJ DbParent) : SGMLTAG(DbParent) {
	m_TagPos = NULL;
	m_NumPairs = 0;
}

void
TAGLIST::ReplaceWithSpace(PCHR data, INT length)
{
	PCHR	p;
	CHR		**tags;
	CHR		**tags_ptr;
	INT		numtags;

	tags = sgml_parse_tags(data, length, &numtags);
	tags_ptr = tags;	
	while(*tags_ptr) 
	{
		if (UsefulSearchField(*tags_ptr) == GDT_FALSE)
		{ // Not interested in this tag so lets kill it and its content...
			// replace with spaces upto and including NULL
//			printf("Killing %s\n", *tags_ptr);
			memset(*tags_ptr, ' ', strlen(*tags_ptr) + 1); 
		}
		else
		{ // Want tag so lets just remove the terminating null inserted by sgml_parse_tags
			*tags_ptr[strlen(*tags_ptr)] = ' ';
		}
		tags_ptr++;
	}
	delete tags;

	for (p = data; p < (data + length); p++) 
	{
		*p = tolower(*p);
		if (!IsAlnum(*p)) 
		{
			*p = ' ';
		}	
	}
	*p = '\0';			// Add a NULL to terminate the record
}

char *ValidTags[] = {"TITLE",
					"H1",
					"H2",
					"H3",
					"H4"
					};

const int NUM_TAGS = 5;

GDT_BOOLEAN 
TAGLIST::UsefulSearchField(const STRING& Field) 
{
	int i;
	GDT_BOOLEAN bFound = GDT_FALSE;
	  STRING FieldName;
	  FieldName=Field;
	  FieldName.UpperCase();
	
	for (i = 0; i < NUM_TAGS && bFound == GDT_FALSE; i++)
	{ // loop through all valid tags to see if we have a match
		if (FieldName.Search(ValidTags[i]))
			bFound = GDT_TRUE;  // Found a tag we want to use.
	}
	return(bFound);
}

/*
- Open the file.
- Read two copies of the file into memory (implementation feature;-)
- Build an index into the SGML-like tag pairs.  An SGML-like tag pair is one
	that begins and ends with precisely the same text, excluding the
	closing tag's slash.  For example:

		<title> </title>        - sgml-like
		<dog> </dog>            - sgml-like
		<!-- test>              - NOT sgml-like
		<a href=> </a>          - NOT sgml-like
- For each valid tag pair, hence field, add the field to the Isearch record
	structure. (as long as the tag matches the list of valid tags)
- Cleanup
*/
void 
TAGLIST::ParseFields(PRECORD NewRecord) 
{
  PFILE  fp;
  STRING fn;
  GPTYPE RecStart, 
         RecEnd, 
         RecLength, 
         ActualLength;
  CHR* 	 RecBuffer;
  CHR*   OrigRecBuffer;
  CHR* 	 file;

  // Open the file
  NewRecord->GetFullFileName(&fn);
  file = fn.NewCString();
  fp = fopen(fn, "rb");
  if (!fp) {
    cout << "SGMLTAG::ParseRecords(): Failed to open file\n\t";
    perror(file);
    return;
  }

  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();
  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      cout << "SGMLTAG::ParseRecords(): Seek failed - ";
      cout << fn << "\n";
      fclose(fp);
      return;	
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      cout << "SGMLTAG::ParseRecords(): Skipping ";
      cout << " zero-length record -" << fn << "...\n";
      fclose(fp);
      return;
    }
    //RecEnd -= 1;
  }

  // Make two copies of the record in memory
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "SGMLTAG::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;	
  }
  RecLength = RecEnd - RecStart;
	
  RecBuffer = new CHR[RecLength + 1];
  if(!RecBuffer) {
    cout << "SGMLTAG::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }
  OrigRecBuffer = new CHR[RecLength + 1];
  if(!OrigRecBuffer) {
    cout << "SGMLTAG::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    fclose(fp);
    return;
  }

  ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "SGMLTAG::ParseRecords(): Failed to fread\n\t";
    perror(file);
    delete [] RecBuffer;
    delete [] OrigRecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "SGMLTAG::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    delete [] OrigRecBuffer;
    return;
  }
  memcpy(OrigRecBuffer, RecBuffer, RecLength);
  OrigRecBuffer[RecLength] = '\0';

  // Parse the record and add fields to record structure
  STRING FieldName;
  FC fc;
  PFCT pfct;
  DF df;
  PDFT pdft;
  CHR **tags;
  CHR **tags_ptr;
  CHR* p;
  INT val_start;
  INT val_len;
  DFD dfd;
  INT numtags;

  pdft = new DFT();
  if(!pdft) {
    cout << "SGMLTAG::ParseRecords(): Failed to allocate DFT - ";
    cout << fn << "\n";
    delete [] RecBuffer;
    delete [] OrigRecBuffer;
    return;
  }
  tags = sgml_parse_tags(RecBuffer, RecLength, &numtags);
  if(tags == NULL) {
    cout << "Unable to parse SGML file " << fn << "\n";
    delete pdft;
    delete [] RecBuffer;
    delete [] OrigRecBuffer;
    return;
  }

  tags_ptr = tags;	
  if (m_TagPos != NULL)
	  delete m_TagPos;
  m_TagPos = new EntryType[numtags];
  m_NumPairs = 0;
  while(*tags_ptr) {
    p = find_end_tag(tags_ptr, *tags_ptr);
    if(p) {
      // We have a tag pair
      val_start = (*tags_ptr + strlen(*tags_ptr) + 1) - 
	RecBuffer;
      val_len = (p - *tags_ptr) - strlen(*tags_ptr) - 2;
      FieldName = *tags_ptr;

	  if (UsefulSearchField(FieldName))	// Validate we want to index this tag
	  { // We have found a tag we want indexed so index it
		  m_TagPos[m_NumPairs].offset = val_start;
		  m_TagPos[m_NumPairs].length = val_len;
		  m_NumPairs++;
//		  printf("%s %li %li \n", *tags_ptr, val_start, val_len);
		  dfd.SetFieldName(FieldName);
		  Db->DfdtAddEntry(dfd);
		  fc.SetFieldStart(val_start);
		  fc.SetFieldEnd(val_start + val_len -1);
		  pfct = new FCT();
		  pfct->AddEntry(fc);
		  df.SetFct(*pfct);
		  df.SetFieldName(FieldName);
		  pdft->AddEntry(df);
		  delete pfct;
	  }
    }
    tags_ptr++;
  }

  delete pdft;
  delete [] RecBuffer;
  delete [] OrigRecBuffer;
  delete tags;
}

TAGLIST::~TAGLIST(){
	if (m_TagPos != NULL)
		delete [] m_TagPos;
	m_TagPos = NULL;
	m_NumPairs = 0;
}

GPTYPE 
TAGLIST::ParseWords(
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
{ 
	INT	GpListSize = 0;
	INT	Position = 0;
	INT	SubLength = 0;
	int curtag = 0;

	for (curtag = 0; curtag < m_NumPairs; curtag++)
	{
		Position = m_TagPos[curtag].offset;
		SubLength = m_TagPos[curtag].length;
		SubLength += Position;
		while (Position < SubLength) 
		{
			if (SubLength > DataLength)
				SubLength = DataLength;
			while ( (Position < SubLength) && (!IsAlnum(DataBuffer[Position])) ) 
			{
				Position++;
			}
			if ( (Position < SubLength) &&	(!(Db->IsStopWord(DataBuffer + Position, DataLength - Position))) ) 
			{
				if (GpListSize >= GpLength) 
				{
					cout << "GpListSize >= GpLength" << endl;
					exit(1);
				}
				GpBuffer[GpListSize++] = DataOffset + Position;
			}
			while ( (Position < SubLength) && (IsAlnum(DataBuffer[Position])) ) 
			{
				Position++;
			}
		}
	}
	return GpListSize;
} 
