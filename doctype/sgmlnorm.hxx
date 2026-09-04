/*-@@@
File:		sgmlnorm.hxx
Version:	1.00
Description:	Class SGMLNORM - Normalized SGML Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef SGMLNORM_HXX
#define SGMLNORM_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A generic SGML/XML-tag-shaped DOCTYPE: any <tag>value</tag> pair
// becomes one field named after the (UnifiedName-mapped) tag, and any
// bare attribute on a start tag (<tag attr="val">) becomes a field
// named "tag@attr" (see store_attributes()). Base class for the
// tag-parsing DOCTYPEs (e.g. SGMLTAG, HTML) that share this machinery.
class SGMLNORM:public DOCTYPE
{
public:
  SGMLNORM (PIDBOBJ DbParent);
  void LoadFieldTable() { return; };
  //  void AddFieldDefs ();
  void ParseRecords (const RECORD & FileRecord);
  void ParseFields (PRECORD NewRecord);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		STRING *StringBuffer);

  ~SGMLNORM ();
  // hooks into the guts of the field parser
  virtual const CHR* UnifiedName (const CHR *tag) const; // for children to play with

  /* SGML helper functions */
  // Splits b (len bytes) in place into a nullptr-terminated array of
  // pointers to each tag's contents (the '>' bytes are overwritten
  // with '\0' to terminate each tag string); returns nullptr if a tag
  // is left unterminated (no closing '>') or on allocation failure.
  // Caller owns the returned array (delete[] it; the CHR* elements
  // point into b, not separately allocated).
  PCHR *parse_tags (CHR *b, GPTYPE len) const;
  // Scans the nullptr-terminated tag list t for a "/"+tag closing tag
  // matching tag's name (up to its first space), case-insensitively.
  // Returns a pointer to the matching entry in t, or nullptr if none.
  const CHR* find_end_tag (char *const *t, const char *tag) const;
  // Parses a start tag's "attr=val ..." content (base_ptr is the
  // record buffer parse_tags() sliced tag_ptr from, needed to compute
  // field-coordinate offsets) into one field per attribute, named
  // "tagname@attrname".
  void store_attributes (PDFT pdft, CHR *base_ptr, CHR *tag_ptr) const;

};

typedef SGMLNORM *PSGMLNORM;

#endif
