/* $Id: marc.cxx,v 1.9 1998/05/12 16:49:10 cnidr Exp $ */
/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Author:	Ray Larson, ray@sherlock.berkeley.edu
 *		School of Library and Information Studies, UC Berkeley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**************************************************************************/
/* DispMARC - print marc records from a file                              */
/**************************************************************************/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

//#define index(s,c) strchr(s,c)

#include "gdt.h"
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

#ifdef UNIX
#include <unistd.h>
#endif

#include "marcdefs.hxx"
#include "memcntl.hxx"
#include "marclib.hxx"
#include "marc.hxx"

// Forward declaration: the definition lives further down (with the
// rest of the "EXTERNAL ROUTINES" block, matching marclib.cxx's own
// `extern struct MemBlock *RememberKey;`), but MARC::~MARC() (BUGFIX
// #1, see docs/BUG_CATALOG.md#srcmarchxx) needs it before that point.
extern struct MemBlock *RememberKey;

MARC::MARC(STRING & Data)
  // BUGFIX #2: c_format/c_maxlen used to be set at the bottom of this
  // constructor, after the GetMARC() failure check's early `return;` --
  // a malformed record left them uninitialized on exactly the path
  // where a caller (with no way to ask "did construction succeed?" --
  // c_rec isn't exposed either) is most likely to still go on to call
  // Print()/GetPrettyBuffer(), which read c_maxlen as a word-wrap width
  // and index into a line buffer with it. Moved into the initializer
  // list so they're always valid. See docs/BUG_CATALOG.md#srcmarchxx.
  : c_format(0), c_maxlen(79)
{
  c_data = Data.NewCString();
  c_len = Data.GetLength();
  if((c_rec = GetMARC(c_data,c_len,0)) == nullptr) {
    cerr << "Error parsing MARC record" << endl;
    return;
  }
}

MARC::~MARC()
{
  if(c_data)
    delete [] c_data;

  // BUGFIX #1: this comment used to be the only trace of the leak --
  // c_rec (and every MARC_FIELD/MARC_SUBFIELD hung off it) was
  // allocated via AllocSafe(&RememberKey, ...) in GetMARC()/marclib.cxx
  // and never freed. Fixed using FreeSafe()'s "free everything"
  // flag=1 path (src/memcntl.cxx), which walks and frees the entire
  // RememberKey chain in one call -- everything GetMARC() allocates
  // goes through that same chain (confirmed via marclib.cxx), except
  // record data itself when GetMARC() is called with copy=0 (as it is
  // here, from the constructor above), which points straight at
  // c_data and is freed separately just above, not double-freed here.
  //
  // Caveat for any *future* MARC caller: RememberKey is one process-
  // wide chain, not per-object, so this call frees the c_rec of every
  // MARC object, not just this one -- safe only because the sole
  // caller in the tree (doctype/usmarc.cxx's USMARC::Present())
  // never keeps two MARC objects alive at once (construct, use,
  // delete, in that order, every time). See
  // docs/BUG_CATALOG.md#srcmarchxx.
  if (c_rec)
    FreeSafe(&RememberKey, nullptr, 1);
}


#define   MENUHT         3

#define   RECBUFSIZE     10000
#define   FIELDBUFSIZE   10000
#define   READONLY       O_RDONLY
#define   BADFILE        -1
#define   TRUE           1
#define   FALSE          0

#ifndef   SEEK_CUR
#define   SEEK_CUR       1
#endif


/* EXTERNAL ROUTINES -- in marclib.c and memcntl.c */

struct MemBlock *RememberKey;
 
/* EXTERNAL VARIABLES */
extern struct MemBlock *RememberKey;    /* key for memory allocation */
char recbuffer[RECBUFSIZE];
char fieldbuffer[FIELDBUFSIZE];
char linebuffer[FIELDBUFSIZE];

