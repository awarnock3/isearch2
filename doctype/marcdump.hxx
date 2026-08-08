// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		marcdump.hxx
Version:	$Revision: 1.1 $
Description:	Class MARCDUMP - output from Yaz utility marcdump
Author:		Archibald Warnock (warnock@clark.net), A/WWW Enterprises
Copyright:	A/WWW Enterprises, Columbia, MD
@@@-*/

#ifndef MARCDUMP_HXX
#define MARCDUMP_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

// A DOCTYPE for MARC records as dumped by the Yaz `marcdump` utility:
// one record per run of lines starting at a line whose tag is "001",
// continuing until the next "001" line or end of file. ParseFields()
// then splits each record into tag/value pairs (see parse_tags() in
// the source) and maps MARC field numbers to named fields via
// UnifiedName()/marcdumpFieldNumToName().
class MARCDUMP
  : public DOCTYPE {
public:
    MARCDUMP(PIDBOBJ DbParent);
    void AddFieldDefs();
    CHR* UnifiedName (CHR *tag);
    void ParseRecords(const RECORD& FileRecord);
    void ParseFields(RECORD *NewRecord);

    void BeforeSearching(SQUERY* SearchQueryPtr);

    void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 STRING *StringBuffer);
    void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING* StringBufferPtr);
    void PresentSutrs(const RESULT& ResultRecord, 
		      const STRING& ElementSet,
		      STRING *StringBufferPtr);
    void PresentHtml(const RESULT& ResultRecord, 
		     const STRING& ElementSet, 
		     STRING *StringBufferPtr);

    ~MARCDUMP();

};
typedef MARCDUMP* PMARCDUMP;

#endif
