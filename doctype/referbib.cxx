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

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		referbib.cxx
Version:	$Revision: 1.6 $
Description:	Class REFERBIB - Refer bibliographic records
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "referbib.hxx"

// BSn's Isearch supports date in the Multiple Document Table.
#if BSN_EXTENSIONS
#include <time.h>
extern time_t date_parse(char *);
#endif


/* A few user-customizable parser features... */
#ifndef USE_UNIFIED_NAMES
# define USE_UNIFIED_NAMES 1 
#endif

#define WANT_MISC 1 /* 1=> Store "unclassified" in "misc", 0=> ignore these */
/* Refer databases can contain some lookbib(1) pre-processor records */
#define IGNORE_LOOKBIB_RECORDS 0 /* 1==> Don't bother with fields, 0=> Collect fields */


#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* CNIDR "hardwires" this */
#endif

/*-----------------------------------------------------------------*/

REFERBIB::REFERBIB (PIDBOBJ DbParent): DOCTYPE (DbParent)
{
}

void REFERBIB::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}

// Splits FileRecord's underlying file into one RECORD per bibliography
// entry, delimited by a blank line, and adds each to Db via
// DocTypeAddRecord(). A file with no internal blank line (a single
// entry) is still indexed in full via the post-loop flush.
void REFERBIB::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Medline records
  GPTYPE Start = FileRecord.GetRecordStart();
//  GPTYPE End = FileRecord.GetRecordEnd();
  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  GPTYPE RecordEnd;

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      cout << "Could not access " << Fn << "\n";
      return;			// File not accessed

    }

#if BSN_EXTENSIONS
  RECORD Record (FileRecord); // Easy way
#else
  /* CNIDR must do it the hard way! (see above ) */
  RECORD Record;
  STRING s;
  FileRecord.GetPathName(&s);
  Record.SetPathName( s );
  FileRecord.GetFileName(&s);
  Record.SetFileName( s );
  FileRecord.GetDocumentType(&s);
  Record.SetDocumentType ( s );
#endif
  // Move to start if defined
  if (Start > 0) fseek(Fp, Start, SEEK_SET);

  enum {HUNTING, CONTINUING, LOOKING} State = LOOKING; 
  int Ch;
  while ((Ch = getc (Fp)) != EOF)
    {
      Position++;
      if (Ch == '\n' && State == HUNTING)
	State = LOOKING;
      else if (Ch == '\n' && State == LOOKING) {
	// Have a record boundary ( a blank line )
	State = CONTINUING;
	SavePosition = Position;
	Record.SetRecordStart (Start);
	RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;

	if (RecordEnd > Start)
	  {
	    Record.SetRecordEnd (RecordEnd);
	    Db->DocTypeAddRecord(Record);
	    Start = SavePosition;
	  }
      } else if (Ch == '%' && State == CONTINUING)
	State = HUNTING; // Now in the body
      else if (State == LOOKING)
	State = HUNTING;
    }				// while
  fclose (Fp);

  Record.SetRecordStart (Start);
  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypereferbibcxx): this checked
  // `SavePosition == 0`, not `Position == 0`, even though it's Position
  // being decremented right below -- SavePosition is only ever assigned
  // inside the loop when a blank-line record boundary is actually
  // found, so for a file containing exactly one reference with no
  // internal blank line (nothing unusual for a small bibliography),
  // SavePosition stayed 0 for the whole file even though Position
  // correctly reached the real end-of-file offset. That made this
  // wrongly take the `RecordEnd = 0` branch, which then failed the
  // `RecordEnd > Start` guard below and silently skipped indexing the
  // record entirely. Confirmed via a before/after test-revert. Fixed
  // by checking the same variable being decremented.
  RecordEnd = (Position == 0) ? 0 : Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}


/*
Refer Fields/Tags:

%A := author             Author's name
%B := book               Book containing article referenced
%C := city               City (place of publication)
%D := date               Date of publication
%E := editor             Editor of book containing article referenced
%F  (ignore)             Footnote number or label (supplied by refer)
%G := gov_order_nr       Government order number
%H := comment            Header commentary, printed before reference
%I := publisher          Issuer (publisher)
%J := journal            Journal containing article
%K := keywords           Keywords to use in locating reference
%L  (ignore)             Label field used by -k option of refer
%M  (ignore)             Bell Labs Memorandum (undefined)
%N := number             Number within volume
%O := comment            Other commentary, printed at end of reference
%P := pages              Page number(s)
%Q := corp_author        Corporate or Foreign Author (unreversed)
%R := report             Report, paper, or thesis (unpublished)
%S := series             Series title
%T := title              Title of article or book
%V := volume             Volume number
%X := abstract           Abstract - used by roffbib, not by refer
%Y,Z  (ignore)           Ignored by refer

Tags maked (ignore) and tags not in list should probably be ignored by Isearch
or thown into a "misc" field

NOTE:
  The above tag name list should be "unified" with other bibliographic
formats that might be added to Isearch, eg. BibTeX
*/