typedef struct {
  const char *label;
  const char *tags;
  const char *subfields;
  const char *beginpunct;
  const char *subfsep;
  const char *endpunct;
  int  newfield;
  int  print_all;
  int  print_indicators;
  int  print_delimiters;
  int  repeatlabel;
  int  indent;
} DISP_FORMAT;
	
DISP_FORMAT defaultformat[] = {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {"Author:" , "1xx", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Publisher:", "260", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Pages:"  , "300", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Series:" , "4xx", "", ""," ", "\n", TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Notes:"  , "5xx", "", "", " ","\n",   TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Subjects:","6xx", "", "", " -- ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Other authors:","7xx", "", "", " ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Call Numbers:","950", "", "", " ","\n",TRUE,FALSE,FALSE,FALSE,FALSE,15},
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

DISP_FORMAT titleformat[] =  {
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};


DISP_FORMAT shortformat[] =  {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {"Author:" , "1xx", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

 DISP_FORMAT marcformat[] =  {
     {"Record ID: ", "", "",""," ","\n", TRUE,FALSE,FALSE,FALSE,FALSE, 0},
     {"" , "xxx", "", "","", "\n",  TRUE,TRUE,TRUE,TRUE,FALSE,0},
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

DISP_FORMAT evaluationformat[] = {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {" ", "", "",""," "," ",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
     {" "  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
/*
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Subjects:","6xx", "", "", " -- ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Call Numbers:","950", "", "", " ","\n",TRUE,FALSE,FALSE,FALSE,FALSE,15},
*/
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

DISP_FORMAT htmlformat[] = {

  // {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
  // {"Author:" , "1xx", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  //     {""  , "245", "", "<H1>"," ", "</H1>\n",  TRUE,FALSE,FALSE,FALSE,FALSE,0},
     {""  , "245", "00a", "<H1>","", "</H1>\n",  TRUE,FALSE,FALSE,FALSE,FALSE,0},
  // {"Publisher:", "260", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Pages:"  , "300", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Series:" , "4xx", "", ""," ", "\n", TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Notes:"  , "5xx", "", "", " ","\n",   TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Subjects:","6xx", "", "", " -- ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Other authors:","7xx", "", "", " ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
  // {"Call Numbers:","950", "", "", " ","\n",TRUE,FALSE,FALSE,FALSE,FALSE,15},
     {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

/* local prototypes */
char *format_field(MARC_FIELD *mf,const DISP_FORMAT *format,CHR *buff,INT repeat);
void  outputline(void *(outfunc)(),CHR *line, INT maxlen, INT indent,
	FILE *fp);
void  OutputString(CHR *line, INT maxlen, INT indent, STRING* Buffer);

void 
MARC::GetPrettyBuffer(STRING *Buffer)
{
  /*
  // Cheese, cheese, cheese;-)
  char *tempfile = tempnam("/tmp", "marc");
  FILE *fp;
  if((fp = fopen(tempfile, "w")) == NULL) {
    *Buffer = "MARC::GetPrettyBuffer() failed to open temp file";
    return;
  }

  Print(fp);
  fclose(fp);

  Buffer->ReadFile(tempfile);
  unlink(tempfile);
  */
  Print(Buffer);
}


void 
MARC::Print(FILE *fp)
{
  INT format = c_format;
  INT displaynum = -1;
  INT maxlen = c_maxlen;
  INT recordID = 1;

  MARC_FIELD *fld;
  //MARC_REC *rec;
  //INT4 lrecl;
  int  repeat = FALSE;
  char *line;
  DISP_FORMAT *formatcontrol, *f;
        
  switch (format) {
  case MARC_FORMAT_SHORT: 
    formatcontrol = &shortformat[0];
    break;
  case MARC_FORMAT_MARC: 
    formatcontrol = &marcformat[0];
    break;
  case MARC_FORMAT_EVALUATION: 
    formatcontrol = &evaluationformat[0];
    break;
  case MARC_FORMAT_TITLE: 
    formatcontrol = &titleformat[0];
    break;
  case MARC_FORMAT_HTML: 
    formatcontrol = &htmlformat[0];
    break;
  default: 
    formatcontrol = &defaultformat[0];
    break;
  }

  if (displaynum == -1) displaynum = recordID;

  for (f = formatcontrol; f->label; f++) {
    /* get the first field in the format */
    fld = GetField(c_rec, (MARC_FIELD *)nullptr, fieldbuffer, f->tags);

    /* if no field found, check for number format */
    if (fld == nullptr && *f->tags == '\0') {
      /* a null tag means output the supplied */
      /* record number			*/
      snprintf(linebuffer, sizeof(linebuffer), "%s%s%d%s", 
	       f->label, f->beginpunct,displaynum, 
	       f->endpunct);
      /* assume it won't be INT4er than maxlen*/
      outputline (nullptr,linebuffer, maxlen, f->indent, fp);
    }	
    repeat = FALSE;
			
    while (fld) {
      if (f->print_all) {
	codeconvert(fieldbuffer);
	STRING lineOut;
	if (*f->label == '\0')
	  lineOut = fld->tag;
	else
	  lineOut = f->label;
	lineOut.Cat(" ");
	lineOut.Cat(f->beginpunct);
	lineOut.Cat(fieldbuffer);
	lineOut.Cat(f->endpunct);
	lineOut.GetCString(linebuffer, sizeof(linebuffer) - 1);
	outputline (nullptr, linebuffer, maxlen, f->indent, fp);
      }
      else  {/* more selective printing */
	line = format_field(fld,f,linebuffer,repeat);
	if (line) outputline (nullptr, line, maxlen, f->indent, fp);
      }
      /* more of the same tag set? */
      fld = GetField((MARC_REC *)nullptr,fld->next,fieldbuffer,f->tags);
      if (fld) repeat = TRUE;
    }
  }
	        return;
}


void 
MARC::Print(STRING* StringBuffer)
{
  STRING Hold;
  INT format = c_format;
  INT displaynum = -1;
  INT maxlen = c_maxlen;
  INT recordID = 1;

  MARC_FIELD *fld;
  //MARC_REC *rec;
  //INT4 lrecl;
  INT  repeat = FALSE;
  CHR *line;
  DISP_FORMAT *formatcontrol, *f;
        
  switch (format) {
  case MARC_FORMAT_SHORT: 
    formatcontrol = &shortformat[0];
    break;
  case MARC_FORMAT_MARC: 
    formatcontrol = &marcformat[0];
    break;
  case MARC_FORMAT_EVALUATION: 
    formatcontrol = &evaluationformat[0];
    break;
  case MARC_FORMAT_TITLE: 
    formatcontrol = &titleformat[0];
    break;
  case MARC_FORMAT_HTML: 
    formatcontrol = &htmlformat[0];
    break;
  default: 
    formatcontrol = &defaultformat[0];
    break;
  }

  if (displaynum == -1) 
    displaynum = recordID;

  for (f = formatcontrol; f->label; f++) {
    // get the first field in the format
    fld = GetField(c_rec, (MARC_FIELD *)nullptr, fieldbuffer, f->tags);

    // if no field found, check for number format
    if (fld == nullptr && *f->tags == '\0') {
      // a null tag means output the supplied record number
      snprintf(linebuffer, sizeof(linebuffer), "%s%s%d%s", 
	       f->label, f->beginpunct,displaynum, 
	       f->endpunct);
      OutputString(linebuffer, maxlen, f->indent, &Hold);
      StringBuffer->Cat(Hold);
    }	
    repeat = FALSE;
			
    while (fld) {
      if (f->print_all) {
	codeconvert(fieldbuffer);
	STRING lineOut;
	if (*f->label == '\0')
	  lineOut = fld->tag;
	else
	  lineOut = f->label;
	lineOut.Cat(" ");
	lineOut.Cat(f->beginpunct);
	lineOut.Cat(fieldbuffer);
	lineOut.Cat(f->endpunct);
	lineOut.GetCString(linebuffer, sizeof(linebuffer) - 1);
	OutputString(linebuffer, maxlen, f->indent, &Hold);
	StringBuffer->Cat(Hold);
      } else  {
	// more selective printing
	line = format_field(fld,f,linebuffer,repeat);
	if (line) {
	  OutputString(line, maxlen, f->indent, &Hold);
	  StringBuffer->Cat(Hold);
	}
      }
      // more of the same tag set?
      fld = GetField((MARC_REC *)nullptr,fld->next,fieldbuffer,f->tags);
      if (fld) 
	repeat = TRUE;
    }
  }
  return;
}


/***********************************************************************/
/* format_field - given a marc field struct and a format item, build   */
/*                a line in a buffer according to the format.          */
/***********************************************************************/
char *
format_field(MARC_FIELD *mf, const DISP_FORMAT *format, CHR *buff, INT repeat)
{
  MARC_SUBFIELD *subf;
  char *linend;
  const char *c;
  INT pos, count, ok=0;
	
  linend = buff;
  *linend = '\0';
  pos = 0;

  if (repeat && (format->repeatlabel == FALSE)) ;  // skip it
  else { // add the label
    for(c = format->label; *c ; *linend++ = *c++) 
      pos++;
  }

  // indentation
  for (; pos < format->indent; pos++) 
    *linend++ = ' ';
  // initial 'punctuation'
  for(c = format->beginpunct; *c ; *linend++ = *c++);
  // subfields
  for (subf = mf->subfield; subf; subf = subf->next) {
    if ((*format->subfields == '\0') || 
	(strchr(format->subfields, subf->code))) {	
      // this one should be copied
      for(c = format->beginpunct; *c ; *linend++ = *c++);
      count = subfcopy(linend,subf->data,1) - 1;
      linend += count;
      for(c = format->subfsep; *c ; *linend++ = *c++);
      ok = TRUE;
    }
  }
	
  if (ok) {
    // backtrack over the last subfield separator
    linend -= strlen(format->subfsep);
    // add end of field punctuation
    if (*format->endpunct) {
      // kill the existing punctuation and trailing blanks
      if (ispunct(*(linend-1)) && ispunct(*format->endpunct) &&
	  *(linend-1) != ')' && *(linend-1) != ']')
	linend--;
      while(*(linend-1) == ' ') 
	linend--;
      for(c = format->endpunct; *c ; *linend++ = *c++) 
	pos++;
    }
    *linend = '\0'; // Null terminate the line
    return(buff);	
  }
  else 
    return(nullptr); // no subfields copied
}



/* break an output line into segments to fit on the screen and call the */
/* output function                                                      */
/*
KAG - Hacking this to ignore outfunc b/c I figure out how to get the C++
compiler to quit complaining about passing the wrong number of args
to outfunc():-(
*/
void 
outputline(void *(outfunc)(), CHR *line, INT maxlen, INT indent, FILE *fp)
{
  INT linelen, i;
  CHR indentstr[80];
  CHR *nextpart, *c;

  // if the line will fit output it now
  if ((linelen = strlen(line)) <= maxlen) {
    //	(*outfunc)(line);
    fwrite(line, 1, strlen(line), fp);
    return;
  }
  else { // put out first part, no indentation
    // BUGFIX #3: this scanned backward for a space with no lower
    // bound -- a single "word" (e.g. a URL or identifier with no
    // spaces) at or past maxlen bytes into a real MARC field's data
    // ran the scan past the start of line, reading (and then writing
    // '\0' into) memory before the buffer. Confirmed a real stack-
    // buffer-underflow with a standalone repro under ASan before
    // fixing (a 299-byte space-less field). Bounded at `line`, falling
    // back to a hard break at maxlen-1 if no space is found in range,
    // instead of scanning indefinitely. See
    // docs/BUG_CATALOG.md#srcmarchxx.
    for (c = &line[maxlen - 1]; c > line && *c != ' '; c--); // find word break
    if (c == line && *c != ' ') c = &line[maxlen - 1]; // no space in range; hard break
    *c = '\0';
    nextpart = c+1;
    fwrite(line, 1, strlen(line), fp);
    //(*outfunc)(line);
    fprintf(fp, "\n");
    //(*outfunc)("\n");
  }
  // set up indent string - add 3 spaces to regular indent for wrapped lines
  indent += 3;
  for (i=0;i<indent; i++) indentstr[i] = ' ';
  indentstr[i] = '\0';

  // loop to output rest of line
  while ((linelen = strlen(nextpart)) > (maxlen - indent)) {
    // BUGFIX #3 (continued, see the first-part word-break scan above):
    // same unbounded-backward-scan shape, same fix.
    for (c = &nextpart[maxlen - indent]; c > nextpart && *c != ' '; c--);
    if (c == nextpart && *c != ' ') c = &nextpart[maxlen - indent]; // hard break
    // find word break
    *c = '\0'; 
    // (*outfunc)(indentstr); 
    // (*outfunc)(nextpart); 
    // (*outfunc)("\n");
    fwrite(indentstr, 1, strlen(indentstr), fp);
    fwrite(nextpart, 1, strlen(nextpart), fp);
    fprintf(fp, "\n");
    nextpart = c+1;  
  }
  
  // (*outfunc)(indentstr); 
  // (*outfunc)(nextpart);
  fwrite(indentstr, 1, strlen(indentstr), fp);
  fwrite(nextpart, 1, strlen(nextpart), fp);
  return;
}


void 
OutputString(CHR *line, INT maxlen, INT indent, STRING* Buffer)
{
  INT linelen, i;
  CHR indentstr[80];
  CHR *nextpart, *c;

  // if the line will fit output it now 
  if ((linelen = strlen(line)) <= maxlen) {
    *Buffer = line;
    return;
  } else { 
    // put out first part, no indentation
    // BUGFIX #3: this scanned backward for a space with no lower
    // bound -- a single "word" (e.g. a URL or identifier with no
    // spaces) at or past maxlen bytes into a real MARC field's data
    // ran the scan past the start of line, reading (and then writing
    // '\0' into) memory before the buffer. Confirmed a real stack-
    // buffer-underflow with a standalone repro under ASan before
    // fixing (a 299-byte space-less field). Bounded at `line`, falling
    // back to a hard break at maxlen-1 if no space is found in range,
    // instead of scanning indefinitely. See
    // docs/BUG_CATALOG.md#srcmarchxx.
    for (c = &line[maxlen - 1]; c > line && *c != ' '; c--); // find word break
    if (c == line && *c != ' ') c = &line[maxlen - 1]; // no space in range; hard break
    *c = '\0';
    nextpart = c+1;
    *Buffer = line;
    Buffer->Cat("\n");
  }
  // set up indent string - add 3 spaces to regular indent for wrapped lines
  indent += 3;
  for (i=0;i<indent; i++) indentstr[i] = ' ';
  indentstr[i] = '\0';

  // loop to output rest of line
  while ((linelen = strlen(nextpart)) > (maxlen - indent)) {
    // BUGFIX #3 (continued, see the first-part word-break scan above):
    // same unbounded-backward-scan shape, same fix.
    for (c = &nextpart[maxlen - indent]; c > nextpart && *c != ' '; c--);
    if (c == nextpart && *c != ' ') c = &nextpart[maxlen - indent]; // hard break
    // find word break
    *c = '\0'; 
    Buffer->Cat(indentstr);
    Buffer->Cat(nextpart);
    Buffer->Cat("\n");
    nextpart = c+1;  
  }
  Buffer->Cat(indentstr);
  Buffer->Cat(nextpart);
  return;
}
