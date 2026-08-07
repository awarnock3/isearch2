/*@@@
File:		dtreg.cxx
Version:	1.00
Description:	Class DTREG - Document Type Registry
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dfd.hxx"
#include "dft.hxx"
#include "dfdt.hxx"
#include "result.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "rset.hxx"
#include "operand.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"

DTREG::DTREG(PIDBOBJ DbParent) {
	Db = DbParent;
	DtDocType = new DOCTYPE(Db);
	DtSIMPLE = 0;
	DtMARKDOWN = 0;
	DtSGMLTAG = 0;
	DtFIRSTLINE = 0;
	DtCOLONDOC = 0;
	DtIAFADOC = 0;
	DtMAILFOLDER = 0;
	DtREFERBIB = 0;
	DtIRLIST = 0;
	DtLISTDIGEST = 0;
	DtMAILDIGEST = 0;
	DtMEDLINE = 0;
	DtFILMLINE = 0;
	DtMEMODOC = 0;
	DtSGMLNORM = 0;
	DtHTML = 0;
	DtONELINE = 0;
	DtPARA = 0;
	DtFILENAME = 0;
	DtFTP = 0;
	DtEMACSINFO = 0;
	DtGOPHER = 0;
	DtBIBTEX = 0;
	DtUSMARC = 0;
	DtANZLIC = 0;
	DtDIF = 0;
	DtGILS = 0;
	DtFGDC = 0;
	DtFGDCSITE = 0;
	DtMARCDUMP = 0;
	DtGILSXML = 0;
	DtCIPP = 0;
	DtEOS_GUIDE = 0;
	DtHTMLTAG = 0;
	DtSOIF = 0;
	DtANZMETA = 0;
	DtTAGLIST = 0;
	DtCIPC = 0;
}

PDOCTYPE DTREG::GetDocTypePtr(const STRING& DocType) {
	if (DocType.Equals("")) {
		return DtDocType;
	}
	STRING DocTypeID;
	DocTypeID = DocType;
	DocTypeID.UpperCase();
	if (DocTypeID.Equals("SIMPLE")) {
		if (!DtSIMPLE) {
			DtSIMPLE = new SIMPLE(Db);
		}
		return DtSIMPLE;
	}
	if (DocTypeID.Equals("MARKDOWN")) {
		if (!DtMARKDOWN) {
			DtMARKDOWN = new MARKDOWN(Db);
		}
		return DtMARKDOWN;
	}
	if (DocTypeID.Equals("SGMLTAG")) {
		if (!DtSGMLTAG) {
			DtSGMLTAG = new SGMLTAG(Db);
		}
		return DtSGMLTAG;
	}
	if (DocTypeID.Equals("FIRSTLINE")) {
		if (!DtFIRSTLINE) {
			DtFIRSTLINE = new FIRSTLINE(Db);
		}
		return DtFIRSTLINE;
	}
	if (DocTypeID.Equals("COLONDOC")) {
		if (!DtCOLONDOC) {
			DtCOLONDOC = new COLONDOC(Db);
		}
		return DtCOLONDOC;
	}
	if (DocTypeID.Equals("IAFADOC")) {
		if (!DtIAFADOC) {
			DtIAFADOC = new IAFADOC(Db);
		}
		return DtIAFADOC;
	}
	if (DocTypeID.Equals("MAILFOLDER")) {
		if (!DtMAILFOLDER) {
			DtMAILFOLDER = new MAILFOLDER(Db);
		}
		return DtMAILFOLDER;
	}
	if (DocTypeID.Equals("REFERBIB")) {
		if (!DtREFERBIB) {
			DtREFERBIB = new REFERBIB(Db);
		}
		return DtREFERBIB;
	}
	if (DocTypeID.Equals("IRLIST")) {
		if (!DtIRLIST) {
			DtIRLIST = new IRLIST(Db);
		}
		return DtIRLIST;
	}
	if (DocTypeID.Equals("LISTDIGEST")) {
		if (!DtLISTDIGEST) {
			DtLISTDIGEST = new LISTDIGEST(Db);
		}
		return DtLISTDIGEST;
	}
	if (DocTypeID.Equals("MAILDIGEST")) {
		if (!DtMAILDIGEST) {
			DtMAILDIGEST = new MAILDIGEST(Db);
		}
		return DtMAILDIGEST;
	}
	if (DocTypeID.Equals("MEDLINE")) {
		if (!DtMEDLINE) {
			DtMEDLINE = new MEDLINE(Db);
		}
		return DtMEDLINE;
	}
	if (DocTypeID.Equals("FILMLINE")) {
		if (!DtFILMLINE) {
			DtFILMLINE = new FILMLINE(Db);
		}
		return DtFILMLINE;
	}
	if (DocTypeID.Equals("MEMODOC")) {
		if (!DtMEMODOC) {
			DtMEMODOC = new MEMODOC(Db);
		}
		return DtMEMODOC;
	}
	if (DocTypeID.Equals("SGMLNORM")) {
		if (!DtSGMLNORM) {
			DtSGMLNORM = new SGMLNORM(Db);
		}
		return DtSGMLNORM;
	}
	if (DocTypeID.Equals("HTML")) {
		if (!DtHTML) {
			DtHTML = new HTML(Db);
		}
		return DtHTML;
	}
	if (DocTypeID.Equals("ONELINE")) {
		if (!DtONELINE) {
			DtONELINE = new ONELINE(Db);
		}
		return DtONELINE;
	}
	if (DocTypeID.Equals("PARA")) {
		if (!DtPARA) {
			DtPARA = new PARA(Db);
		}
		return DtPARA;
	}
	if (DocTypeID.Equals("FILENAME")) {
		if (!DtFILENAME) {
			DtFILENAME = new FILENAME(Db);
		}
		return DtFILENAME;
	}
	if (DocTypeID.Equals("FTP")) {
		if (!DtFTP) {
			DtFTP = new FTP(Db);
		}
		return DtFTP;
	}
	if (DocTypeID.Equals("EMACSINFO")) {
		if (!DtEMACSINFO) {
			DtEMACSINFO = new EMACSINFO(Db);
		}
		return DtEMACSINFO;
	}
	if (DocTypeID.Equals("GOPHER")) {
		if (!DtGOPHER) {
			DtGOPHER = new GOPHER(Db);
		}
		return DtGOPHER;
	}
	if (DocTypeID.Equals("BIBTEX")) {
		if (!DtBIBTEX) {
			DtBIBTEX = new BIBTEX(Db);
		}
		return DtBIBTEX;
	}
	if (DocTypeID.Equals("USMARC")) {
		if (!DtUSMARC) {
			DtUSMARC = new USMARC(Db);
		}
		return DtUSMARC;
	}
	if (DocTypeID.Equals("ANZLIC")) {
		if (!DtANZLIC) {
			DtANZLIC = new ANZLIC(Db);
		}
		return DtANZLIC;
	}
	if (DocTypeID.Equals("DIF")) {
		if (!DtDIF) {
			DtDIF = new DIF(Db);
		}
		return DtDIF;
	}
	if (DocTypeID.Equals("GILS")) {
		if (!DtGILS) {
			DtGILS = new GILS(Db);
		}
		return DtGILS;
	}
	if (DocTypeID.Equals("FGDC")) {
		if (!DtFGDC) {
			DtFGDC = new FGDC(Db);
		}
		return DtFGDC;
	}
	if (DocTypeID.Equals("FGDCSITE")) {
		if (!DtFGDCSITE) {
			DtFGDCSITE = new FGDCSITE(Db);
		}
		return DtFGDCSITE;
	}
	if (DocTypeID.Equals("MARCDUMP")) {
		if (!DtMARCDUMP) {
			DtMARCDUMP = new MARCDUMP(Db);
		}
		return DtMARCDUMP;
	}
	if (DocTypeID.Equals("GILSXML")) {
		if (!DtGILSXML) {
			DtGILSXML = new GILSXML(Db);
		}
		return DtGILSXML;
	}
	if (DocTypeID.Equals("CIPP")) {
		if (!DtCIPP) {
			DtCIPP = new CIPP(Db);
		}
		return DtCIPP;
	}
	if (DocTypeID.Equals("EOS_GUIDE")) {
		if (!DtEOS_GUIDE) {
			DtEOS_GUIDE = new EOS_GUIDE(Db);
		}
		return DtEOS_GUIDE;
	}
	if (DocTypeID.Equals("HTMLTAG")) {
		if (!DtHTMLTAG) {
			DtHTMLTAG = new HTMLTAG(Db);
		}
		return DtHTMLTAG;
	}
	if (DocTypeID.Equals("SOIF")) {
		if (!DtSOIF) {
			DtSOIF = new SOIF(Db);
		}
		return DtSOIF;
	}
	if (DocTypeID.Equals("ANZMETA")) {
		if (!DtANZMETA) {
			DtANZMETA = new ANZMETA(Db);
		}
		return DtANZMETA;
	}
	if (DocTypeID.Equals("TAGLIST")) {
		if (!DtTAGLIST) {
			DtTAGLIST = new TAGLIST(Db);
		}
		return DtTAGLIST;
	}
	if (DocTypeID.Equals("CIPC")) {
		if (!DtCIPC) {
			DtCIPC = new CIPC(Db);
		}
		return DtCIPC;
	}
	return DtDocType;
}

void DTREG::GetDocTypeList(PSTRLIST StringListBuffer) const {
	STRING s;
	STRLIST DocTypeList;
	s = "SIMPLE";
	DocTypeList.AddEntry(s);
	s = "MARKDOWN";
	DocTypeList.AddEntry(s);
	s = "SGMLTAG";
	DocTypeList.AddEntry(s);
	s = "FIRSTLINE";
	DocTypeList.AddEntry(s);
	s = "COLONDOC";
	DocTypeList.AddEntry(s);
	s = "IAFADOC";
	DocTypeList.AddEntry(s);
	s = "MAILFOLDER";
	DocTypeList.AddEntry(s);
	s = "REFERBIB";
	DocTypeList.AddEntry(s);
	s = "IRLIST";
	DocTypeList.AddEntry(s);
	s = "LISTDIGEST";
	DocTypeList.AddEntry(s);
	s = "MAILDIGEST";
	DocTypeList.AddEntry(s);
	s = "MEDLINE";
	DocTypeList.AddEntry(s);
	s = "FILMLINE";
	DocTypeList.AddEntry(s);
	s = "MEMODOC";
	DocTypeList.AddEntry(s);
	s = "SGMLNORM";
	DocTypeList.AddEntry(s);
	s = "HTML";
	DocTypeList.AddEntry(s);
	s = "ONELINE";
	DocTypeList.AddEntry(s);
	s = "PARA";
	DocTypeList.AddEntry(s);
	s = "FILENAME";
	DocTypeList.AddEntry(s);
	s = "FTP";
	DocTypeList.AddEntry(s);
	s = "EMACSINFO";
	DocTypeList.AddEntry(s);
	s = "GOPHER";
	DocTypeList.AddEntry(s);
	s = "BIBTEX";
	DocTypeList.AddEntry(s);
	s = "USMARC";
	DocTypeList.AddEntry(s);
	s = "ANZLIC";
	DocTypeList.AddEntry(s);
	s = "DIF";
	DocTypeList.AddEntry(s);
	s = "GILS";
	DocTypeList.AddEntry(s);
	s = "FGDC";
	DocTypeList.AddEntry(s);
	s = "FGDCSITE";
	DocTypeList.AddEntry(s);
	s = "MARCDUMP";
	DocTypeList.AddEntry(s);
	s = "GILSXML";
	DocTypeList.AddEntry(s);
	s = "CIPP";
	DocTypeList.AddEntry(s);
	s = "EOS_GUIDE";
	DocTypeList.AddEntry(s);
	s = "HTMLTAG";
	DocTypeList.AddEntry(s);
	s = "SOIF";
	DocTypeList.AddEntry(s);
	s = "ANZMETA";
	DocTypeList.AddEntry(s);
	s = "TAGLIST";
	DocTypeList.AddEntry(s);
	s = "CIPC";
	DocTypeList.AddEntry(s);
	*StringListBuffer = DocTypeList;
}

DTREG::~DTREG() {
	delete DtDocType;
	if (DtSIMPLE) {
		delete DtSIMPLE;
	}
	if (DtMARKDOWN) {
		delete DtMARKDOWN;
	}
	if (DtSGMLTAG) {
		delete DtSGMLTAG;
	}
	if (DtFIRSTLINE) {
		delete DtFIRSTLINE;
	}
	if (DtCOLONDOC) {
		delete DtCOLONDOC;
	}
	if (DtIAFADOC) {
		delete DtIAFADOC;
	}
	if (DtMAILFOLDER) {
		delete DtMAILFOLDER;
	}
	if (DtREFERBIB) {
		delete DtREFERBIB;
	}
	if (DtIRLIST) {
		delete DtIRLIST;
	}
	if (DtLISTDIGEST) {
		delete DtLISTDIGEST;
	}
	if (DtMAILDIGEST) {
		delete DtMAILDIGEST;
	}
	if (DtMEDLINE) {
		delete DtMEDLINE;
	}
	if (DtFILMLINE) {
		delete DtFILMLINE;
	}
	if (DtMEMODOC) {
		delete DtMEMODOC;
	}
	if (DtSGMLNORM) {
		delete DtSGMLNORM;
	}
	if (DtHTML) {
		delete DtHTML;
	}
	if (DtONELINE) {
		delete DtONELINE;
	}
	if (DtPARA) {
		delete DtPARA;
	}
	if (DtFILENAME) {
		delete DtFILENAME;
	}
	if (DtFTP) {
		delete DtFTP;
	}
	if (DtEMACSINFO) {
		delete DtEMACSINFO;
	}
	if (DtGOPHER) {
		delete DtGOPHER;
	}
	if (DtBIBTEX) {
		delete DtBIBTEX;
	}
	if (DtUSMARC) {
		delete DtUSMARC;
	}
	if (DtANZLIC) {
		delete DtANZLIC;
	}
	if (DtDIF) {
		delete DtDIF;
	}
	if (DtGILS) {
		delete DtGILS;
	}
	if (DtFGDC) {
		delete DtFGDC;
	}
	if (DtFGDCSITE) {
		delete DtFGDCSITE;
	}
	if (DtMARCDUMP) {
		delete DtMARCDUMP;
	}
	if (DtGILSXML) {
		delete DtGILSXML;
	}
	if (DtCIPP) {
		delete DtCIPP;
	}
	if (DtEOS_GUIDE) {
		delete DtEOS_GUIDE;
	}
	if (DtHTMLTAG) {
		delete DtHTMLTAG;
	}
	if (DtSOIF) {
		delete DtSOIF;
	}
	if (DtANZMETA) {
		delete DtANZMETA;
	}
	if (DtTAGLIST) {
		delete DtTAGLIST;
	}
	if (DtCIPC) {
		delete DtCIPC;
	}
}
