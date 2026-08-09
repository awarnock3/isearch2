/* $Id: defs.cxx,v 1.8 2000/03/07 12:03:28 cnidr Exp $ */
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
File:		defs.cxx
Version:	1.02
$Revision: 1.8 $
Description:	General definitions
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "gdt.h"

const CHR* IsearchDefaultDbName = "ISEARCH";

const CHR* IsearchVersion     = VERS; // Passed down in Makefile

const CHR* Bib1AttributeSet = "1.2.840.10003.3.1";
const CHR* GilsAttributeSet = "1.2.840.10003.3.5";
const CHR* StasAttributeSet = "1.2.840.10003.3.6";
const CHR* GeoAttributeSet  = "1.2.840.10003.3.9";
const CHR* CipAttributeSet  = "1.2.840.10003.3.1000.99.1";
//const CHR* GilsAttributeSet = "1.2.840.10003.3.3";
//const CHR* StasAttributeSet = "1.2.840.10003.3.1000.6.1";
const CHR* IsearchAttributeSet = "1.2.840.10003.3.1000.34.1";

// Record Syntaxes
const CHR* SutrsRecordSyntax    = "SUTRS";
const CHR* UsmarcRecordSyntax   = "USMARC";
const CHR* HtmlRecordSyntax     = "HTML";
const CHR* SgmlRecordSyntax     = "SGML";
const CHR* XmlRecordSyntax      = "XML";
const CHR* MimeRecordSyntax     = "MIME";
const CHR* GRS1RecordSyntax     = "GRS-1";

const CHR* UsmarcRecordSyntaxOID    = "1.2.840.10003.5.10";
const CHR* SutrsRecordSyntaxOID     = "1.2.840.10003.5.101";
const CHR* GRS1RecordSyntaxOID      = "1.2.840.10003.5.105";
const CHR* OldHtmlRecordSyntaxOID   = "1.2.840.10003.5.108";
const CHR* MimeRecordSyntaxOID      = "1.2.840.10003.5.109";
const CHR* HtmlRecordSyntaxOID      = "1.2.840.10003.5.109.3";
const CHR* SgmlRecordSyntaxOID      = "1.2.840.10003.5.109.9";
const CHR* XmlRecordSyntaxOID       = "1.2.840.10003.5.109.10";
const CHR* CNIDRHtmlRecordSyntaxOID = "1.2.840.10003.5.1000.34.1";
const CHR* CNIDRSgmlRecordSyntaxOID = "1.2.840.10003.5.1000.34.2";

const CHR* DbExtDbInfo = ".dbi";
const CHR* DbExtIndex = ".inx";
const CHR* DbExtMdt = ".mdt";
const CHR* DbExtMdtKeyIndex = ".mdk";
const CHR* DbExtMdtGpIndex = ".mdg";
const CHR* DbExtDfd = ".dfd";
const CHR* DbExtIndexQueue1 = ".iq1";
const CHR* DbExtIndexQueue2 = ".iq2";
const CHR* DbExtTemp = ".tmp";
const CHR* DbExtDict = ".dic";
const CHR* DbExtSparse = ".spr";
const CHR* DbExtCentroid = ".cen";
const CHR* DbExtDbState = ".sta";
