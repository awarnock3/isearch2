/*-@@@
File:		mailfolder.hxx
Version:	1.00
Description:	Class MAILFOLDER - Mail Folder Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef MAILFOLDER_HXX
#define MAILFOLDER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A Unix mail-folder (mbox-style) DOCTYPE: splits a folder into
// per-message records at blank-line-then-"From "/"Article " boundaries
// (ParseRecords()), then parses each message's RFC822-ish headers into
// fields plus a "Message-body" field for everything after the blank
// line separating headers from the body (ParseFields()). See the
// format comment above accept_tag() in mailfolder.cxx for the full
// grammar and known limitations (MIME/encodings not handled).
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
