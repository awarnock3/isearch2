static const char RCS_Id[]="$Id: litmed.cxx,v 1.1 1998/05/19 21:01:46 cnidr Exp $";

/*
 * litmed2.cxx -- based on BSN's html.cxx
 */

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
File:		html.cxx
Version:	$Revision: 1.1 $
Description:	Class HTMLTAG - WWW HTML Document Type
Author:   	Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
// #include "common.hxx"
#include "isearch.hxx"
#include "litmed.hxx"

#if BSN_EXTENSIONS < 1
# define BRIEF_MAGIC "B" /* This is what CNIDR uses */
#endif

/* ------- LITMED Support --------------------------------------------- */

LITMED::LITMED (PIDBOBJ DbParent) : SGMLNORM (DbParent)
{
}

void LITMED::ParseRecords (const RECORD& FileRecord)
{
#if 1
  SGMLNORM::ParseRecords (FileRecord);
#else /* DOES NOT WORK, Why? */
  // Handle Proxy LITMED Cached Records

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      cout << "Could not access " << Fn << "\n";
      return;			// File not accessed

    }

  // Search for Start of HTML
  GPTYPE Start = 0;
  int Ch;
  while ((Ch = getc (Fp)) != EOF)
    {
      if (Ch == '<')
	break;
      Start++;
    }
  if (Ch == EOF)
    Start = 0;

  fseek (Fp, 0, SEEK_END);
  GPTYPE End = (GPTYPE)(ftell (Fp) - 1);

  fclose (Fp);

  // Skip the Cache Header
  RECORD Record (FileRecord);
  Record.SetRecordStart (Start);
  Record.SetRecordEnd (End);

  Db->DocTypeAddRecord(Record);
#endif
}

// Kludge to identity Attribute tags
static inline int IsLITMEDAttributeTag (const char *tag)
{
  // HTML Attributes where we are also interested in values
  static struct {
    char *tag;
    unsigned char len;
  } Tags[] = {
    { NULL, 0 }
  };

  if (*tag == '/')
    tag++;

  char ch = tolower(*tag);
  for (int i = 0; Tags[i].len; i++)
    {
      if ((ch == Tags[i].tag[0]
	  && StrNCaseCmp (tag, Tags[i].tag, Tags[i].len) == 0)
	  && isspace (tag[Tags[i].len]))
	return Tags[i].len;
    }
  return 0;			// Not found

}

#if STRICT_LITMED

static int IsLITMEDFieldTag (const char *tag)
{
  static struct
  {
    const char *tag;
    unsigned char len;
  } HtmlTagList[] = {
    {"title", 5},	/* TEMPORARY! */
    {"xtitle", 6},
    {"xannotator2", 11 },
    {"xannotator", 10 },
    {"xauthor", 7},
    {"xcommentary", 11},
    {"xedition", 8},
    {"xeditors", 8},
    {"xgenre", 6},
    {"xkeywords", 9},
    {"xmedium", 7},
    {"xmiscellaneous", 14},
    {"xpublisher", 10},
    {"xsource", 7},
    {"xsummary", 8},
    { 0, 0 }
  };

  // Is this part of a "meaningful" tag?
  if (*tag == '/')
    tag++;

  char ch = tolower(*tag);

  for (int i = 0; HtmlTagList[i].len; i++)
    {
      if (ch == HtmlTagList[i].tag[0]
	  && StrNCaseCmp (tag, HtmlTagList[i].tag, (int) HtmlTagList[i].len) == 0
	  && !isalnum (tag[HtmlTagList[i].len]))
	{
	  return HtmlTagList[i].len;	// YES

	}
    }

  // Is it a tag where we just want the attribute value?
  return IsLITMEDAttributeTag (tag);
}

#else

// Some tags in HTML will don't want to clutter
// our index with.....
static int IgnoreLITMEDTag (const char *tag)
{
  static struct {
    const char *tag;
    unsigned char len;
  } IgnoreList[] = {
    { 0, 0 }
  };

  // Is this part of a tag that we don't want?
  if (*tag == '/') tag++;

  char ch = tolower(*tag);

  for (int i = 0; IgnoreList[i].len; i++)
    {
      if (ch == IgnoreList[i].tag[0]
	  && StrNCaseCmp (tag, IgnoreList[i].tag, (int) IgnoreList[i].len) == 0
	  && !isalnum (tag[IgnoreList[i].len]))
	{
	  return IgnoreList[i].len;	// YES

	}
    }
  return 0;
}
#endif

