
/*** Local Configurations for BSn doctypes ****/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// BUGFIX #1: this file had no include guard at all, unlike every other
// .hxx in the tree -- harmless today only because doctype/mailfolder.cxx
// (its sole current includer) includes it exactly once, but a latent
// double-inclusion hazard for any future includer.
#ifndef DOC_CONF_HXX
#define DOC_CONF_HXX

// Most of this file's macros (STRICT_HTML, RESTRICT_MAIL_FIELDS,
// SHOW_MAIL_DATE, USE_UNIFIED_NAMES and its per-doctype overrides) are
// currently unused: mailfolder.cxx, the only file that #includes this
// header, only actually relies on BSN_EXTENSIONS/BRIEF_MAGIC below.
// Every other doctype/*.cxx that references USE_UNIFIED_NAMES or a
// per-doctype override (referbib.cxx, medline.cxx, filmline.cxx, ...)
// defines its own local fallback instead of including this header.

// HTML
#define STRICT_HTML     	1 /* 0 ==> Accept Anything, 1==> accept only "known" tags */

// Mail
#define RESTRICT_MAIL_FIELDS	1 /* 0 ==> Accept anything, 1==> only those in list */
#define SHOW_MAIL_DATE   	0 /* 0 ==> Headline "From: Subject" 1==> "From dd/mm/yy: Subject" */ 

// Bibliographic Formats

// Use Native or Unified names by default?
// BUGFIX #2: this was defined a second time, identically, further down
// under a second "// General" comment -- a harmless (same-value)
// redefinition, but a clear copy-paste duplicate. Removed the second one.
#define USE_UNIFIED_NAMES 1

// Define Below to individualy modify above behaviour
//#define REFER_UNIFIED_NAMES 1
//#define MEDLINE_UNIFIED_NAMES 1
//#define FILMLINE_UNIFIED_NAMES 1

// --------- End User Configurable Options

// Specific
#ifndef REFER_UNIFIED_NAMES
#define REFER_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif
#ifndef MEDLINE_UNIFIED_NAMES
#define MEDLINE_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif
#ifndef FILMLINE_UNIFIED_NAMES
#define FILMLINE_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif

// General
#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0 ==> CNIDR's Isearch, 1==> BSn's version */
#endif
#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* CNIDR "hardwires" this */
#endif
// Not fixed: if BSN_EXTENSIONS were ever set to >=1, BRIEF_MAGIC would
// never get defined here at all. Not unique to this file -- every
// doctype/*.cxx and doctype/*.hxx with its own local BSN_EXTENSIONS
// fallback (anzlic.hxx, cipc.hxx, cipp.hxx, fgdc.hxx, html.hxx, ...)
// has this exact same shape, and BSN_EXTENSIONS is never actually set
// to 1 anywhere in this tree today. A tree-wide incomplete-BSn-mode
// design choice, not a bug specific to this file.

#endif // DOC_CONF_HXX
