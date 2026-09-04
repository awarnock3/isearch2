// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		firstline.hxx
Version:	1.00
Description:	Class FIRSTLINE - Text Document Type, first line is headline
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/
#ifndef FIRSTLINE_HXX
// BUGFIX #1 (docs/BUG_CATALOG.md#doctypefirstlinecxx): this include
// guard was missing its #define, unlike every other header in this
// tree (`#ifndef X_HXX` / `#define X_HXX`) -- FIRSTLINE_HXX was never
// actually set, so a second #include of this header in the same
// translation unit would re-process the whole file and redefine class
// FIRSTLINE. Purely additive (no declared signature changes), so not
// subject to the GENERAL step 4 header freeze.
#define FIRSTLINE_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

// Indexes a text file with its first line (up to the first '\r' or
// '\n', or EOF) treated as a single "Headline" field. Everything else
// in the file is not otherwise field-parsed.
class FIRSTLINE : public DOCTYPE {
public:
	FIRSTLINE(PIDBOBJ DbParent);
	virtual void ParseFields(PRECORD NewRecord);
	virtual ~FIRSTLINE();
};

typedef FIRSTLINE* PFIRSTLINE;

#endif
