// $Id: fgdcsite.hxx,v 1.4 1998/06/20 00:06:14 cnidr Exp $
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
File:		fgdcsite.hxx
Version:	$Revision: 1.4 $
Description:	Class FGDCSITE - locator records for FGDC nodes
Author:		Archie Warnock
Changes:	
@@@*/

#ifndef FGDCSITE_HXX
#define FGDCSITE_HXX

//#include <sys/time.h>
#include <time.h>
#include "defs.hxx"
#include "isearch.hxx"
#include "doctype.hxx"
#include "sgmltag.hxx"

#define FGDC_SGML_EXTENSION "sgml"
#define FGDC_HTML_EXTENSION "html"
#define FGDC_TEXT_EXTENSION "text"

#define SHORT_FGDC_SGML_EXTENSION "sgm"
#define SHORT_FGDC_HTML_EXTENSION "htm"
#define SHORT_FGDC_TEXT_EXTENSION "txt"

#define FGDC_SGML_EXTENSION_UC "SGML"
#define FGDC_HTML_EXTENSION_UC "HTML"
#define FGDC_TEXT_EXTENSION_UC "TEXT"

#define SHORT_FGDC_SGML_EXTENSION_UC "SGM"
#define SHORT_FGDC_HTML_EXTENSION_UC "HTM"
#define SHORT_FGDC_TEXT_EXTENSION_UC "TXT"


class FGDCSITE 
  : public SGMLTAG 
{
public:
  FGDCSITE(PIDBOBJ DbParent);
  void LoadFieldTable();
  ~FGDCSITE();
  void Present(const RESULT& ResultRecord, 
	       const STRING& ElementSet,
	       const STRING& RecordSyntax, 
	       STRING* StringBufferPtr);

private:
  GDT_BOOLEAN UsefulSearchField(const STRING& Field);
};

typedef FGDCSITE* PFGDCSITE;

#endif
