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


/**
 * @brief A GCMD/DIF (Directory Interchange Format) DOCTYPE: "Fieldname:
 * value" / "Group: name ... End_Group" text, parsed by a small
 * hand-written recursive-descent parser (start()/atom()/field()/
 * group()/... below, implementing the grammar in the file-level doc
 * comment in dif.cxx) driven by a character-at-a-time scanner
 * (sgetc()/sungetc()/tell()) over RecBuffer. See dif.cxx for the
 * grammar and RecBufferLen's role in keeping the scanner in bounds.
 */
class DIF : public COLONDOC {
public:

  /*
   * Original Methods
   *
   */
  /// Constructs a DIF handler for database @p DbParent.
  DIF(PIDBOBJ DbParent);
  /// Loads the FIELDTYPE file (`-o fieldtype=<filename>`) into Db->FieldTypes.
  void LoadFieldTable();
  /// Fetches a field's data with embedded newlines/CRs stripped.
  GDT_BOOLEAN GetCleanedFieldData(const RESULT& ResultRecord,
				  const STRING& FieldName,
				  const STRING& FieldType,
				  STRING& Buffer);
  /// Parses a GCMD-style coordinate string ("34.5N") to signed decimal degrees.
  DOUBLE ParseNumeric(const CHR *Buffer);
  /// Parses a single DIF date value into both @p fStart and @p fEnd.
  void ParseDate(const CHR *Buffer, DOUBLE* fStart, DOUBLE* fEnd);
  /// Parses a single date to DIF's own sortable sentinel convention.
  DOUBLE ParseDateSingle(const CHR *Buffer);
  /// Parses a record's fields via the recursive-descent parser and attaches the resulting DFT.
  void ParseFields(PRECORD NewRecord);
  /// Parses a DIF START_DATE/STOP_DATE range into @p fStart / @p fEnd.
  void ParseDateRange(const CHR *Buffer, DOUBLE* fStart,
		      DOUBLE* fEnd);
  /// Formats a DIF record for display per the requested element set and record syntax.
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, PSTRING StringBuffer);
  /// Destroys the DIF handler (no owned resources to release).
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
  /// Reads the next byte from RecBuffer, clamped to RecBufferLen.
  long sgetc();
  /// Backs the scanner up one byte (pairs with sgetc()).
  void sungetc();
  /// Returns the scanner's current byte offset into RecBuffer.
  long tell();
  /// Skips spaces, tabs, and newlines.
  void moveNextWord();
  /// Skips spaces and tabs only (newlines are significant).
  void skipWhitespace();
  /// Reads the rest of the current line into `token`, honoring "&\n" line continuations.
  void readNewLine();
  /// Reads one whitespace-delimited word into `token`.
  void readWord();
  /// Declared but never defined or called anywhere in this file; dead API surface.
  void eofError(char *s);
  /// Declared but never defined or called anywhere in this file; dead API surface.
  void printError(char *msg);
  //  int nextToken();
  /// Retrieves the next token per the scanner's DFA; stores its text in `token`.
  enum TokenType nextToken();

  /*
   * Parser Methods
   *
   */
  /// Grammar entry point: `[START] --> [ATOM] [ATOMTAIL] | lambda`.
  void start();
  /// Grammar rule `[ATOM] --> [GROUP] | [FIELD]`.
  void atom();
  /// Grammar rule `[ATOMTAIL] --> [ATOM] [ATOMTAIL] | lambda`.
  void atomtail();
  /// Grammar rule `[FIELD] --> FieldType TextType`.
  void field();
  /// Grammar rule `[GROUP] --> GroupType FieldWithoutColon [GROUPBODY] EndGroupType`.
  void group();
  /// Grammar rule `[GROUPBODY] --> [ATOMTAIL] | [ML]`.
  void groupbody();
  /// Grammar rule `[ML] --> textMLType [ML] | EndGroupType`.
  void textML();
  /// Reports a grammar-rule mismatch (non-fatal; parsing continues).
  void parserError(const char *);
  /// Records one parsed field's normalized name and [start,stop] value range.
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
