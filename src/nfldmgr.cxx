// $Id
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
File:		index.cxx
Version:	$Revision: 1.6 $
Description:	Class NUMERICFLDMGR
Author:		Jim Fullton, CNIDR
@@@*/

#include <stdlib.h>
#include <string.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "result.hxx"
#include "strlist.hxx"
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
#include "nfldmgr.hxx"



NUMERICFLDMGR::NUMERICFLDMGR()
{
  NumFields = 0;
		
}
NUMERICFLDMGR::~NUMERICFLDMGR()
{
  if(NumFields>0)
    delete [] fields;
}

INT NUMERICFLDMGR::Find(INT Attribute, INT4 Relation, FLOAT Key)
{
  INT FieldIndex;
  INT4 Position=0;

  FieldIndex=LocateFieldByAttribute(Attribute);
  if(FieldIndex==-1)		// no such field
    return(0);
//  Position=fields[FieldIndex].Find(Key,Relation);
  if(Position==-1)
    return(0);
  else
    return(1);
}



/* reads field definition file and loads fields */

INT NUMERICFLDMGR::LoadFields(PCHR dbName)
{
  CHR FullName[256];
  CHR Input[256];
  CHR TypeString[128];
  INT Attribute;
  FILE *fp;
  INT counter=0;
  
  sprintf(FullName,"%s.fdf",dbName); // make definition file name
  fp=fopen(FullName,"rb");
  if(fp==NULL)
    return(0);			// no fields
  while(fgets(Input,256,fp)!=NULL){
    PCHR p;
    p=strchr(Input,'\n');
    if(p)
      *p='\0';			// zap newline
    p=strchr(Input,'#');
    if(p)
      *p='\0';			// zap comments
    if(!strlen(Input))
      continue;
    ++counter;
  }

  fields=new NUMERICLIST[counter];
  rewind(fp);
  while(fgets(Input,256,fp)!=NULL){
    PCHR p;
    p=strchr(Input,'\n');
    if(p)
      *p='\0';			// zap newline
    p=strchr(Input,'#');
    if(p)
      *p='\0';			// zap comments
    if(!strlen(Input))
      continue;
    sscanf(Input,"%d %s",&Attribute,TypeString);
    // 62	TEXT
    // 12	NUMERIC or whatever
    // etc
    
    if(!StrCaseCmp(TypeString,"TEXT")){
      continue;
    }else{
      // make the field
      CHR FieldFile[256];
      
      sprintf(FieldFile,"%s.%d",dbName,Attribute);
      fields[NumFields].SetFileName(FieldFile);
//      fields[NumFields].LoadTable();
      if(fields[NumFields].GetCount==0)
	continue;
      fields[NumFields].SetAttribute(Attribute);
      fields[NumFields].Sort();
      ++NumFields;
      
      
    }
  }	
  fclose(fp);
  
  return(NumFields);
}

/* returns -1 if no field for this attribute, field index if matched */

INT NUMERICFLDMGR::LocateFieldByAttribute(INT Attribute)
{
  INT i,hit=0;
  
  for(i=0; i<NumFields; i++){
    if(Attribute==fields[i].GetAttribute()){
      hit=1;
      break;
    }
  }
  if(hit==0)
    return(-1);
  else{
    return(i);
  }
}


void NUMERICFLDMGR::GetResult(INT use, PIRSET s, PIDBOBJ Parent)
{
  INT i,w;
  INT4 gp;
  IRESULT iresult;
  MDT* ThisMdt;

  //fields[i].ResetHitPosition();
  i=LocateFieldByAttribute(use);
//  printf("GetResult: Found field %d by attribute\n",i);
  while((gp=fields[i].GetNextHitPosition())!=-1){
    w = Parent->GetMainMdt()->LookupByGp(gp);
    ThisMdt = Parent->GetMainMdt();
    iresult.SetMdtIndex(w);
    iresult.SetHitCount(1);
    iresult.SetScore(0);
    iresult.SetMdt(*ThisMdt);
    s->AddEntry(iresult, 1);
  }
}
