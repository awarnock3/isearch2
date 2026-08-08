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

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// NUMERICFLDMGR manages a set of per-attribute NUMERICLIST field
// tables (one per configured numeric field) loaded from a database's
// "<dbName>.fdf" definition file. Note: this class has no callers
// anywhere in the current tree and src/Makefile's OBJ list doesn't
// link nfldmgr.o into the production binary either -- it is dead code,
// though still processed per the standard pipeline. It also relies on
// two features left commented-out by the original author (LoadTable()
// in LoadFields(), and NUMERICLIST::Find() in Find()); see
// docs/BUG_CATALOG.md for why those are documented as incomplete
// rather than guessed at.

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



// BUGFIX #2 (docs/BUG_CATALOG.md#srcnfldmgrcxx): fields/MaxEntries were
// left uninitialized; fields is only read in ~NUMERICFLDMGR()/GetResult()
// when NumFields>0, which LoadFields() guarantees, but leaving a raw
// pointer member indeterminate is needless risk for a one-line fix.
NUMERICFLDMGR::NUMERICFLDMGR() : fields(nullptr), NumFields(0), MaxEntries(0)
{
}
// BUGFIX #5: was `if(NumFields>0) delete [] fields;`. LoadFields()
// allocates `fields` (an array sized by the .fdf file's line count)
// before the loop that increments NumFields, so any call that ends up
// loading zero fields -- e.g. every field being "TEXT", or (as things
// stand today, see BUGFIX #1's note) every field's GetCount() coming
// back 0 -- leaked the whole array and its NUMERICLIST elements'
// internal STRING buffers. Confirmed via a real ASan leak report from
// this exact path. `delete` on a null pointer is a no-op, so the
// NumFields>0 guard was never actually needed.
NUMERICFLDMGR::~NUMERICFLDMGR()
{
  delete [] fields;
}

// Incomplete: the actual per-field lookup (fields[FieldIndex].Find(Key,
// Relation)) is commented out below, so Position is always its 0
// initializer and this always returns 1 (found) once Attribute matches
// a loaded field, regardless of Key/Relation. Left as-is rather than
// guessed at -- see the file-level comment above.
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



// Parses "<dbName>.fdf" (one "<attribute> <type>" line per field,
// '#' starts a comment, blank lines skipped) and loads a NUMERICLIST
// per non-TEXT field. Returns the number of fields loaded (0 if the
// .fdf file doesn't exist).
INT NUMERICFLDMGR::LoadFields(PCHR dbName)
{
  CHR FullName[256];
  CHR Input[256];
  CHR TypeString[128];
  INT Attribute;
  FILE *fp;
  INT counter=0;

  // BUGFIX #2: a second call would otherwise leak whatever `fields`
  // already pointed at from a prior call (delete on nullptr is a
  // no-op, so this is safe even before the first successful call; see
  // BUGFIX #5 for why NumFields>0 alone isn't sufficient here).
  delete [] fields;
  NumFields=0;

  snprintf(FullName,sizeof(FullName),"%s.fdf",dbName); // make definition file name
  fp=fopen(FullName,"rb");
  if(fp==nullptr)
    return(0);			// no fields
  while(fgets(Input,256,fp)!=nullptr){
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
  while(fgets(Input,256,fp)!=nullptr){
    PCHR p;
    p=strchr(Input,'\n');
    if(p)
      *p='\0';			// zap newline
    p=strchr(Input,'#');
    if(p)
      *p='\0';			// zap comments
    if(!strlen(Input))
      continue;
    // BUGFIX #4: unbounded %s could overflow TypeString[128] if a line's
    // second token is longer than 127 bytes.
    sscanf(Input,"%d %127s",&Attribute,TypeString);
    // 62	TEXT
    // 12	NUMERIC or whatever
    // etc

    if(!StrCaseCmp(TypeString,"TEXT")){
      continue;
    }else{
      // make the field
      CHR FieldFile[256];

      snprintf(FieldFile,sizeof(FieldFile),"%s.%d",dbName,Attribute);
      fields[NumFields].SetFileName(FieldFile);
//      fields[NumFields].LoadTable();
      // BUGFIX #1: was `GetCount==0` (member function reference, not a
      // call -- doesn't even compile, "did you forget the ()?"). Note
      // LoadTable() above is itself commented out, so GetCount() is
      // always 0 here regardless; see docs/BUG_CATALOG.md for why this
      // is left as a documented incomplete feature rather than guessed
      // at further.
      if(fields[NumFields].GetCount()==0)
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


// Adds one IRESULT to `s` per hit position recorded against the
// NUMERICLIST field matching attribute `use`; a no-op if no field
// matches (see BUGFIX #3).
void NUMERICFLDMGR::GetResult(INT use, PIRSET s, PIDBOBJ Parent)
{
  INT i,w;
  INT4 gp;
  IRESULT iresult;
  MDT* ThisMdt;

  //fields[i].ResetHitPosition();
  i=LocateFieldByAttribute(use);
//  printf("GetResult: Found field %d by attribute\n",i);
  // BUGFIX #3: LocateFieldByAttribute() returns -1 when `use` doesn't
  // match any loaded field (including when none were ever loaded);
  // indexing fields[-1] below was an out-of-bounds read with no guard.
  if(i==-1)
    return;
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
