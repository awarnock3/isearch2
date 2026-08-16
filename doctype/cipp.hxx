// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		cip-product.hxx
Version:	1.00
Description:	class CIPP - NASA/CIP Product Metadata Document Type
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and NASA
@@@-*/

#ifndef CIPP_HXX
#define CIPP_HXX

#ifndef SGMLNORM_HXX
# include "sgmlnorm.hxx"
#endif

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

#define CIPP_ACCEPT_EMPTY_TAGS	0	/* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */

#define MAXNESTINGLEN 1024

#define CIPP_SGML_EXTENSION "cip"

// A NASA/CIP (Collection Information Product) SGML-tagged product
// metadata DOCTYPE -- CIPC's sibling (product-level metadata instead
// of collection-level), built the same way on SGMLNORM's tag-based
// parsing and sharing its ParseFields() shape almost verbatim,
// including the nested-tag nesting stack (Nested, of CIP_Element).
// See doctype/cipc.cxx's class-level comment and doctype/cipp.cxx for
// the exact grammar. Note: store_attributes() below is declared but
// never defined here -- ParseFields() always calls
// SGMLNORM::store_attributes() explicitly instead, so this
// declaration is inert (never ODR-used).
class CIPP
  : public SGMLNORM
{
public:
  CIPP (PIDBOBJ DbParent);
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

   ~CIPP ();

/* SGML helper functions */
  PCHR       *parse_tags (CHR *b, GPTYPE len) const;
  const CHR  *find_end_tag (char **t, const char *tag) const;
  void        store_attributes (DFT *pdft, CHR *base_ptr, CHR *tag_ptr) const;

private:
  virtual GDT_BOOLEAN UsefulSearchField(const STRING& Field);
  void        ParseExtent(const CHR* Buffer, DOUBLE* extent);


};
typedef CIPP *PCIPP;


class CIP_Element 
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
typedef CIP_Element *PCIP_Element;

#endif
