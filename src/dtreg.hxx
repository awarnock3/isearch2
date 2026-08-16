/*@@@
File:		dtreg.hxx
Version:	1.00
Description:	Class DTREG - Document Type Registry
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef DTREG_HXX
#define DTREG_HXX

#include "../doctype/doctype.hxx"
#include "../doctype/simple.hxx"
#include "../doctype/markdown.hxx"
#include "../doctype/sgmltag.hxx"
#include "../doctype/firstline.hxx"
#include "../doctype/colondoc.hxx"
#include "../doctype/iafadoc.hxx"
#include "../doctype/mailfolder.hxx"
#include "../doctype/referbib.hxx"
#include "../doctype/irlist.hxx"
#include "../doctype/listdigest.hxx"
#include "../doctype/maildigest.hxx"
#include "../doctype/medline.hxx"
#include "../doctype/filmline.hxx"
#include "../doctype/memodoc.hxx"
#include "../doctype/sgmlnorm.hxx"
#include "../doctype/html.hxx"
#include "../doctype/oneline.hxx"
#include "../doctype/para.hxx"
#include "../doctype/filename.hxx"
#include "../doctype/ftp.hxx"
#include "../doctype/emacsinfo.hxx"
#include "../doctype/gopher.hxx"
#include "../doctype/bibtex.hxx"
#include "../doctype/usmarc.hxx"
#include "../doctype/anzlic.hxx"
#include "../doctype/dif.hxx"
#include "../doctype/gils.hxx"
#include "../doctype/fgdc.hxx"
#include "../doctype/fgdcsite.hxx"
#include "../doctype/marcdump.hxx"
#include "../doctype/gilsxml.hxx"
#include "../doctype/cipp.hxx"
#include "../doctype/eos_guide.hxx"
#include "../doctype/htmltag.hxx"
#include "../doctype/soif.hxx"
#include "../doctype/anzmeta.hxx"
#include "../doctype/taglist.hxx"
#include "../doctype/cipc.hxx"

class DTREG {
public:
	DTREG(PIDBOBJ DbParent);
	PDOCTYPE GetDocTypePtr(const STRING& DocType);
	void GetDocTypeList(PSTRLIST StringListBuffer) const;
	~DTREG();
private:
	PIDBOBJ Db;
	PDOCTYPE DtDocType;
	PSIMPLE DtSIMPLE;
	PMARKDOWN DtMARKDOWN;
	PSGMLTAG DtSGMLTAG;
	PFIRSTLINE DtFIRSTLINE;
	PCOLONDOC DtCOLONDOC;
	PIAFADOC DtIAFADOC;
	PMAILFOLDER DtMAILFOLDER;
	PREFERBIB DtREFERBIB;
	PIRLIST DtIRLIST;
	PLISTDIGEST DtLISTDIGEST;
	PMAILDIGEST DtMAILDIGEST;
	PMEDLINE DtMEDLINE;
	PFILMLINE DtFILMLINE;
	PMEMODOC DtMEMODOC;
	PSGMLNORM DtSGMLNORM;
	PHTML DtHTML;
	PONELINE DtONELINE;
	PPARA DtPARA;
	PFILENAME DtFILENAME;
	PFTP DtFTP;
	PEMACSINFO DtEMACSINFO;
	PGOPHER DtGOPHER;
	PBIBTEX DtBIBTEX;
	PUSMARC DtUSMARC;
	PANZLIC DtANZLIC;
	PDIF DtDIF;
	PGILS DtGILS;
	PFGDC DtFGDC;
	PFGDCSITE DtFGDCSITE;
	PMARCDUMP DtMARCDUMP;
	PGILSXML DtGILSXML;
	PCIPP DtCIPP;
	PEOS_GUIDE DtEOS_GUIDE;
	PHTMLTAG DtHTMLTAG;
	PSOIF DtSOIF;
	PANZMETA DtANZMETA;
	PTAGLIST DtTAGLIST;
	PCIPC DtCIPC;
};

typedef DTREG* PDTREG;

#endif
