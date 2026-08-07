/* $Id: defs.hxx,v 1.26 2000/11/15 21:30:18 cnidr Exp $ */
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
File:		defs.hxx
Version:	1.02
$Revision: 1.26 $
Description:	General definitions
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
//
// Tree-wide grab bag of `extern` declarations (defined in defs.cxx),
// GDT-derived typedefs, Z39.50/GILS attribute and structure-type
// numbers, on-disk file-extension/size constants, and a handful of
// convenience macros (COUT/EXIT_ERROR/RETURN_ERROR/RETURN_ZERO).
// Included from nearly everywhere in the tree, so nothing here should
// change in a way that isn't purely additive -- see BUGFIX #1 and the
// deferred macro-hygiene note below for the two things actually found.

#ifndef DEFS_HXX
#define DEFS_HXX

#ifdef UNIX
#include <unistd.h>
#elif defined(_WIN32) || defined(_MSC_VER)
#include <process.h>
#endif

#include "gdt.h"
#include <iostream> // BUGFIX #1 (see docs/BUG_CATALOG.md#srcdefshxx): needed for std::cout, below.

extern const CHR* IsearchDefaultDbName;
extern const CHR* IsearchVersion;

typedef UINT4 GPTYPE;
typedef GPTYPE* PGPTYPE;

extern const CHR* Bib1AttributeSet;
extern const CHR* GilsAttributeSet;
extern const CHR* GeoAttributeSet;
extern const CHR* CipAttributeSet;
extern const CHR* StasAttributeSet;

extern const CHR* IsearchAttributeSet;

// Record Syntaxes
extern const CHR* SutrsRecordSyntax;
extern const CHR* UsmarcRecordSyntax;
extern const CHR* HtmlRecordSyntax;
extern const CHR* SgmlRecordSyntax;
extern const CHR* XmlRecordSyntax;
extern const CHR* MimeRecordSyntax;
extern const CHR* GRS1RecordSyntax;

extern const CHR* UsmarcRecordSyntaxOID;
extern const CHR* SutrsRecordSyntaxOID;
extern const CHR* GRS1RecordSyntaxOID;
extern const CHR* OldHtmlRecordSyntaxOID;
extern const CHR* MimeRecordSyntaxOID;
extern const CHR* HtmlRecordSyntaxOID;
extern const CHR* SgmlRecordSyntaxOID;
extern const CHR* XmlRecordSyntaxOID;
extern const CHR* CNIDRHtmlRecordSyntaxOID;
extern const CHR* CNIDRSgmlRecordSyntaxOID;

extern const CHR* DbExtDbInfo;
extern const CHR* DbExtIndex;
extern const CHR* DbExtMdt;
extern const CHR* DbExtMdtKeyIndex;
extern const CHR* DbExtMdtGpIndex;
extern const CHR* DbExtDfd;
extern const CHR* DbExtIndexQueue1;
extern const CHR* DbExtIndexQueue2;
extern const CHR* DbExtTemp;
extern const CHR* DbExtDict;
extern const CHR* DbExtSparse;
extern const CHR* DbExtCentroid;
extern const CHR* DbExtDbState;

const INT  IsearchMagicNumber  = 7;    // Updated for new numeric indexes
const INT  IsearchFieldAttr    = 1;
const INT  IsearchWeightAttr   = 7;
const INT  IsearchTypeAttr     = 8;

// Z39.50 Attribute Numbers
const INT ZdistUseAttr          = 1;
const INT ZdistRelationAttr     = 2;
const INT ZdistPositionAttr     = 3;
const INT ZdistStructureAttr    = 4;
const INT ZdistTruncationAttr   = 5;
const INT ZdistCompletenessAttr = 6;

// Z39.50 AttributeValues (also other profiles)
const INT ZRelLT             = 1;
const INT ZRelLE             = 2;
const INT ZRelEQ             = 3;
const INT ZRelGE             = 4;
const INT ZRelGT             = 5;
const INT ZRelNE             = 6;
const INT ZRelOverlaps       = 7;
const INT ZRelEnclosedWithin = 8;
const INT ZRelEncloses       = 9;
const INT ZRelOutside        = 10;
const INT ZRelNear           = 11;
const INT ZRelMembersEQ      = 12;
const INT ZRelMembersNE      = 13;
const INT ZRelBefore         = 14;
const INT ZRelBeforeDuring   = 15;
const INT ZRelDuring         = 16;
const INT ZRelDuringAfter    = 17;
const INT ZRelAfter          = 18;

