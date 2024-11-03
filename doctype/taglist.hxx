// $Id: taglist.hxx,v 1.3 2000/10/12 20:55:25 cnidr Exp $
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
File:		taglist.hxx
Version:	1.0
$Revision: 1.3 $
Description:	Class TAGLIST - SGML-like Document Type
Author:		Richard Shiels
Changes:	See taglist.cxx
@@@*/

#ifndef TAGLIST_HXX
#define TAGLIST_HXX

//#include <sys/time.h>
#include <time.h>
#include "defs.hxx"
#include "doctype.hxx"
#include "sgmltag.hxx"

struct EntryType
{
	int	offset;
	int length;
};

class TAGLIST 
  : public SGMLTAG {
public:
    TAGLIST(PIDBOBJ DbParent);
    ~TAGLIST();
//	bool IsValidTag(char *tag);
	void ParseFields(PRECORD NewRecord);
	GPTYPE ParseWords(CHR* DataBuffer, INT DataLength, 
			    INT DataOffset, GPTYPE* GpBuffer, 
			    INT GpLength);
	void	 ReplaceWithSpace(PCHR data, INT length);
public:
	GDT_BOOLEAN UsefulSearchField(const STRING& Field);

private:
	EntryType		* m_TagPos;
	INT		m_NumPairs;
};

typedef TAGLIST *PTAGLIST;

#endif
