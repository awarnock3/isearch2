/* $Id: iknowdoc.cxx,v 1.7 1998/11/04 04:50:12 cnidr Exp $ */
/************************************************************************
Copyright (c) 1994,1995 Basis Systeme netzwerk, Munich
              Brecherspitzstr. 8
              D-81541 Munich, Germany

              ISRCH-LIC-1B EXPORT: Tue Aug 15 14:20:42 MET DST 1995

              Public Software License Agreement:
              ----------------------------------

Basis Systeme netzwerk(*) (herein after referred to as BSn) hereby
provides COMPANY (herein after referred to as "Licensee") with a
non-exclusive, royalty-free, worldwide license to use, reproduce, modify
and redistribute this software and its documentation (hereafter referred
to as "Materials"), in whole or in part, with Licensee's products under
the following conditions:

1. All copyrights and restrictions in the source files of the Software
Materials will be honored.

2. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

3. The origin of these Materials will be explicitly stated in Licensee's
accompanying documentation as developed by Basis Systeme netzwerk (BSn)
and its collaborators.

4. The name of the author(s) or BSn may not be used to endorse or
promote products derived from these Materials without specific prior
written permission.

5. Versions of the Software Materials or documentation that are altered
or changed will be marked as such.

6. Licensee shall make reasonable efforts to provide BSn with all
enhancements and modifications made by Licensee to the Software
Materials, for a period of three years from the date of execution of
this License. BSn shall have the right to use and/or redistribute the
modifications and enhancements without accounting to Licensee.

Enhancements and Modifications shall be defined as follows:
    i) Changes to the source code, support files or documentation.
   ii) Documentation directly related to Licensee's distribution of the
       software.
  iii) Licensee software modules that actively solicit services from
       the software and accompanying user documentation.

7. Users of this software agree to make their best efforts to inform BSn
of noteworthy uses of this software.

8. You agree that neither you, nor your customers, intend to, or will,
export these Materials to any country which such export or transmission
is restricted by applicable law without prior written consent of the
appropriate government agency with jurisdiction over such export or
transmission.

8. BSn makes no representation on the suitability of the Software
Materials for any purpose.  The SOFTWARE IS PROVIDED "AS IS" AND WITHOUT
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.

9. Licensee agrees to indemnify, defend and hold harmless BSn from any
loss, claim, damage or liability of any kind, including attorney's fees
and court costs, arising out of or in connection with any use of the
Materials under this License.

10. In no event shall BSn be liable to Licensee or third parties
licensed by licensee for any indirect, special, incidental, or
consequential damages (including lost profits).

11. BSn has no knowledge of any conditions that would impair its right
to license the Materials.  Notwithstanding the foregoing, BSn does not
make any warranties or representations that the Materials are free of
claims by third parties of patent, copyright infringement or the like,
nor does BSn assume any liability in respect of any such infringement of
rights of third parties due to Licensee operation under this license.

12. IN NO EVENT SHALL BSN OR THE AUTHORS BE LIABLE FOR ANY SPECIAL,
INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

13. The place of execution of this agreement is Munich and the
applicable laws are those of the Federal Republic of Germany. The
agreement also remains in force even in states/jurisdictions that
exclude one or more clauses. For these cases the applicable clauses are
to be replaced by other agreements that come as close as possible to the
original intent.

"Diese Vereinbarung unterliegt dem Recht der Bundesrepublik Deutschland.
Sie enthaelt saemtliche Vereinbarungen, welche die Parteien hinsichtlich
des Vereinbarungsgegenstandes getroffen haben, und ersetzt alle
vorhergehenden muendlichen oder schriftlichen Abreden. Diese
Vereinbarung bleibt in Zweifel auch bei rechtlicher Unwirksamkeit
enzelner Bestimmungen in seinen uebrigen Teilen verbindlich. Unwirksame
Bestimmungen sind durch Regulungen zu ersetzen, die dem angestrebten
Erfolg moeglichst nahe kommen."

___________________________________________________________________________
(*)Basis Systeme netzwerk, Brecherspitzstr. 8, 81541 Muenchen, Germany 

************************************************************************/

/************************************************************************
Copyright Notice

Copyright (c) MCNC, Center for Networked Information Discovery and
Retrieval, 1996.

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

3. The names of MCNC and Center for Networked Information Discovery and
Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of
MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*-@@@
File:           iknowdoc.cxx
Version:        1.0
$Revision: 1.7 $
Description:    Extended COLONDOC doctype, allowing value-only searches
Author:         Edward C. Zimmermann, edz@bsn.com
Modification:   Tim Gemma, stone@cnidr.org
@@@-*/

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "iknowdoc.hxx"
#include "rcache.hxx"
#include "dtreg.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "index.hxx"
#include "registry.hxx"
#include "idb.hxx"

IKNOWDOC::IKNOWDOC (PIDBOBJ DbParent) : COLONDOC (DbParent)
{
}

static PCHR *parse_tags (PCHR b, GPTYPE len);

