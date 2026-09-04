/*@@@
File:		stopword.cxx
Version:	1.00
Description:	Class STOPWORD - Stop word list
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// No callers anywhere in the current tree, and stopword.o isn't in
// src/Makefile's production OBJ list either -- dead code, though still
// processed per the standard pipeline.

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "isearch.hxx"
#include "stopword.hxx"

STOPWORD::STOPWORD(
		//@ManMemo: Name of file to be mapped to this object.
		const STRING& FileName
		) {
	// Initialize state variables
	Changed = GDT_FALSE;
	Sorted = GDT_TRUE;
	// Try to open the file
	FILE* Fp = fopen(FileName, "rb");
	if (Fp) {
		// Get file size
		fseek(Fp, 0, SEEK_END);
		TotalEntries = ftell(Fp) / MaxStopWordLength;
		fseek(Fp, 0, SEEK_SET);
		BlockPtr = new CHR[TotalEntries * MaxStopWordLength];
		fread(BlockPtr, 1, TotalEntries * MaxStopWordLength, Fp);
		fclose(Fp);
	} else {
		BlockPtr = 0;
		TotalEntries = 0;
	}
	SwFileName = FileName;
}

static int StopwordCompareWords(
		//@ManMemo: First word to compare.
		const void* Word1,
		//@ManMemo: Second word to compare.
		const void* Word2
		) {
	return StrCaseCmp((CHR*)Word1, (CHR*)Word2);
}

INT STOPWORD::AddWords(
		//@ManMemo: List of words to be added to stop word list.
		const STRLIST& WordList
		) {
	INT Total = WordList.GetTotalEntries();
	if (Total > 0) {
		INT x;
		STRING Sw;
		// First remove entries that are already in the list
		STRLIST NewWordList;
		CHR AWord[MaxStopWordLength];
		for (x=1; x<=Total; x++) {
			WordList.GetEntry(x, &Sw);
			Sw.GetCString(AWord, MaxStopWordLength);
			if (IsStopWord(AWord) == GDT_FALSE) {
				NewWordList.AddEntry(Sw);
			}
		}
		// Now add them
		Total = NewWordList.GetTotalEntries();
		INT OldTotalEntries = TotalEntries;
		Resize(TotalEntries + Total);
		for (x=0; x<Total; x++) {
			NewWordList.GetEntry(x + 1, &Sw);
			Sw.GetCString(BlockPtr + (MaxStopWordLength * (OldTotalEntries + x)), MaxStopWordLength);
		}
		Sorted = GDT_FALSE;
		Changed = GDT_TRUE;
	}
	return Total;
}

INT STOPWORD::ImportFromTextFile(
		//@ManMemo: Name of text file to import words from.
		const STRING& FileName
		) {
	STRLIST Strlist;
	FILE* Fp = fopen(FileName, "rb");
	if (Fp) {
		CHR WordBuffer[1024];
		INT n;
		while (fgets(WordBuffer, 1024, Fp)) {
		  //			while (!isalnum(WordBuffer[n=(strlen(WordBuffer)-1)])) {
			// BUGFIX #1 (docs/BUG_CATALOG.md#srcstopwordcxx): the original
			// `while (!IsAlnum(WordBuffer[n=(strlen(WordBuffer)-1)]))`
			// computed strlen()-1 on an unsigned size_t; once the trimming
			// below reduced the line to an empty string (e.g. a blank or
			// all-punctuation line), strlen()-1 underflowed, truncating to
			// n==-1, indexing WordBuffer[-1] out of bounds forever (an
			// infinite loop as well as a stack-buffer-underflow, since
			// WordBuffer[-1]='\0' never becomes alnum). Confirmed with a
			// standalone repro before fixing. Guard on the length instead.
			n = strlen(WordBuffer);
			while (n > 0 && !IsAlnum(WordBuffer[n-1])) {
				WordBuffer[--n] = '\0';
			}
			//			while ( (!isalnum(WordBuffer[0])) && (WordBuffer[0] != '\0') ) {
			// BUGFIX #2: strcpy()'s source and destination overlap here
			// (shifting the buffer left by one byte); the C standard
			// leaves strcpy() undefined behavior for overlapping ranges.
			// Confirmed via a real ASan strcpy-param-overlap report before
			// fixing. memmove() is explicitly overlap-safe.
			while ( (!IsAlnum(WordBuffer[0])) && (WordBuffer[0] != '\0') ) {
				memmove(WordBuffer, WordBuffer + 1, strlen(WordBuffer));
			}
			if (WordBuffer[0] != '\0') {
				Strlist.AddEntry(WordBuffer);
			}
		}
		return AddWords(Strlist);
	} else {
		return 0;
	}
}

void STOPWORD::EnsureSortedList() {
	if (Sorted == GDT_FALSE) {
		if (BlockPtr) {
			qsort(BlockPtr, TotalEntries, MaxStopWordLength, StopwordCompareWords);
		}
		Sorted = GDT_TRUE;
		Changed = GDT_TRUE;
	}
}

GDT_BOOLEAN STOPWORD::IsStopWord(
		//@ManMemo: Word to be searched for in the stop word list.
		const CHR* WordPtr
		) {
	if (BlockPtr) {
		EnsureSortedList();
		if (bsearch(WordPtr, BlockPtr, TotalEntries, MaxStopWordLength, StopwordCompareWords)) {
			return GDT_TRUE;
		} else {
			return GDT_FALSE;
		}
	} else {
		return GDT_FALSE;
	}
}

void STOPWORD::Resize(
		//@ManMemo: New size (# of elements) of memory block.
		const INT NewTotalEntries
		) {
	CHR* NewBlockPtr = new CHR[NewTotalEntries * MaxStopWordLength];
	if (BlockPtr) {
		if (NewTotalEntries < TotalEntries) {
			TotalEntries = NewTotalEntries;
		}
		memcpy(NewBlockPtr, BlockPtr, TotalEntries * MaxStopWordLength);
		delete [] BlockPtr;
	}
	BlockPtr = NewBlockPtr;
	TotalEntries = NewTotalEntries;
}

STOPWORD::~STOPWORD() {
	if (Changed) {
		EnsureSortedList();
		// Write words back out to file
		FILE* Fp = fopen(SwFileName, "wb");
		if (Fp) {
			fwrite(BlockPtr, 1, TotalEntries * MaxStopWordLength, Fp);
			fclose(Fp);
		}
	}
	delete [] BlockPtr;
}