const CHR *REFERBIB::UnifiedName(const CHR *tag) const
{
#if USE_UNIFIED_NAMES
  const char *Table[] = {
    /* A */ "author",
    /* B */ "book",
    /* C */ "city",
    /* D */ "date",
    /* E */ "editor",
    /* F */ nullptr,
    /* G */ "gov_order_nr",
    /* H */ "comment",
    /* I */ "publisher",
    /* J */ "journal",
    /* K */ "keywords",
    /* L */ nullptr,
    /* M */ nullptr,
    /* N */ "number",
    /* O */ "comment",
    /* P */ "pages",
    /* Q */ "corp_author",
    /* R */ "report",
    /* S */ "series",
    /* T */ "title",
    /* U */ nullptr,
    /* V */ "volume",
    /* W */ nullptr,
    /* X */ "abstract",
    /* Y */ nullptr,
    /* Z */ nullptr
  };

  // Make sure its a "legal" tag;
  if (tag[0] != '%' || tag[2] != '\0') return tag;
  // Ignore lower case tags
  if (tag[1] < 'A' || tag[1] > 'Z') return nullptr;
  // Return unified field name
  return Table[(unsigned)tag[1] - (unsigned)'A'];
#else
  return tag; // Identity
#endif
}

// Forward reference
static PCHR *parse_tags (PCHR b, GPTYPE len);

// Reads NewRecord's bytes off disk (or, if RecordEnd is unset, treats
// the whole file as a single record -- see BUGFIX #2), splits them
// into "%X value" tag/value pairs via the file-local parse_tags(), and
// adds one DF field per pair to NewRecord's DFT, named via
// UnifiedName() (falling back to "Misc" for an unrecognized tag, per
// WANT_MISC).
void REFERBIB::ParseFields (PRECORD NewRecord)
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
      // BUGFIX #2 (docs/BUG_CATALOG.md#doctypereferbibcxx): this used to
      // be `ftell(fp) - 1`, the same off-by-one truncation already fixed
      // in doctype/colondoc.cxx's BUGFIX #1, doctype/marcdump.cxx's
      // BUGFIX #2, and doctype/memodoc.cxx's BUGFIX #1 -- confirmed via
      // a before/after test-revert to drop the last byte of every
      // record read through this fallback.
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
	  cout << "Warning: No `" << doctype << "' fields/tags in \"" << fn << "\" record.\n";
	}
       else
	{
	  cout << "Unable to parse `" << doctype << "' record in \"" << fn << "\".\n";
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
      PCHR p = tags_ptr[1];
      // BUGFIX #4 (docs/BUG_CATALOG.md#doctypereferbibcxx): this used
      // to be `&RecBuffer[RecLength]` (the buffer's allocated
      // capacity), not ActualLength (how much fread() actually
      // returned) -- same fix as doctype/colondoc.cxx's BUGFIX #3 and
      // doctype/memodoc.cxx's BUGFIX #4, for the same reason.
      if (p == nullptr)
	p = &RecBuffer[ActualLength]; // End of buffer
      // eg "%A "
      int off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ' '
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      INT val_len = (p - *tags_ptr) - off;
      // BUGFIX #5 (docs/BUG_CATALOG.md#doctypereferbibcxx): this used
      // to unconditionally subtract 1 more here ("leave off the \n"),
      // assuming a trailing newline always sits just before `p` -- true
      // for every interior field, but not for the last field's
      // end-of-buffer fallback when the file doesn't end with '\n'.
      // Same fix as doctype/colondoc.cxx's BUGFIX #1b and
      // doctype/memodoc.cxx's BUGFIX #5.
      if (val_len > 0 && p[-1] == '\n')
	val_len--;
      // BUGFIX #6 (docs/BUG_CATALOG.md#doctypereferbibcxx): this checked
      // RecBuffer[val_len + val_start], one byte *past* the value's
      // actual last character (val_start + val_len - 1) -- same
      // off-by-one already fixed in doctype/colondoc.cxx's BUGFIX #2
      // and doctype/memodoc.cxx's BUGFIX #6.
      // Strip potential trailing while space
      while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	val_len--;
      if (val_len < 1) continue; // forget empty fields

#if BSN_EXTENSIONS
      if (strncmp(*tags_ptr, "%D", 2) == 0)
	{
	  time_t date = date_parse(*tags_ptr + 2);
	  NewRecord->SetDate(date);
	}
#endif

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
      // BUGFIX #7 (docs/BUG_CATALOG.md#doctypereferbibcxx): this used
      // to be `SetFieldEnd(val_start + val_len)`, one past the correct
      // *inclusive* end index -- same fix as doctype/colondoc.cxx's
      // BUGFIX #4 and doctype/memodoc.cxx's BUGFIX #7, for the same
      // reason (matches doctype/sgmlnorm.cxx's/doctype/sgmltag.cxx's
      // own `val_start + val_len - 1`, and src/index.cxx derives a
      // field's length as `GetFieldEnd() - GetFieldStart() + 1`, which
      // only works for an inclusive end).
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

void REFERBIB::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, PSTRING StringBuffer)
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
       // Get a Title
       STRING Tag = UnifiedName("%T");
       STRING Value;
       DOCTYPE::Present (ResultRecord, Tag, &Value);
       if (Value.GetLength() == 0)
	{
	  Tag = UnifiedName("%B");
	  DOCTYPE::Present (ResultRecord, Tag, &Value);
	}
       *StringBuffer = Value;

       // Get an author
       Tag = UnifiedName("%A");
       DOCTYPE::Present (ResultRecord, Tag, &Value);
       if (Value.GetLength() == 0)
	{
	  Tag = UnifiedName("%E");
	  DOCTYPE::Present (ResultRecord, Tag, &Value);
	}

	if (Value.GetLength())
	  {
	    if (StringBuffer->GetLength()) StringBuffer->Cat (", ");
	    StringBuffer->Cat (Value);
	  }
    }
  else
    DOCTYPE::Present (ResultRecord, ElementSet, StringBuffer);
}

