/*@@@
File:		stopword.hxx
Version:	1.00
Description:	Class STOPWORD - Stop word list
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef STOPWORD_HXX
#define STOPWORD_HXX

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "defs.hxx"
#include "string.hxx"
#include "strlist.hxx"

const INT MaxStopWordLength = 16;	// Should be moved to defs.hxx, ultimately should be configurable.
					// Note that actual maximum word length is MaxStopWordLength - 1.

// A stop word list backed by a flat fixed-record (MaxStopWordLength
// bytes each) memory block, loaded from and flushed back to a single
// file. Unrelated to IDBOBJ::IsStopWord() (a different, pure-virtual
// per-doctype mechanism) despite the similar name -- this class has no
// callers anywhere in the current tree. See stopword.cxx for details.
//@ManMemo: Stop word list class.
class STOPWORD {
public:
	//@ManMemo: Constructor must be called with the name of a file to map for the life of the object.
	STOPWORD(const STRING& FileName);
	//@ManMemo: Adds a list of words to the stop word list.
	INT AddWords(const STRLIST& WordList);
	//@ManMemo: Imports a list of words from a text file (one word per line).
	INT ImportFromTextFile(const STRING& FileName);
	//@ManMemo: Returns GDT_TRUE if given word is present in the stop word list.
	GDT_BOOLEAN IsStopWord(const CHR* WordPtr);
	//@ManMemo: Sort stop word list if necessary (generally should not be called externally).
	void EnsureSortedList();
	//@ManMemo: Automatically flushes any changes to disk.
	~STOPWORD();
private:
	//@ManMemo: Resizes the stop word list memory block (internal use only).
	void Resize(const INT NewTotalEntries);
	//@ManMemo: Pointer to the stop word list memory block.
	CHR* BlockPtr;
	//@ManMemo: Number of elements of size MaxStopWordLength in the memory block.
	INT TotalEntries;
	//@ManMemo: GDT_TRUE if this object has changed since creation.
	GDT_BOOLEAN Changed;
	//@ManMemo: GDT_TRUE if the stop word list is currently sorted.
	GDT_BOOLEAN Sorted;
	//@ManMemo: File name of the file to be mapped to this object, changes are flushed to this file.
	STRING SwFileName;
};

#endif

