// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*@@@
File:		dif.hxx
Version:	1.00
$Revision: 1.12 $
Description:	Class DIF - SGML-like Text w/ static present function
Author:		Archie Warnock, warnock@awcubed.com
Revised:        Chris Gokey
@@@*/

#ifndef DIF_HXX
#define DIF_HXX

#if defined(_MSDOS) || defined(_WIN32)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "defs.hxx"
#include "doctype.hxx"
#include "colondoc.hxx"

#define DIF_SGML_EXTENSION "sgm"
#define DIF_HTML_EXTENSION "htm"
#define DIF_TEXT_EXTENSION "sut"


// A GCMD/DIF (Directory Interchange Format) DOCTYPE: "Fieldname:
// value" / "Group: name ... End_Group" text, parsed by a small
// hand-written recursive-descent parser (start()/atom()/field()/
// group()/... below, implementing the grammar in the comment above
// DIF::start() in dif.cxx) driven by a character-at-a-time scanner
// (sgetc()/sungetc()/tell()) over RecBuffer. See dif.cxx for the
// grammar and RecBufferLen's role in keeping the scanner in bounds.
class DIF : public COLONDOC {
public:

  /* 
   * Original Methods
   *
   */
  DIF(PIDBOBJ DbParent);
  void LoadFieldTable();
  GDT_BOOLEAN GetCleanedFieldData(const RESULT& ResultRecord, 
				  const STRING& FieldName,
				  const STRING& FieldType,
				  STRING& Buffer);
  DOUBLE ParseNumeric(const CHR *Buffer);
  void ParseDate(const CHR *Buffer, DOUBLE* fStart, DOUBLE* fEnd);
  DOUBLE ParseDateSingle(const CHR *Buffer);
  void ParseFields(PRECORD NewRecord);
  void ParseDateRange(const CHR *Buffer, DOUBLE* fStart,
		      DOUBLE* fEnd);
  void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
	       const STRING& RecordSyntax, PSTRING StringBuffer);
  ~DIF();

  /* 
   * Constants 
   *
   */  
  enum TokenType { eofType = 0, fieldType = 1, groupType = 2, textMLType = 3,
     textType = 4, fldWOcolonType = 5, endGroupType = 6, errorType = 7 };
 
  /*
   * Variables Needed 
   * 
   */
  char *RecBuffer;
  // Set once in ParseFields() right after RecBuffer is allocated and
  // NUL-terminated; sgetc() (see dif.cxx BUGFIX #1) uses this to keep
  // pos from ever running past RecBuffer's allocated bounds.
  int RecBufferLen;
  STRING groupName;
  STRING token;
  int state;
  int status;
  //  int toktype;
  enum TokenType toktype;
  int pos;
  int count;

  PDFT pdft;

  /*
   * Scanner Methods
   *
   */
  long sgetc();
  void sungetc();
  long tell();
  void moveNextWord();
  void skipWhitespace();
  void readNewLine();
  void readWord();
  void eofError(char *s);
  void printError(char *msg);
  //  int nextToken();
  enum TokenType nextToken();
  
  /*
   * Parser Methods
   *
   */
  void start();
  void atom();
  void atomtail();
  void field();
  void group();
  void groupbody();
  void textML();
  void parserError(const char *);
  void writeField(char *fld, long start, long stop);

  /* 
   * Constants 
   *
   */  
  /*
    extern  const int eofType        ;
    extern  const int fieldType      ;
    extern  const int groupType      ;
    extern  const int textMLType     ;
    extern  const int textType       ;
    extern  const int fldWOcolonType ;
    extern  const int endGroupType   ;
    extern  const int errorType      ;
  */
};

typedef DIF* PDIF;

#endif
