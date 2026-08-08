// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*
 * Based on: doctype/html.cxx
 */

/*-@@@
File:		litmed.hxx
Version:	1.03
Description:	Class LITMED - WWW LITMED Document Type
Author:		Edward C. Zimmermann, edz@bsn.com/Roy Smith, roy@nyu.edu
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef LITMED_HXX
#define LITMED_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmlnorm.hxx"

/*
 * 0==> Accept most tags
 * 1==> Accept only certain tags
 */
// Unlike doctype/html.hxx's equivalent STRICT_HTML (guarded with
// #ifndef so it can be overridden by a compiler flag), this is an
// unconditional #define -- any -DSTRICT_LITMED=0 on the command line
// gets silently redefined back to 1 by this line, so LITMED::
// ParseFields()'s #else branch (IgnoreLITMEDTag()) is permanently
// unreachable in practice, same as doctype/html.cxx's own genuinely-
// dead `#if 1` branch. Left as its own historical curiosity rather
// than "fixed" into an #ifndef, since making the #else branch
// actually reachable would be a behavior change with no clear
// specification for what the #else path should accept.
#define STRICT_LITMED	1

// An SGML/HTML-tagged LITMED (medical literature) DOCTYPE, adapted
// from doctype/html.cxx -- shares its architecture (tag scanning via
// the inherited SGMLNORM::parse_tags()/find_end_tag(), an allowlist of
// recognized field tags gated by STRICT_LITMED) but not its
// "minimized tag" <DD>/<DT>/<LI>/<TL> fallback handling, which was
// never carried over.
class LITMED:public SGMLNORM
{
  public:
  LITMED (PIDBOBJ DbParent);
  void ParseRecords (const RECORD & FileRecord);
  void ParseFields (PRECORD NewRecord);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		PSTRING StringBuffer);
   ~LITMED ();
};
typedef LITMED *PLITMED;

#endif
