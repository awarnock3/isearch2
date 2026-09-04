// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// $Id: anzmeta.hxx,v 1.2 2000/10/11 14:02:16 cnidr Exp $
/*-@@@
File:		anzmeta.hxx
Description:	class ANZMETA - adaptation of FGDC Document Type

Author:         David Crossley <crossley@indexgeo.com.au>
IG_version:     $Revision: 1.2 $

Previous:       fgdc.hxx 1.00
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and USGS/FGDC
@@@-*/

#ifndef ANZMETA_HXX
#define ANZMETA_HXX

#ifndef SGMLNORM_HXX
# include "sgmlnorm.hxx"
#endif

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

#ifndef ANZ_ACCEPT_EMPTY_TAGS
#define ANZ_ACCEPT_EMPTY_TAGS	0	/* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */
#endif

#define MAXNESTINGLEN 1024

#define ANZ_SGML_EXTENSION "sgml"
#define ANZ_XML_EXTENSION  "xml"
#define ANZ_HTML_EXTENSION "html"
#define ANZ_TEXT_EXTENSION "txt"

#define SHORT_ANZ_SGML_EXTENSION "sgm"
#define SHORT_ANZ_HTML_EXTENSION "htm"
#define SHORT_ANZ_TEXT_EXTENSION "txt"

#define ANZ_SGML_EXTENSION_UC "SGML"
#define ANZ_XML_EXTENSION_UC  "XML"
#define ANZ_HTML_EXTENSION_UC "HTML"
#define ANZ_TEXT_EXTENSION_UC "TEXT"

#define SHORT_ANZ_SGML_EXTENSION_UC "SGM"
#define SHORT_ANZ_HTML_EXTENSION_UC "HTM"
#define SHORT_ANZ_TEXT_EXTENSION_UC "TXT"

class ANZMETA
  : public SGMLNORM
{
public:
  ANZMETA (PIDBOBJ DbParent);
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
   ~ANZMETA ();

/* SGML helper functions */
  PCHR       *parse_tags (CHR *b, GPTYPE len) const;
  const CHR  *find_end_tag (char **t, const char *tag) const;
  void        store_attributes (DFT *pdft, CHR *base_ptr, CHR *tag_ptr) const;

private:
  virtual GDT_BOOLEAN UsefulSearchField(const STRING& Field);
  void        ParseExtent(const CHR* Buffer, DOUBLE* extent);


};
typedef ANZMETA *PANZMETA;

class ZMD_Element 
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
typedef ZMD_Element *PZMD_Element;

#endif
