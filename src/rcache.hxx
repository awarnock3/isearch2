/*@@@
File:		rcache.hxx
Version:	1.00
Description:	Class RCACHE - Result Set Cache
Author:		Jim Fullton
@@@*/
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef RCACHE_HXX
#define RCACHE_HXX

#include "defs.hxx"   // BUGFIX #1: was commented out; INT below needs it.
#include "string.hxx" // BUGFIX #1: was commented out; STRING below needs it.
#include "irset.hxx"  // BUGFIX #1: was commented out; IRSET below needs it.
#include "idbobj.hxx" // BUGFIX #1: was commented out; PIDBOBJ below needs it.

#define MAXCACHE 20

// A fixed-capacity (MAXCACHE) cache of recent IRSET search results,
// keyed by the (Term, Relation, FieldName, DBName) query that produced
// each one. Currently unused in this tree -- its one call site
// (src/index.cxx) is commented out -- but its own bugs (see
// docs/BUG_CATALOG.md) are real and were confirmed with standalone
// repros independent of that dormant caller.
class RCACHE {
public:
	RCACHE(const PIDBOBJ Parent);
	// Returns the cache slot matching this exact query, or -1 if none
	// matches. Pass the result to Fetch() to retrieve the cached IRSET.
	INT Check(STRING Term, INT Relation, STRING FieldName, STRING DBName);
	// Caches a duplicate of *Set under this query, evicting the entry
	// with the fewest total entries first if the cache is full. Returns
	// the slot it was stored at.
	INT Add(STRING Term, INT Relation, STRING FieldName, STRING DBName, IRSET *Set);
	// Returns a duplicate of the IRSET at Location (as returned by a
	// prior Check()), or nullptr if Location is out of range.
	IRSET *Fetch(INT Location);

	~RCACHE();
private:

	INT Count;
	IRSET *ResultSet[MAXCACHE];
	INT Relation[MAXCACHE];
	STRING Term[MAXCACHE];
	STRING FieldName[MAXCACHE];
	STRING DBName[MAXCACHE];
	PIDBOBJ Parent;
};

#endif