// Search for the next occurance of an element of tags in tag_list
static const char *find_next_tag (const char *const *tag_list, const char *const *tags)
{
  if (*tag_list == NULL)
    return NULL;

  for (size_t i = 1; tag_list[i]; i++)
    {
      for (size_t j = 0; tags[j]; j++)
	{
	  if (0 == StrCaseCmp (tag_list[i], tags[j]))
	    {
//            cout << "LITMED implied: End of < " << *tag_list << "> is <" << tags[j] << ">\n";
	      return tag_list[i];
	    }
	}
    }
  return NULL;			// No end tag found

}


// Parse more-or-less valid HTML (as well as a few typical
// invalid but common constructs).
// Kludgy but common uses of HTML are kludgy as well
// --- and the job here is to parse and not to validate!
void LITMED::ParseFields (PRECORD NewRecord)
{

  // Open the file
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");

  if (fp == NULL)
    {
    error:
      cout << "Unable to parse LITMED file \"" << fn << "\"\n";
      return;
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    {
      fseek (fp, 0, SEEK_END);
      RecStart = 0;
      RecEnd = ftell (fp) - 1;
    }
  if (-1 == fseek (fp, RecStart, SEEK_SET))
    goto error;			// ERROR

  // Read the whole record in a buffer
  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = new CHR[RecLength + 1];
  size_t ActualLength = (size_t) fread (RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  fclose (fp);

  STRING FieldName;
  FC fc;
  DF df;
  DFD dfd;

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL)
    {
      delete[]RecBuffer;	// Clean up
      goto error;		// ERROR
    }

  PDFT pdft = new DFT ();
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      if ((*tags_ptr)[0] == '/')
	continue;
#if STRICT_LITMED
      // Accept only those tags we "know about"
      if (IsLITMEDFieldTag(*tags_ptr) == 0)
	continue;
#else
      // Accept almost anything not in our list to ignore
      if (IgnoreLITMEDTag(*tags_ptr))
	continue;
#endif

      const char *p = find_end_tag (tags_ptr, *tags_ptr);

      if (p != NULL)
	{
	  // We have a tag pair
	  size_t tag_len = strlen (*tags_ptr);
	  size_t val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
	  int val_len = (p - *tags_ptr) - tag_len - 2;

	  // Skip leading white space
	  while (isspace (RecBuffer[val_start]))
	    val_start++, val_len--;
	  // Leave off trailing white space
	  while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	    val_len--;
	  // Don't bother storing empty fields
	  if (val_len > 0) {
	    // Now cut out the complex values to get tag name
	    CHR orig_char = 0;
	    char* tcp;
	    for (tcp = *tags_ptr; *tcp; tcp++)
	      if (isspace (*tcp))
		{
		  orig_char = *tcp;
		  *tcp = '\0';
		  break;
		}

	    FieldName = *tags_ptr;
//	    FieldName.UpperCase(); // store tags uppercase (not needed yet)
	    if (orig_char) *tcp = orig_char;

	    dfd.SetFieldName (FieldName);
	    Db->DfdtAddEntry (dfd);
	    fc.SetFieldStart (val_start);
	    fc.SetFieldEnd (val_start + val_len - 1);
	    PFCT pfct = new FCT ();
	    pfct->AddEntry (fc);
	    df.SetFct (*pfct);
	    df.SetFieldName (FieldName);
	    pdft->AddEntry (df);
	    delete pfct;
	  }
	}

      // Store the Attribute value if in our list 
      if ((IsLITMEDAttributeTag (*tags_ptr)) > 0)
	{
	  store_attributes (/* Db, */ pdft, RecBuffer, *tags_ptr);
	}
      else if (p == NULL)
	{
#if STRICT_LITMED
	  // Give some information
	  cout << "LITMED Warning: \""
	    << fn << "\" offset " << (*tags_ptr - RecBuffer) << ": "
	    << "No end tag for <" << *tags_ptr << "> found, skipping field.\n";
#endif
	}
    }				/* for() */

  NewRecord->SetDft (*pdft);
  delete pdft;
  delete[]RecBuffer;
  delete [] tags;
}

void LITMED:: Present (const RESULT& ResultRecord, const STRING& ElementSet,
 PSTRING StringBuffer)
{
  STRING FieldName;

  if (ElementSet.Equals(BRIEF_MAGIC))
    FieldName = "title"; // Brief headline is "title"
  else
    FieldName = ElementSet;

  SGMLNORM::Present (ResultRecord, FieldName, StringBuffer);
}

LITMED::~LITMED ()
{
}
