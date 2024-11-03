/* $Id: hash.cxx,v 1.5 1998/05/12 16:49:04 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1994.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/


/*@@@
File:		hash.cxx
Version:	1.00
$Revision: 1.5 $
Description:	Hash class
Author:		Jim Fullton (Jim.Fullton@cnidr.org)
@@@*/

#include <stdlib.h>
#include <string.h>
#include "gdt.h"
#include "string.hxx"
#include "hash.hxx"


INT IndexStr2Num(CHR *s)
{
  
  INT Value=0,st=*s;
  INT t=0;
  
  while(*s){
    Value+=((*s)*(*s))-t;
    ++t;
    ++s;
  }
  return(Value+st);
}


HASH::HASH()
{
  Setup(997);
}

HASH::HASH(INT Size)
{
  Setup(Size);
}

void HASH::Setup(INT Size)
{
  INT i;

  TableSize=Size;
  H=new Item_type[TableSize];
  for(i=0; i<TableSize; i++){
    H[i].Key=-1;
    H[i].State=0;		// empty
    H[i].Block=(Data_type)NULL;
  }
  
}

void HASH::GetValue(const STRING& a, STRING *b) const
{

  CHR name[256];
  Key_type v;
  Item_type *r;

  a.GetCString(name,256);

  v=IndexStr2Num(name);
  r=Find(v);
  if(r==NULL){
    *b="";
  }else{
    *b=(CHR *)r->Block;
    delete r;
  }

}

void HASH::AddEntry(const STRING& a) const
{
  // a is of type name=value
  
  CHR name[256],Value[256],d[513];
  CHR *p;
  Key_type v;
  Item_type r;
   
  a.GetCString(d,512);
  p=strchr(d,'=');
  *p='\0';
  strcpy(name,d);
  ++p;
 
  strcpy(Value,p);
  r.Key=IndexStr2Num(name);
  r.Block=strdup(Value);
  if (!Check(r.Key)) {
    // printf("Add %s/%s\n",name,Value);
    Insert(r);
  } else {
   // printf("%s/%s already exists\n",name,Value);
  }

}

INT HASH::HashFunction(Key_type s) const
{
  // this must be adjusted for the Key type!!!
  // Key could be a class, but why bother for a hash table?
  
  return abs(s%TableSize);
}

// this is sort of bogus.

INT HASH::Insert(Item_type r) const
{
  INT c=0;			// count to ensure table is not full
  INT i=1;			// increment used for quadratic probing
  INT p;			// current probed position
  
  p=HashFunction(r.Key);
  while(H[p].State==1 &&	// filled position (not empty or deleted)
	H[p].Key!=r.Key &&		// the key doesn't already exist
	c<=TableSize/2) {		// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }

  if(H[p].State!=1) {		// it's empty or deleted
    H[p].State=1;
    H[p].Key=r.Key;
    H[p].Block=r.Block;
  } else if(H[p].Key==r.Key)	// oops - duplicate key
    return(1); 
  else
    return(2);			// table overflow
  return(0);			// valid insertion
  
}

Item_type* HASH::Find(Key_type r) const
{
  INT c=0;			// count to ensure table is not full
  INT i=1;			// increment used for quadratic probing
  INT p;			// current probed position
  Item_type *result;
  
  p=HashFunction(r);
  while(H[p].State!=0 &&	// filled or deleted  position (not empty)
	H[p].Key!=r &&		// key not yet found
	c<=TableSize/2) {	// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }
  if (H[p].Key==r) {
    result=new Item_type;
    result->Key=H[p].Key;
    result->State=1;
    result->Block=H[p].Block;
    return(result);
  } else
    return NULL;

}

INT HASH::Check(Key_type r) const
{
  INT c=0;			// count to ensure table is not full
  INT i=1;			// increment used for quadratic probing
  INT p;			// current probed position
  
  p=HashFunction(r);
  while(H[p].State!=0 &&	// filled or deleted  position (not empty)
	H[p].Key!=r &&		// key not yet found
	c<=TableSize/2) {	// there is room for probing.....
    c++;
    p+=i;
    i+=2;
    if(p>=TableSize)
      p%=TableSize;
  }
  if(H[p].Key==r) {
    return(1);
  } else
    return(0);

}

HASH::~HASH()
{
  INT i;
  for(i=0; i<TableSize; i++) {
    if(H[i].Block)
      free(H[i].Block);
  }
  delete [] H;
  TableSize=0;
}
