/*-@@@
File:		anzlic.hxx
Version:	1.00
Description:	class ANZLIC - ANZLIC Document Type
Author:		Archie Warnock, warnock@clark.net
		Adapted from HTML Class
Original:	Edward C. Zimmermann, edz@bsn.com
Copyright:	A/WWW Enterprises, MCNC/CNIDR and USGS/FGDC
@@@-*/

#ifndef ANZLIC_HXX
#define ANZLIC_HXX

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

#define ANZLIC_ACCEPT_EMPTY_TAGS	0	/* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */

#define MAXNESTINGLEN 1024
//#define SUTRS_OID "1.2.840.10003.5.101"
//#define USMARC_OID "1.2.840.10003.5.10"
//#define NEW_HTML_OID "1.2.840.10003.5.108"
//#define NEW2_HTML_OID "1.2.840.10003.5.109.3"
//#define HTML_OID "1.2.840.10003.5.1000.34.1"
//#define SGML_OID "1.2.840.10003.5.1000.34.2"
//#define XML_OID "1.2.840.10003.5.109.10"

#define ANZLIC_SGML_EXTENSION "sgml"
#define ANZLIC_XML_EXTENSION "xml"
#define ANZLIC_HTML_EXTENSION "html"
#define ANZLIC_TEXT_EXTENSION "txt"

#define SHORT_ANZLIC_SGML_EXTENSION "sgm"
#define SHORT_ANZLIC_HTML_EXTENSION "htm"
#define SHORT_ANZLIC_TEXT_EXTENSION "txt"

#define ANZLIC_SGML_EXTENSION_UC "SGML"
#define ANZLIC_XML_EXTENSION_UC  "XML"
#define ANZLIC_HTML_EXTENSION_UC "HTML"
#define ANZLIC_TEXT_EXTENSION_UC "TXT"

#define SHORT_ANZLIC_SGML_EXTENSION_UC "SGM"
#define SHORT_ANZLIC_HTML_EXTENSION_UC "HTM"
#define SHORT_ANZLIC_TEXT_EXTENSION_UC "TXT"

class ANZLIC:public SGMLNORM
{
public:
  ANZLIC (PIDBOBJ DbParent);
  void LoadFieldTable();
  void ParseRecords (const RECORD & FileRecord);
  void ParseFields (PRECORD NewRecord);
  GDT_BOOLEAN GetCleanedFieldData(const RESULT& ResultRecord, 
				  const STRING& FieldName,
				  const STRING& FieldType,
				  STRING& Buffer);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		const STRING & RecordSuntax, PSTRING StringBuffer);
//  LONG ParseDateSingle(const PCHR Buffer);
  DOUBLE ParseDateSingle(const PCHR Buffer);
  void ParseDateRange(const PCHR Buffer, DOUBLE* fStart, 
		      DOUBLE* fEnd);
//  void ParseGPoly(const PCHR Buffer);
   ~ANZLIC ();
/* SGML helper functions */
   PCHR *parse_tags (PCHR b, GPTYPE len) const;
  //   const PCHR find_end_tag (const char *const *t, const char *tag) const;
   const PCHR find_end_tag (char **t, const char *tag) const;
   void store_attributes (PDFT pdft, PCHR base_ptr, PCHR tag_ptr) const;
private:
  GDT_BOOLEAN UsefulSearchField(const STRING& Field);


};
typedef ANZLIC *PANZLIC;

class AMD_Element 
{
  public:
  void set_tag(const STRING NewTag) { tag = NewTag; }
  STRING& get_tag() { return tag; }
  void set_start(const INT NewStart) { tag_start = NewStart; }
  INT get_start() { return tag_start; }
  void set_end(const INT NewEnd) { tag_end = NewEnd; }
  INT get_end() { return tag_end; }

  private:
  STRING tag;
  INT tag_start, tag_end;
};
typedef AMD_Element *PAMD_Element;

#endif
