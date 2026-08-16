// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		sgmlgils.hxx
Version:	1.00
Description:	Class SGMLGILS
Author:		Kevin Gamiel
Copyright:	CNIDR
@@@-*/

#ifndef SGMLGILS_HXX
#define SGMLGILS_HXX

#include "sgmlnorm.hxx"

// A GILS SGML DOCTYPE that dispatches Present() by RecordSyntax OID
// (SUTRS or GRS1) rather than by name, unlike every other RecordSyntax-
// aware DOCTYPE in this tree (which compare against the plain-name
// constants, e.g. SutrsRecordSyntax). GetGRS1Record() is a minimal
// placeholder, matching this class's own GetSUTRSRecord()'s "F"/"G"
// element sets -- GRS1 (Generic Record Syntax 1) formatting was never
// implemented here.
class SGMLGILS:public SGMLNORM {
  public:
  SGMLGILS (PIDBOBJ DbParent);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		const STRING & RecordSyntax, PSTRING StringBuffer);
  void GetSUTRSRecord (const RESULT & ResultRecord, const STRING & ElementSet,
		PSTRING StringBuffer);
  // BUGFIX #3 (docs/BUG_CATALOG.md#doctypeincomingsgmlgilscxx): added --
  // Present() already called this (undeclared, uncompilable) function.
  // Purely additive; this header is never included by any other file
  // in the tree (confirmed via grep), so there is no ripple risk.
  void GetGRS1Record (const RESULT & ResultRecord, const STRING & ElementSet,
		PSTRING StringBuffer);

  ~SGMLGILS ();
};

typedef SGMLGILS *PSGMLGILS;

#endif
