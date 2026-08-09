// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*

File:        usmarc.hxx
Version:     1
Description: class USMARC - MARC records for library use
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef USMARC_HXX
#define USMARC_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

typedef struct mde {
  char field[4];
  char subfield[3]; // Zeros if it has no subfields
  char length[5];
  char offset[6];
} marc_dir_entry;

// The current record's raw bytes and parsed MARC directory, shared as
// globals (not USMARC instance members) between ParseFields() -- which
// allocates and populates them via readMarcStructure() -- and the
// ParseWords() call that follows for the same record, which is
// responsible for freeing them. See doctype/usmarc.cxx's BUGFIX #3 for
// why every allocation site frees any prior value first and every free
// site nulls the pointer afterward.
extern CHR *RecBuffer;
extern GPTYPE marcNumDirEntries;
extern GPTYPE marcRecordLength;
extern GPTYPE marcBaseAddr;

extern marc_dir_entry *marcDir;

// A DOCTYPE for USMARC/MARC21 library records: ParseRecords() splits a
// batch file into one record per length-prefixed MARC record;
// ParseFields() reads the directory (see readMarcStructure()) and
// indexes each field both by its raw MARC tag (e.g. "245") and, via
// ParseData[]'s field/subfield/tag-letter table, under a friendlier
// name (e.g. "title"); ParseWords() then restricts word-position
// extraction to those same field ranges.
class USMARC
  : public DOCTYPE
{
public:
  USMARC(PIDBOBJ DbParent);
  void   ParseRecords(const RECORD& FileRecord);   
  void   ParseFields(PRECORD NewRecord);
  GPTYPE ParseWords(CHR* DataBuffer, INT DataLength, INT DataOffset, 
		    GPTYPE* GpBuffer, INT GpLength);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 STRING* StringBuffer);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING* StringBuffer);
  ~USMARC();
private:
  void addSearchEntry(PDFT pdft, STRING fieldName, int fieldStart, int fieldEnd);
  void readFileContents(PRECORD NewRecord);
  int readRecordLength(void);
  int readBaseAddr(void);
  void readMarcStructure(PRECORD NewRecord);
  int usefulMarcField(const char *fieldStr);
  int compareReg(const char *s1, const char *s2);
  char findNextTag(char *RecBuffer, int &pos, int &tagPos, int &tagLength);
};

typedef USMARC* PUSMARC;

#endif