// Strict date searching for GEO profile tests
// The default date search for GEO matches if any date in the target
// interval matches the query date.  These attributes change the default
// to match only if all dates in the target interval matches the query.
const INT ZRelBefore_Strict       = 3014;
const INT ZRelBeforeDuring_Strict = 3015;
const INT ZRelDuring_Strict       = 3016;
const INT ZRelDuringAfter_Strict  = 3017;
const INT ZRelAfter_Strict        = 3018;

const INT ZPosFirstField    = 1;
const INT ZPosFirstSubField = 2;
const INT ZPosAny           = 3;

const INT ZStructPhrase      = 1;
const INT ZStructWord        = 2;
const INT ZStructKey         = 3;
const INT ZStructYear        = 4;
const INT ZStructDate        = 5;
const INT ZStructWordList    = 6;
const INT ZStructDateTime    = 100;
const INT ZStructNameNorm    = 101;
const INT ZStructNameUnnorm  = 102;
const INT ZStructStructure   = 103;
const INT ZStructURx         = 104;
const INT ZStructText        = 105;
const INT ZStructDocText     = 106;
const INT ZStructLocalNumber = 107;
const INT ZStructCoord       = 108;
const INT ZStructCoordString = 109;
const INT ZStructCoordRange  = 110;
const INT ZStructBounding    = 111;
const INT ZStructComposite   = 112;
const INT ZStructRealMeas    = 113;
const INT ZStructIntMeas     = 114;
const INT ZStructDateRange   = 115;

const INT ZGEOStructCoord       = 200;
const INT ZGEOStructCoordString = 201;
const INT ZGEOStructBounding    = 202;
const INT ZGEOStructComposite   = 204;
const INT ZGEOStructRealMeas    = 205;
const INT ZGEOStructIntMeas     = 206;
const INT ZGEOStructDateRange   = 207;
const INT ZGEOStructDateString  = 210;

const INT ZTruncRight     = 1;
const INT ZTruncLeft      = 2;
const INT ZTruncLeftRight = 3;
const INT ZTruncNone      = 100;
const INT ZTruncProcess   = 101;
const INT ZTruncRE1       = 102;
const INT ZTruncRE2       = 103;

const INT ZCompleteIncSub    = 1;
const INT ZCompleteCompSub   = 2;
const INT ZCompleteCompField = 3;

// Op Types
const INT TypeOperand  = 1;
const INT TypeOperator = 2;
// Operand Types
const INT TypeTerm = 1;
const INT TypeRset = 2;
// Operator Types
const INT OperatorOr       = 1;
const INT OperatorAnd      = 2;
const INT OperatorAndNot   = 3;
const INT OperatorNear     = 15;
const INT OperatorCharProx = 16;

const INT DocumentKeySize  = 16;
const INT DocumentTypeSize = 64;
const INT DocFileNameSize  = 512;
const INT DocPathNameSize  = 2048;
const INT StringCompLength = 32;	// how many characters to compare

const INT4 IsearchDbStateReady   = 0;  // DB ready for searching
const INT4 IsearchDbStateBusy    = 1;  // DB files are being modified
const INT4 IsearchDbStateInvalid = 2;  // DB files have been corrupted because  some operation was interrupted

const INT IndexingStatusParsingDocument = 1;
const INT IndexingStatusIndexing        = 2;
const INT IndexingStatusMerging         = 3;
const INT IndexingStatusParsingFiles    = 4;
const INT IndexingStatusKeySet          = 5;

//#if defined(PLATFORM_MSVC) || defined (_MSDOS) || defined (_WIN32)
#if defined(PLATFORM_MSVC)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

// BUGFIX #1 (see docs/BUG_CATALOG.md#srcdefshxx): was `#define COUT
// cout` -- an unqualified name with nothing in this header to declare
// it. It only ever compiled because every real caller happened to
// separately pull in `using namespace std;` (via string.hxx) ahead of
// this header -- true today, not guaranteed. Qualifying it here and
// including <iostream> above makes COUT work regardless of what else
// a caller has included.
#define COUT std::cout

// Found but deliberately NOT changed here (see docs/BUG_CATALOG.md#srcdefshxx,
// "found but out of scope"): none of these three wrap their body in
// `do { ... } while(0)`, the standard hygiene fix for multi-statement
// macros, so a use like `if (x) EXIT_ERROR; else ...` can silently
// misparse. Left alone because src/result.cxx:229 -- already compiled
// by `make tests` -- invokes EXIT_ERROR with no trailing semicolon,
// relying on today's bare-`{}` expansion; wrapping in do/while(0)
// would break that call site, which is out of scope for this turn.
#define EXIT_ERROR {fflush(stdout); fflush(stderr); exit(1);}
#define RETURN_ERROR {fflush(stdout); fflush(stderr); return(1);}
#define RETURN_ZERO {fflush(stdout); fflush(stderr); return(0);}
#endif
