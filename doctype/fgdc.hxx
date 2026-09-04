// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		fgdc.hxx
Version:	1.00
Description:	class FGDC - FGDC Document Type
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and USGS/FGDC
@@@-*/

#ifndef FGDC_HXX
#define FGDC_HXX

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

#define FGDC_ACCEPT_EMPTY_TAGS	0	/* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */

#define MAXNESTINGLEN 1024

#define FGDC_SGML_EXTENSION "sgml"
#define FGDC_HTML_EXTENSION "html"
#define FGDC_TEXT_EXTENSION "text"
#define FGDC_XML_EXTENSION  "xml"

#define SHORT_FGDC_SGML_EXTENSION "sgm"
#define SHORT_FGDC_HTML_EXTENSION "htm"
#define SHORT_FGDC_TEXT_EXTENSION "txt"
#define SHORT_FGDC_XML_EXTENSION  "xml"

#define FGDC_SGML_EXTENSION_UC "SGML"
#define FGDC_HTML_EXTENSION_UC "HTML"
#define FGDC_TEXT_EXTENSION_UC "TEXT"
#define FGDC_XML_EXTENSION_UC  "XML"

#define SHORT_FGDC_SGML_EXTENSION_UC "SGM"
#define SHORT_FGDC_HTML_EXTENSION_UC "HTM"
#define SHORT_FGDC_TEXT_EXTENSION_UC "TXT"
#define SHORT_FGDC_XML_EXTENSION_UC  "XML"

// A USGS/FGDC (Federal Geographic Data Committee) SGML-tagged metadata
// DOCTYPE, built on SGMLNORM's tag-based parsing -- the third sibling
// in a family with doctype/cipc.cxx and doctype/cipp.cxx (same
// ParseFields()/LoadFieldTable()/ParseDate()/ParseDateRange()/
// parse_tags()/find_end_tag() structure and bugs; see doctype/cipc.hxx
// for the shared architecture description and doctype/fgdc.cxx's
// BUGFIX comments for the cross-references). ParseFields() walks
// SGML-style <tag>value</tag> pairs (nesting handled via the Nested
// stack of MD_Element), indexing both a short field name and a full,
// underscore-joined nested field-path name for each recognized tag.
// Also home to the live, canonical GetNumericValue() that
// doctype/cipc.cxx and doctype/cipp.cxx both rely on via their own
// commented-out copies + extern declarations.
class FGDC
  : public SGMLNORM
{
public:
  FGDC (PIDBOBJ DbParent);
  void        LoadFieldTable();
  void        ParseRecords (const RECORD& FileRecord);
  void        ParseFields (RECORD *NewRecord);
  GDT_BOOLEAN GetCleanedFieldData(const RESULT& ResultRecord, 
				  const STRING& FieldName,
				  const STRING& FieldType,
				  STRING& Buffer);
  void        Present (const RESULT& ResultRecord, 
		       const STRING & ElementSet,
		       const STRING& RecordSuntax, 
		       STRING *StringBuffer);
  void        ParseDate(const CHR *Buffer, DOUBLE* fStart, DOUBLE* fEnd);
  void        ParseDate(const STRING& Buffer, DOUBLE* fStart, DOUBLE* fEnd);
  void        ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
			     DOUBLE* fEnd);
  void        ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
			     DOUBLE* fEnd);
  INT         ParseGPoly(const CHR *Buffer, DOUBLE Vertices[]);

  DOUBLE      ParseComputed(const STRING& FieldName, const CHR *Buffer);
   ~FGDC ();

/* SGML helper functions */
  PCHR       *parse_tags (CHR *b, GPTYPE len) const;
  const CHR  *find_end_tag (char **t, const char *tag) const;
  void        store_attributes (DFT *pdft, CHR *base_ptr, CHR *tag_ptr) const;

private:
  virtual GDT_BOOLEAN UsefulSearchField(const STRING& Field);
  void        ParseExtent(const CHR* Buffer, DOUBLE* extent);


};
typedef FGDC *PFGDC;


class MD_Element 
{
public:
  void    set_tag(const STRING NewTag)  { tag = NewTag; }
  STRING& get_tag()                     { return tag; }
  void    set_start(const INT NewStart) { tag_start = NewStart; }
  INT     get_start()                   { return tag_start; }
  void    set_end(const INT NewEnd)     { tag_end = NewEnd; }
  INT     get_end()                     { return tag_end; }

private:
  STRING  tag;
  INT     tag_start, tag_end;
};
typedef MD_Element *PMD_Element;

#endif