void IKNOWDOC::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp)
    {
      return;           // ERROR
    }
 
  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    {
      fseek (fp, 0L, SEEK_END);
      RecStart = 0;
      RecEnd = ftell (fp) - 1;
    }
  fseek (fp, (long)RecStart, SEEK_SET);
  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = new CHR[RecLength + 1];
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  fclose (fp);
  RecBuffer[ActualLength] = '\0';
 
  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      STRING doctype;
      NewRecord->GetDocumentType(&doctype);
      if (tags)
        {
          delete [] tags;
          cout << "Warning: No `" << doctype << "' fields/tags in \"" << fn << "\"\n";
        }
      else
        {
          cout << "Unable to parse `" << doctype << "' record in \"" << fn << "\"\n";
        }
      delete [] RecBuffer;
      return;
    }
 
  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName;
  // Walk though tags
  INT cnt=0;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      cnt++;
      PCHR p = tags_ptr[1]; // end of field
      if (p == NULL) // If no end of field
        p = &RecBuffer[RecLength]; // use end of buffer
      // eg "Author:"
      size_t off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ':'
      while (isspace (RecBuffer[val_start]))
        val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;
      // Strip potential trailing while space
      while (val_len > 0 && isspace (RecBuffer[val_len + val_start]))
        val_len--;
      //      if (val_len <= 0) continue; // Don't bother with empty fields
      if (val_len < 0) continue; // Don't bother with empty fields
 
      const CHR *unified_name = UnifiedName(*tags_ptr);
#if WANT_MISC
      // Throw "unclassified" into Misc
      FieldName = unified_name ? unified_name: "Misc";
#else
      // Ignore "unclassified" fields
      if (unified_name == NULL) continue; // ignore these
      FieldName = unified_name;
#endif
      dfd.SetFieldName (FieldName);
// Addition (TG) - Keeps track of the list of template-types for later storage.
      if (FieldName.CaseEquals("Template")) {
        PCHR pstr=new CHR[val_len+2];
        memcpy(pstr,RecBuffer+val_start,val_len+1);
        pstr[val_len+1]='\0';
        STRING foo=pstr;
        if (!TemplateTypes.SearchCase(foo)) {
          TemplateTypes.AddEntry(foo);
        }
      }
      if ((cnt==1) && (FieldName!="Template")) {
	 cerr << "Record in \"" << fn 
	 << "\" does not begin with a Template type!\n";
          EXIT_ERROR;
      }
      if ((cnt==2) && (FieldName!="Handle")) {
	 cerr << "Record in \"" << fn 
         << "\" does not have a Handle as its second field!\n";
          EXIT_ERROR;
      }
// End Addition
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      PFCT pfct = new FCT ();
      pfct->AddEntry (fc);
      df.SetFct (*pfct);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
      delete pfct;
// Addition (TG) - All non field-name information stored in field "Value-only"
      FieldName="Value-only";
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      pfct = new FCT ();
      pfct->AddEntry (fc);
      df.SetFct (*pfct);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
      delete pfct;
    }
// End addition 
  NewRecord->SetDft (*pdft);
  delete pdft;
  delete[]RecBuffer;
  delete [] tags;
}

static PCHR *parse_tags (PCHR b, GPTYPE len)
{
  PCHR *t;                      // array of pointers to first char of tags
  size_t tc = 0;                // tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;  // max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING } State = HUNTING;
 
  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = new PCHR [max_num_tags];
  for (GPTYPE i = 0; i < len; i++)
    {
      if (b[i] == '\r' || b[i] == '\v')
        continue; // Skip over
      if (State == HUNTING && !isspace(b[i]))
        {
          t[tc] = &b[i];
          State = STARTED;
        }
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '\t'))
        {
          State = CONTINUING;
        }
      else if ((State == STARTED) && (b[i] == ':'))
        {
          b[i] = '\0';
          // Expand memory if needed
          if (++tc == max_num_tags - 1)
            {
              // allocate more space
              max_num_tags += TAG_GROW_SIZE;
              PCHR *New = new PCHR [max_num_tags];
              if (New == NULL)
                {
                  delete [] t;
                  return NULL; // NO MORE CORE!
                }
              memcpy(New, t, tc*sizeof(PCHR));
              delete [] t;
              t = New;
            }
          State = CONTINUING;
        }
      else if ((State == CONTINUING) && (b[i] == '\n'))
        {
          State = HUNTING;
        }
/* Define XXXX above to NOT allow white space
   before field names */
#if XXXX
      else if (State == HUNTING)
        State = CONTINUING;
#endif
    }
  t[tc] = (PCHR) NULL;
  return t;
}


IKNOWDOC::~IKNOWDOC ()
{
// Addition (TG) - Store list of template-types in file extension .tpt
  STRING temp;
  ((IDB*)Db)->GetDbFileStem(&temp);
  temp.Cat(".tpt");
  PCHR filename=temp.NewCString();
  ofstream out(filename);
  INT len=TemplateTypes.GetTotalEntries();
  for (INT i=1;i<=len;i++) {
    TemplateTypes.GetEntry(i,&temp);
    out << temp << endl;
  }
  delete [] filename;
// End addition
}
