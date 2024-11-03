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
    delete ResultSet[i];
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

