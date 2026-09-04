// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"

RCACHE::RCACHE(const PIDBOBJ DbParent)
{
  Count=0;
  Parent=DbParent;

}
PIRSET RCACHE::Fetch(INT w)
{
  // BUGFIX #3: was unconditionally indexing ResultSet[w] with no bounds
  // check -- callers are expected to pass a slot returned by Check()
  // (which returns -1 for "not found"), but nothing enforced that, and
  // -1 or any Location >= Count would read out of bounds. Every other
  // indexed accessor in this tree (e.g. DFT::GetEntry, FCT::GetEntry)
  // validates its index; this one didn't.
  if (w < 0 || w >= Count) {
    return nullptr;
  }

  PIRSET Temp;

  Temp=ResultSet[w]->Duplicate();
#ifdef DEBUG
  printf("Got entry from cache pos %d\n",w);
#endif
  return(Temp);
}

INT RCACHE::Add(STRING LocalTerm, INT LocalRelation, STRING LocalFieldName,
		STRING LocalDBName, IRSET *Set)
{
  INT Minimum=999999999;
  INT MinPos=-1,i;
  if(Count==MAXCACHE){
    for(i=0; i<Count; i++){
      if(ResultSet[i]->GetTotalEntries()<Minimum){
	Minimum=ResultSet[i]->GetTotalEntries();
	MinPos=i;
      }
    }
    // BUGFIX #2: was `delete ResultSet[i]`, deleting whatever garbage
    // pointer happened to follow the array (i == Count == MAXCACHE here,
    // one past ResultSet's last valid index, since the loop above always
    // runs to completion) instead of the entry the loop just picked for
    // eviction. Confirmed real: filling the cache and adding one more
    // entry produced a UBSan "index 20 out of bounds for type 'IRSET
    // *[20]'" at this line, and the entry that should have been evicted
    // (ResultSet[MinPos]) leaked instead, since it was never freed
    // before being overwritten below.
    delete ResultSet[MinPos];
  }else
    MinPos=Count++;
  ResultSet[MinPos]=Set->Duplicate();
  Term[MinPos]=LocalTerm;
  FieldName[MinPos]=LocalFieldName;
  Relation[MinPos]=LocalRelation;
  DBName[MinPos]=LocalDBName;
#ifdef DEBUG
  cout<< "Added " << LocalTerm << " to cache at "<< MinPos << endl;
#endif
  return(MinPos);
  
}


INT RCACHE::Check(STRING LocalTerm, INT LocalRelation, STRING LocalFieldName, STRING LocalDBName)
{
  INT i;

  for(i=0; i<Count; i++){
    if(LocalTerm==Term[i] 
       && LocalRelation==Relation[i] 
       && LocalDBName==DBName[i] 
       && LocalFieldName==FieldName[i]) {
      return(i);
    }

  }
  return(-1); 

}

RCACHE::~RCACHE()
{
  INT i;

  for(i=0; i<Count; i++)
    delete ResultSet[i];
  Count=0;
}

