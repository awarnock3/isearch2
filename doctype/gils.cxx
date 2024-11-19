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
File:		gils.cxx
Version:	1.00
Description:	Class GILS - SGML-like Text w/ static output files
Author:		Archie Warnock, warnock@clark.net
@@@*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "isearch.hxx"
#include "gils.hxx"


// Local prototypes
GILS::GILS(PIDBOBJ DbParent) : SGMLNORM(DbParent) {
}

void GILS::Present(const RESULT& ResultRecord, const STRING& ElementSet, 
		     const STRING& RecordSyntax, PSTRING StringBuffer)
{

  if (ElementSet.Equals("B")) {
    STRLIST Strlist;
    STRING TitleTag,Title;
//    PCHR headline;
    GDT_BOOLEAN Status;

    TitleTag = "title"; // Brief headline is "title"
    Status = Db->GetFieldData(ResultRecord, TitleTag, &Strlist);
    if (Status) {
      Strlist.Join("\n",&Title);
      Title.Replace("\n"," ");
      Title.Replace("\r"," ");
    } else
      Title = "";
    *StringBuffer = Title;

  } else {
    PCHR b;
    PFILE fp;
    STRING FullFilename;
    INT n, filelen;
    LONG lRecStart, lRecEnd, ActualLength;

    ResultRecord.GetFullFileName(&FullFilename);
    n = FullFilename.SearchReverse('.');
    FullFilename.EraseAfter(n);

    if (RecordSyntax.Equals(HtmlRecordSyntax))
      FullFilename.Cat(GILS_HTML_EXTENSION);
    else if (RecordSyntax.Equals(HtmlRecordSyntax))
      FullFilename.Cat(GILS_HTML_EXTENSION);
    else if (RecordSyntax.Equals(SutrsRecordSyntax))
      FullFilename.Cat(GILS_TEXT_EXTENSION);
    else if (RecordSyntax.Equals(SgmlRecordSyntax))
      FullFilename.Cat(GILS_SGML_EXTENSION);
    else if (RecordSyntax.Equals(SgmlRecordSyntax))
      FullFilename.Cat(GILS_SGML_EXTENSION);
    else
      FullFilename.Cat(GILS_XML_EXTENSION);

    lRecStart=0L;

    fp = fopen(FullFilename, "rb");
    if (!fp) {
      *StringBuffer = "File not found";
      return;
    }

    if ((n=fseek(fp, 0L, SEEK_END)) != 0) {
      return;
    }

    lRecEnd = ftell(fp);

    if ((n=fseek(fp, lRecStart, SEEK_SET)) != 0) {
      return;
    }

    filelen = lRecEnd - lRecStart;
    b = new CHR[filelen + 1];
    ActualLength = fread(b, 1, filelen, fp);
    fclose(fp);
    b[ActualLength] = '\0';
  
    if (ActualLength != 0) {
      *StringBuffer = b;
    }
    delete [] b;
  }
  return;
}

GILS::~GILS() {
}

