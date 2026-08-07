const char RCS_Id[]="$Id: colondoc.cxx,v 1.6 1998/05/12 16:48:28 cnidr Exp $";

/************************************************************************
Copyright (c) 1994,1995 Basis Systeme netzwerk, Munich
              Brecherspitzstr. 8
              D-81541 Munich, Germany

              ISRCH-LIC-1B EXPORT: Tue Aug 15 14:20:42 MET DST 1995

              Public Software License Agreement:
              ----------------------------------

Basis Systeme netzwerk(*) (herein after referred to as BSn) hereby
provides COMPANY (herein after referred to as "Licensee") with a
non-exclusive, royalty-free, worldwide license to use, reproduce,
modify and redistribute this software and its documentation (hereafter
referred to as "Materials"), in whole or in part, with Licensee's
products under the following conditions:

1. All copyrights and restrictions in the source files of the Software
Materials will be honored.

2. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included
in this distribution must remain intact.

3. The origin of these Materials will be explicitly stated in Licensee's
accompanying documentation as developed by Basis Systeme netzwerk (BSn)
and its collaborators.

4. The name of the author(s) or BSn may not be used to endorse or promote
products derived from these Materials without specific prior written
permission.

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

7. Users of this software agree to make their best efforts to inform
BSn of noteworthy uses of this software.

8. You agree that neither you, nor your customers, intend to, or will,
export these Materials to any country which such export or transmission
is restricted by applicable law without prior written consent of the
appropriate government agency with jurisdiction over such export or
transmission.

8. BSn makes no representation on the suitability of the Software
Materials for any purpose.  The SOFTWARE IS PROVIDED "AS IS" AND
WITHOUT EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.

9. Licensee agrees to indemnify, defend and hold harmless BSn from any
loss, claim, damage or liability of any kind, including attorney's fees
and court costs, arising out of or in connection with any use of the
Materials under this License.

10. In no event shall BSn be liable to Licensee or third parties
licensed by licensee for any indirect, special, incidental, or
consequential damages (including lost profits).

11. BSn has no knowledge of any conditions that would impair its right
to license the Materials.  Notwithstanding the foregoing, BSn does
not make any warranties or representations that the Materials are
free of claims by third parties of patent, copyright infringement
or the like, nor does BSn assume any liability in respect of any
such infringement of rights of third parties due to Licensee operation
under this license.

12. IN NO EVENT SHALL BSN OR THE AUTHORS BE LIABLE FOR ANY SPECIAL,
INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

13. The place of execution of this agreement is Munich and the applicable
laws are those of the Federal Republic of Germany. The agreement also
remains in force even in states/jurisdictions that exclude one or more
clauses. For these cases the applicable clauses are to be replaced by
other agreements that come as close as possible to the original intent.

"Diese Vereinbarung unterliegt dem Recht der Bundesrepublik Deutschland.
Sie enthaelt saemtliche Vereinbarungen, welche die Parteien hinsichtlich
des Vereinbarungsgegenstandes getroffen haben, und ersetzt alle
vorhergehenden muendlichen oder schriftlichen Abreden. Diese Vereinbarung
bleibt in Zweifel auch bei rechtlicher Unwirksamkeit enzelner Bestimmungen
in seinen uebrigen Teilen verbindlich. Unwirksame Bestimmungen sind
durch Regulungen zu ersetzen, die dem angestrebten Erfolg moeglichst nahe
kommen."

___________________________________________________________________________________
(*)Basis Systeme netzwerk, Brecherspitzstr. 8, 81541 Muenchen, Germany 

************************************************************************/

/*-@@@
File:		colondoc.cxx
Version:	$Revision: 1.6 $
Description:	Class COLONDOC - IAFA and other Colon Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Distribution:   Isite modifications by A. Warnock (warnock@clark.net)
@@@-*/

// ISEARCH2-CLEANUP: processed 2026-08-07
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "colondoc.hxx"


#define XXXX 0 /* set to 1 to NOT allow white space before field names */

// Local prototypes
static PCHR *parse_tags (PCHR b, GPTYPE len);

COLONDOC::COLONDOC (PIDBOBJ DbParent): DOCTYPE (DbParent)
{
}

void COLONDOC::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}

void COLONDOC::ParseRecords (const RECORD& FileRecord)
{
  // For records use the default parsing mechanism
  DOCTYPE::ParseRecords (FileRecord);
}


const CHR *COLONDOC::UnifiedName (const CHR *tag) const
{
  return tag; // Identity
}

