/*-@@@
File:		sgmlnorm.hxx
Version:	1.00
Description:	Class SGMLNORM - Normalized SGML Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef SGMLNORM_HXX
#define SGMLNORM_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

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
  PCHR *parse_tags (CHR *b, GPTYPE len) const;
  const CHR* find_end_tag (char *const *t, const char *tag) const;
  void store_attributes (PDFT pdft, CHR *base_ptr, CHR *tag_ptr) const;

};

typedef SGMLNORM *PSGMLNORM;

#endif
