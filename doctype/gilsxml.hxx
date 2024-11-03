// $Id: gilsxml.hxx,v 1.2 1998/11/04 04:49:15 cnidr Exp $
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
File:		gilsxml.hxx
Version:	$Revision: 1.2 $
Description:	Class GILSXML - for XML-tagged GILS records
Derived from:   SGMLTAG - SGML-like Document Type
Author:		Archie Warnock (awww@home.com), A/WWW Enterprises
Originally by:  Kevin Gamiel, Kevin.Gamiel@cnidr.org
@@@*/

#ifndef GILSXML_HXX
#define GILSXML_HXX

//#include <sys/time.h>
#include <time.h>
#include "defs.hxx"
#include "doctype.hxx"
#include "sgmltag.hxx"

class GILSXML
  : public SGMLTAG {
public:
    GILSXML(IDBOBJ *DbParent);
    ~GILSXML();
    //    void ParseFields(PRECORD NewRecord);
    //    void   LoadFieldTable() {};
    //    void   AddFieldDefs() {};

    void   Present(const RESULT& ResultRecord, 
		   const STRING& ElementSet,
		   STRING* StringBufferPtr);
    void   Present(const RESULT& ResultRecord, 
		   const STRING& ElementSet,
		   const STRING& RecordSyntax, 
		   STRING* StringBufferPtr);
    void Present_HTML_G(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SGML_G(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SUTRS_G(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_HTML_S(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SGML_S(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SUTRS_S(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_HTML_F(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SGML_F(const RESULT& ResultRecord, STRING *StringBuffer);
    void Present_SUTRS_F(const RESULT& ResultRecord, STRING *StringBuffer);
    /*
      void   ParseDate(const CHR *Buffer, DOUBLE* fStart, 
      DOUBLE* fEnd) {};
      void   ParseDate(const STRING& Buffer, DOUBLE* fStart, 
      DOUBLE* fEnd) {};
      void   ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
      DOUBLE* fEnd) {};
      void   ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
      DOUBLE* fEnd) {};
      DOUBLE ParseNumeric(const CHR *Buffer);
      void   ParseGPoly(const CHR *Buffer) {};
    */

private:
    GDT_BOOLEAN UsefulSearchField(const STRING& Field);

};

typedef GILSXML* PGILSXML;

#endif
