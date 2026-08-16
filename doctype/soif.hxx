// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// $Id: soif.hxx,v 1.2 2000/02/24 20:51:21 cnidr Exp $
/*
File:        soif.hxx
Version:     1
$Revision: 1.2 $
Description: Class SOIF - Harvest SOIF records
             (derived from bibtex.cxx by Erik Scott)
Author:      Peter Valkenburg
*/


#ifndef SOIF_HXX
#define SOIF_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// A DOCTYPE for Harvest SOIF (Summary Object Interchange Format)
// records: `ParseRecords()` treats the whole file as a single record
// (no multi-record splitting), and `ParseFields()` parses either a
// leading `"@FILE { <url>\n"` line (mapped to a "url" field) or
// `"name{len}:\t<len bytes>\n"` attribute/value pairs, up to the
// closing `"}\n"`.
class SOIF : public DOCTYPE {
public:
   SOIF(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(PRECORD NewRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~SOIF();
};

typedef SOIF* PSOIF;

#endif
