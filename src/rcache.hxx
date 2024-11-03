/*@@@
File:		rcache.hxx
Version:	1.00
Description:	Class RCACHE - Result Set Cache
Author:		Jim Fullton
@@@*/

#ifndef RCACHE_HXX
#define RCACHE_HXX
/*
#include "defs.hxx"
#include "string.hxx"
#include "irset.hxx"
#include "idbobj.hxx"
*/
#define MAXCACHE 20
class RCACHE {
public:
	RCACHE(const PIDBOBJ Parent);
	INT Check(STRING Term, INT Relation, STRING FieldName, STRING DBName);
	INT Add(STRING Term, INT Relation, STRING FieldName, STRING DBName, IRSET *Set);
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
