#ifndef FILEMAP_HXX
#define FILEMAP_HXX

// BUGFIX #1: previously had no includes at all (not even a commented-out
// block like most other files in this tree) despite needing GPTYPE,
// STRING, PSTRING, INT, PMDT and PIDBOBJ below. Mirrors the include list
// filemap.cxx itself already needed to use this header at all.
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
#include "idbobj.hxx"

struct _table{
  GPTYPE GpStart;
  GPTYPE LocalStart;
  GPTYPE LocalEnd;
  STRING Path;
};

// Maps a "global" byte offset (as used throughout the index -- a
// document's position within the conceptual concatenation of every
// indexed file) back to the on-disk file it falls within, via a
// binary search over one [GpStart+LocalStart, GpStart+LocalEnd] range
// per MDT entry. Built once from the parent database's main MDT; relies
// on MDT entries already being in ascending GpStart order (true by
// construction -- each document's global start is the running total of
// bytes indexed so far when the corresponding entry was added).
class FILEMAP{

 public:

  FILEMAP(const PIDBOBJ p);
  // Returns the global start offset of the file containing gp, and
  // writes that file's path into *s, its length into *size, and its
  // local (within-file) start offset into *LS. Returns 0 (with a
  // "Lookup failed" diagnostic to stdout) if gp isn't in any file.
  GPTYPE GetNameByGlobal(GPTYPE gp, PSTRING s,INT *size, INT *LS);
  // Like GetNameByGlobal, but only returns the global start offset.
  GPTYPE GetKeyByGlobal(GPTYPE gp);
  ~FILEMAP();

 private:
  struct _table *Items;
  INT MdtCount;
  PMDT mdt;
  PIDBOBJ Parent;

};
#endif
