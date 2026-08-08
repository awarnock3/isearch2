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
// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*-@@@
File:		irlist.cxx
Version:	$Revision: 1.3 $
Description:	Class IRLIST - IRList Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "isearch.hxx"
#include "irlist.hxx"

/* #define BSN_EXTENSIONS 1 */

IRLIST::IRLIST (PIDBOBJ DbParent): MAILFOLDER (DbParent)
{
}

// Splits FileRecord's underlying digest file into one RECORD per
// message, at each "*********" magic separator line or each blank-
// line-preceded mbox "From " line, and adds each to Db via
// DocTypeAddRecord() (each later gets its own ParseFields() call, via
// the inherited MAILFOLDER::ParseFields()).
void IRLIST::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Mail message records
  GPTYPE Start = 0;
  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  GPTYPE RecordEnd;

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      cout << "Could not access '" << Fn << "'\n";
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

  char buf[512];
  const char magic[] = "*********";
  const size_t magic_len = sizeof(magic)/sizeof(char)-1;

  // BUGFIX #1 (docs/BUG_CATALOG.md#doctypeirlistcxx): IsMailFromLine()
  // matches any line starting with a legal Unix mbox "From " envelope
  // header -- extremely common *inside* a mail digest's quoted/forwarded
  // message bodies, not just at genuine message boundaries. The already-
  // processed base class, doctype/mailfolder.cxx's own ParseRecords(),
  // guards this exact same check with a `Look` flag that's only true
  // when the *previous* line was blank (the real mbox convention: a
  // blank line always precedes a new message's "From " line) -- this
  // override dropped that guard entirely, so any "From " text anywhere
  // in a digest's body would spuriously fragment it into bogus records.
  // The magic separator doesn't need this guard (9 asterisks is already
  // distinctive), so only the IsMailFromLine() branch is gated.
  GDT_BOOLEAN Look = GDT_TRUE;

  // Read lines from file and search for record seperation
  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != nullptr)
    {
      // Search for "magic" line type or mail "from "
      size_t line_len = strlen(buf);
      if ((line_len > magic_len && strncmp(buf, magic, magic_len) == 0)
	|| (Look && IsMailFromLine(buf)) )
	{
	  if (buf[0] == magic[0]) Position += line_len;
	  SavePosition = Position;
	  Record.SetRecordStart (Start);
	  // BUGFIX #2 (docs/BUG_CATALOG.md#doctypeirlistcxx): GPTYPE is
	  // UINT4 (src/defs.hxx) -- when SavePosition is 0 (the trigger
	  // line is the very first line of the file, e.g. a standard
	  // mbox file's leading "From " line), `SavePosition - 1`
	  // underflows to UINT_MAX, and `RecordEnd > Start` (0) then
	  // passes, adding a record whose end is ~4 billion bytes past
	  // the real file. Same underflow doctype/mailfolder.cxx's own
	  // ParseRecords() already guards against; this override never
	  // picked up the same fix. Guarded the same way here.
	  RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;

	  if (RecordEnd > Start)
	    {
	      Record.SetRecordEnd (RecordEnd);
	      Db->DocTypeAddRecord(Record);
	      Start = SavePosition;
	    }
	  if (buf[0] != magic[0]) Position += line_len;
	}
      else
       Position += line_len;
      Look = (buf[0] == '\n' || buf[0] == '\r') ? GDT_TRUE : GDT_FALSE;
    }

  fclose (Fp);

  Record.SetRecordStart (Start);
  // BUGFIX #2 (continued): same underflow guard as above -- Position
  // is 0 here for a genuinely empty file (fgets() never succeeded even
  // once), which would otherwise add a bogus ~4-billion-byte record
  // for a zero-byte file.
  RecordEnd = (Position == 0) ? 0 : Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}

IRLIST::~IRLIST ()
{
}
