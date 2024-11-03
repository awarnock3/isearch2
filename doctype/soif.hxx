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
