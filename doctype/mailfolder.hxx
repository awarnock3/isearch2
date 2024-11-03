/*-@@@
File:		mailfolder.hxx
Version:	1.00
Description:	Class MAILFOLDER - Mail Folder Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MAILFOLDER_HXX
#define MAILFOLDER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

class MAILFOLDER 
  :  public DOCTYPE {
public:
    MAILFOLDER(PIDBOBJ DbParent);
    void AddFieldDefs();
    void ParseRecords(const RECORD& FileRecord);
    void ParseFields(PRECORD NewRecord);
    void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING *StringBuffer);
    ~MAILFOLDER();
    // To Manipulate the "acceptable" mail fields
    GDT_BOOLEAN accept_tag(const CHR *tag) const;
    // Utility functions
    GDT_BOOLEAN IsMailFromLine(const char *line) const;
    GDT_BOOLEAN IsNewsLine(const char *line) const;
    CHR *NameKey(CHR *buf, GDT_BOOLEAN name = GDT_TRUE) const;

private:
    PCHR *parse_tags(CHR *b, GPTYPE len) const;
};

typedef MAILFOLDER* PMAILFOLDER;

#endif
