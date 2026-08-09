// $Id: thesaurus.hxx,v 1.1 1999/04/17 03:23:50 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) A/WWW Enterprises, 1999.
************************************************************************/

/*@@@
File:		thesaurus.hxx
Version:	$Revision: 1.1 $
Description:	Class THESAURUS - Thesaurus and synonyms
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef THES_HXX
#define THES_HXX

#include <stdlib.h>
#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"


class TH_PARENT {
public:
  TH_PARENT();
  INT4   GetGlobalStart();
  void   SetGlobalStart(INT4 x);
  void   GetString(STRING* TheParent);
  void   SetString(const STRING& NewParent);
  void   Copy(const TH_PARENT& OtherValue);
  TH_PARENT& operator=(const TH_PARENT& OtherValue);
  ~TH_PARENT();

private:
  INT4   GlobalStart;
  STRING Term;
};

typedef TH_PARENT* PTH_PARENT;


/// Owns a heap-allocated, growable table of TH_PARENT entries. Grows
/// automatically in AddEntry() when full (see BUGFIX #1). Non-copyable:
/// no live call site ever copies one, so the copy constructor and
/// operator= are both deleted rather than given deep-copy semantics.
class TH_PARENT_LIST {
public:
  TH_PARENT_LIST();
  // BUGFIX #3 (docs/BUG_CATALOG.md#srcthesaurushxx): TH_PARENT_LIST
  // owned `table` (heap-allocated, freed in ~TH_PARENT_LIST()) with no
  // user-declared copy constructor or operator= -- the compiler-
  // generated ones shallow-copied `table`, confirmed to cause a
  // heap-use-after-free (ASan) on copy. No live call site ever copies a
  // TH_PARENT_LIST, so it's made explicitly non-copyable rather than
  // given deep-copy semantics.
  TH_PARENT_LIST(const TH_PARENT_LIST&) = delete;
  TH_PARENT_LIST& operator=(const TH_PARENT_LIST&) = delete;
  void AddEntry(const TH_PARENT& NewParent);
  void GetEntry(const INT4 index, TH_PARENT* TheParent);
  TH_PARENT* GetEntry(const INT4 index);
  INT4 GetCount();
  void Dump(PFILE fp);
  void LoadTable(PFILE fp);
  void WriteTable(PFILE fp);
  void Sort();
  void *Search(const void* term);
  ~TH_PARENT_LIST();

private:
  PTH_PARENT table; // The table of all parent terms
  INT4       Count;
  INT4       MaxEntries;
};


class TH_ENTRY {
public:
  TH_ENTRY();
  INT4  GetGlobalStart();
  void  SetGlobalStart(INT4 x);
  void  GetString(STRING* TheChild);
  void  SetString(const STRING& NewChild);
  INT4  GetParentPtr();
  void  SetParentPtr(INT4 x);
  ~TH_ENTRY();

private:
  INT4      GlobalStart;
  INT4      ParentPtr;
  STRING    Term;
};

typedef TH_ENTRY* PTH_ENTRY;


/// Owns a heap-allocated, growable table of TH_ENTRY entries. Grows
/// automatically in AddEntry() when full (see BUGFIX #1). Non-copyable,
/// matching TH_PARENT_LIST above: no live call site ever copies one.
class TH_ENTRY_LIST {
public:
  TH_ENTRY_LIST();
  // BUGFIX #3, second class -- see TH_PARENT_LIST above.
  TH_ENTRY_LIST(const TH_ENTRY_LIST&) = delete;
  TH_ENTRY_LIST& operator=(const TH_ENTRY_LIST&) = delete;
  void AddEntry(const TH_ENTRY& NewEntry);
  void GetEntry(const INT4 index, TH_ENTRY* TheEntry);
  TH_ENTRY* GetEntry(const INT4 index);
  INT4 GetCount();
  void Dump(PFILE fp);
  void WriteTable(PFILE fp);
  void Sort();
  void LoadTable(PFILE fp);
  void *Search(const void* term);
  ~TH_ENTRY_LIST();

private:
  PTH_ENTRY  table; // The table of all parent terms
  INT4       Count;
  INT4       MaxEntries;
};


extern const CHR* DbExtDbSynonyms;
extern const CHR* DbExtDbSynParents;
extern const CHR* DbExtDbSynChildren;

// Maximum line length for parent+children (need safe buffers for disk I/O!)
#define MAX_SYN_LENGTH 1024

class THESAURUS {
public:
  THESAURUS(const STRING& DbPathName, const STRING& DbFileName);
  THESAURUS(const STRING& SourceFileName, const STRING& DbPathName, 
	    const STRING& DbFileName);
  void  SetFileName(const STRING& Fn);
  void  GetFileName(STRING* Fn);
  void  GetChildren(const STRING& ParentTerm, STRLIST* Children);
  void  GetParent(const STRING& ChildTerm, STRING* TheParent);
  ~THESAURUS();

private:
  FILE*       OpenSynonymFile(const char *mode);
  FILE*       OpenParentsFile(const char *mode);
  FILE*       OpenChildrenFile(const char *mode);
  void        GetIndirectString(PFILE fp, const INT4 ptr, STRING* term);
  void        LoadParents();
  void        LoadChildren();
  GDT_BOOLEAN MatchParent(const STRING& ParentTerm, INT4 *ptr);
  GDT_BOOLEAN MatchChild(const STRING& Term, INT4 *ptr);

  TH_PARENT_LIST Parents;
  TH_ENTRY_LIST  Children;
  STRING         BaseFileName;

};

typedef THESAURUS* PTHESAURUS;

#endif /* THES_HXX */