REFERBIB::~REFERBIB ()
{
}

/*-
   What:        Given a buffer of Refer data:
   returns a list of char* to all characters pointing to the TAG

   Refer Records:
%X ...
.....
%Y ...
%Z ...
...
....

A refer tag is % followed by a letter followed by a space.
Tags such as %br are NOT really tags but placeholders to
represent a blank line (break).

Fields are continued when the line has no tag

-*/
static PCHR *parse_tags (PCHR b, GPTYPE len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING, LOOK_BIB} State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = new PCHR [max_num_tags];
  // BUGFIX #3 (docs/BUG_CATALOG.md#doctypereferbibcxx): this was
  // `i < len - 4`; len is GPTYPE (unsigned UINT4, src/defs.hxx), so for
  // any record under 4 bytes (trivially reached via BUGFIX #2's
  // fallback for an empty file) `len - 4` underflowed to just under
  // UINT_MAX, and the loop body's `b[i+1]`/`b[i+2]` reads ran straight
  // past RecBuffer's real allocation on its very first iteration --
  // same bug already fixed in doctype/memodoc.cxx's BUGFIX #2,
  // confirmed there via ASan and fixed identically here without
  // re-deriving. Rewritten as `i + 4 < len`, an exactly equivalent
  // comparison for every len that doesn't underflow, since it never
  // subtracts from the unsigned len at all.
  for (GPTYPE i = 0; i + 4 < len; i++)
    {
      if (b[i] == '\r' || b[i] == '\v')
 	continue; // Skip over
      else if (State == HUNTING
	&& b[i] == '%'
	&& isalpha(b[i+1])
	&& isspace(b[i+2]) )
	{
	  t[tc] = &b[i]; 
	  State = STARTED;
	}
#if IGNORE_LOOKBIB_RECORDS
      else if (State = HUNTING && b[i] == '.' && b[i+1] == '[')
	{
	  // Lookbib stuff
	  State = LOOK_BIB; // Now in a lookbib(1) record
	}
      else if (State = LOOK_BIB && b[i] == '.' && b[i+1] == ']')
	{
	  // End Lookbib
	  State = HUNTING; // back to business
	} 
#endif
      else if (State == STARTED && isspace(b[i]))
	{
	  b[i] = '\0';
	  // Expand memory if needed
	  if (++tc == max_num_tags - 2)
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
      else if ((State == CONTINUING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
      else if (State == HUNTING)
	State = CONTINUING;
    }
  t[tc] = (PCHR) nullptr;
  return t;
}