// Reads NewRecord's bytes off disk, splits them into "Tag:"-delimited
// segments via the file-local parse_tags(), and adds one DF field
// (name from UnifiedName(tag), value trimmed of leading/trailing
// whitespace) per segment to NewRecord's DFT.
void COLONDOC::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp)
    {
      return;		// ERROR
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    {
      fseek (fp, 0L, SEEK_END);
      RecStart = 0;
      // BUGFIX #1: this used to be `ftell(fp) - 1`, silently dropping
      // the last byte of every record read through this fallback (the
      // common case: RecordEnd defaults to 0). Neither sgmlnorm.cxx
      // nor sgmltag.cxx do this same "-1" -- sgmltag.cxx even has a
      // `//RecEnd -= 1;` left commented out at the same spot, i.e. a
      // prior author considered and rejected exactly this adjustment.
      // See docs/BUG_CATALOG.md#doctypecolondoccxx.
      RecEnd = ftell (fp);
    }
  fseek (fp, (long)RecStart, SEEK_SET);
  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = new CHR[RecLength + 1];
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  fclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == nullptr || tags[0] == nullptr)
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
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      PCHR p = tags_ptr[1]; // end of field
      // BUGFIX #3: this fallback used RecLength (the buffer's
      // allocated capacity), not ActualLength (how much was actually
      // read) -- harmless when they're equal (the normal case, now
      // that BUGFIX #1 above no longer under-sizes RecLength), but on
      // a short fread() this would extend the last field's value into
      // unread/uninitialized bytes rather than stopping at the real
      // end of the data. See docs/BUG_CATALOG.md#doctypecolondoccxx.
      if (p == nullptr) // If no end of field
	p = &RecBuffer[ActualLength]; // use end of buffer
      // eg "Author:"
      size_t off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ':'
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      INT val_len = (p - *tags_ptr) - off;
      // BUGFIX #1b: this used to unconditionally subtract 1 more here
      // ("leave off the \n"), assuming a trailing newline always sits
      // just before `p`. True for every interior field (the format
      // guarantees exactly one '\n' before the next "Tag:" line), but
      // NOT for the last field's end-of-buffer fallback above when the
      // file doesn't end with '\n' -- confirmed with a standalone test
      // (a file ending "...Jane Doe" with no trailing newline came back
      // as "Jane Do", one byte short). Only exclude it if it's
      // actually there. See docs/BUG_CATALOG.md#doctypecolondoccxx.
      if (val_len > 0 && p[-1] == '\n')
	val_len--;
      // BUGFIX #2: this checked RecBuffer[val_len + val_start], i.e.
      // one byte *past* the value's actual last character
      // (val_start + val_len - 1) -- almost always the delimiter
      // ('\n' or start of the next tag) that val_len's own "- 1"
      // above already excludes, and almost always whitespace, so this
      // silently trimmed one real trailing character off of nearly
      // every field value. Confirmed with a standalone repro
      // ("Hello World" came back as "Hello Worl"). See
      // docs/BUG_CATALOG.md#doctypecolondoccxx.
      // Strip potential trailing while space
      while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	val_len--;
      if (val_len < 0) continue; // Don't bother with empty fields (J. Mandel)
      //      if (val_len <= 0) continue; // Don't bother with empty fields

      const CHR *unified_name = UnifiedName(*tags_ptr);
#if WANT_MISC
      // Throw "unclassified" into Misc
      FieldName = unified_name ? unified_name: "Misc";
#else
      // Ignore "unclassified" fields
      if (unified_name == nullptr) continue; // ignore these
      FieldName = unified_name;
#endif
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      // BUGFIX #4: this used to be `SetFieldEnd(val_start + val_len)`,
      // one past the correct *inclusive* end index (every other
      // doctype parser -- sgmlnorm.cxx, sgmltag.cxx -- computes
      // `val_start + val_len - 1` here, and src/index.cxx derives a
      // field's length as `GetFieldEnd() - GetFieldStart() + 1`, which
      // only works for an inclusive end). This exact "+1" happened to
      // cancel out against BUGFIX #2's "-1" above in the common case
      // (one real trim iteration), which is almost certainly why this
      // went unnoticed -- but the two bugs were independent, and
      // fixing #2 alone (without this) would have made the stored
      // field coordinates wrong instead of accidentally right. See
      // docs/BUG_CATALOG.md#doctypecolondoccxx.
      fc.SetFieldEnd (val_start + val_len - 1);
      PFCT pfct = new FCT ();
      pfct->AddEntry (fc);
      df.SetFct (*pfct);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
      delete pfct;
    }

  NewRecord->SetDft (*pdft);
  delete pdft;
  delete[]RecBuffer;
  delete [] tags;
}

void COLONDOC::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, PSTRING StringBuffer)
{
  DOCTYPE::Present (ResultRecord, ElementSet, StringBuffer);
}

COLONDOC::~COLONDOC ()
{
}

/*-
   What:        Given a buffer of ColonTag (eg. IAFA) data:
   returns a list of char* to all characters pointing to the TAG

   Colon Records:
TAG1: ...
.....
TAG2: ...
TAG3: ...
...
....

1) Fields are continued when the line has no tag
2) Field names may NOT contain white space
3) The space BEFORE field names MAY contain white space
4) Between the field name and the ':' NO white space is
   allowed.

-*/
static PCHR *parse_tags (PCHR b, GPTYPE len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
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
	      if (New == nullptr)
		{
		  delete [] t;
		  return nullptr; // NO MORE CORE!
		}
	      memcpy(New, t, tc*sizeof(PCHR));
 	      delete [] t;
	      t = New;
	    }
	  State = CONTINUING;
	}
      else if ((State == CONTINUING || State == STARTED) && (b[i] == '\n'))
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
  t[tc] = nullptr;
  return t;
}
