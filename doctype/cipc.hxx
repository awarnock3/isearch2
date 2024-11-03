/*-@@@
File:		cip-collection.hxx
Version:	1.00
Description:	class CIPC - NASA/CIP Collection Metadata Document Type
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and NASA
@@@-*/

#ifndef CIPC_HXX
#define CIPC_HXX

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

#define CIPC_ACCEPT_EMPTY_TAGS	0	/* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */

#define MAXNESTINGLEN 1024

#define CIPC_SGML_EXTENSION "cip"

class CIPC
  : public SGMLNORM
{
public:
  CIPC (PIDBOBJ DbParent);
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

   ~CIPC ();

/* SGML helper functions */
  PCHR       *parse_tags (CHR *b, GPTYPE len) const;
  const CHR  *find_end_tag (char **t, const char *tag) const;
  void        store_attributes (DFT *pdft, CHR *base_ptr, CHR *tag_ptr) const;

private:
  virtual GDT_BOOLEAN UsefulSearchField(const STRING& Field);
  void        ParseExtent(const CHR* Buffer, DOUBLE* extent);


};
typedef CIPC *PCIPC;


class CIPC_Element 
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
typedef CIPC_Element *PCIPC_Element;

#endif
