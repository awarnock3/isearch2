// $Id: cgi-util.cxx,v 1.9 1998/05/12 16:48:05 cnidr Exp $
/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1996. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of noteworthy
uses of this software. 

3. The names of MCNC and Clearinghouse for Networked Information Discovery
and Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

/*@@@
File:          	cgi-util.cxx
Version:        1.04
$Revision: 1.9 $
Description:    CGI utilities
Authors:        Kevin Gamiel, kgamiel@cnidr.org
		Tim Gemma, stone@k12.cnidr.org
@@@*/

// change record:
// reset z and initialized entry_point to fix "GET" method    9/25/96 dtw
// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <string.h>
#include <ctype.h>
#include <limits>
#include "cgi-util.hxx"

using namespace std;

/*
# Class: CGIAPP
# Method: GetInput
# Comments: Reads Forms data from standard input, parses out arguments.
*/

void CGIAPP::GetInput() {
  INT ContentLen=0, x, y, z, len, nn;
  CHR temp1[256], temp2[256], temp3[256];
  CHR EmptyQuery[1] = "";
  CHR *meth, *p, *query=nullptr;

  if ((meth = (char *)getenv("REQUEST_METHOD"))==nullptr) {
    cout << "Unable to get request_method" << endl;
    exit(1);
  }
  if (!strcmp(meth,"POST")) {
    if ((p=(char *)getenv("CONTENT_LENGTH")))
      ContentLen = atoi(p);
    Method=POST;
  } else {
    if (!strcmp(meth,"GET")) {
      query = (char *)getenv("QUERY_STRING");
      if (query == nullptr) {
        // BUGFIX #3: this pointed query at a string-literal "" instead
        // of a mutable buffer, but plustospace()/unescape_url() just
        // below both write through their argument unconditionally
        // (unescape_url() always writes a '\0' terminator even for an
        // already-empty string) -- a write to read-only literal memory,
        // undefined behavior that crashes on a typical modern OS.
        // Confirmed with a standalone repro under ASan before fixing:
        // AddressSanitizer: SEGV ... WRITE ... in unescape_url ... in
        // CGIAPP::GetInput -- triggered by simply not having a
        // QUERY_STRING set for a GET request. See
        // docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx.
        query = EmptyQuery;
      }
      Method=GET;
    } else {
      cout << "This program is to be referenced with a METHOD of POST or GET.\n";
      cout << "If you don't understand this, see this ";
      cout << "<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/";
      cout << "Docs/fill-out-forms/overview.html\">forms overview</A>" << endl;
      exit (1);
    }
  }
  // Build the list of cgi entries
  if (Method==POST) {
    entry_count=0;
    for (x = 0; ContentLen>0; x++) {
      if (x >= CGI_MAXENTRIES) {
        break;
      }
      // BUGFIX #1: this told getline the buffer was ContentLen+1 bytes
      // -- the *remaining POST body length*, entirely attacker-
      // controlled via the Content-Length header -- when temp1 is
      // actually a fixed 256-byte stack array. A POST body over 255
      // bytes with no '&' in the first 255 overflowed the stack.
      // Confirmed with a standalone repro under ASan before fixing:
      // AddressSanitizer: stack-buffer-overflow ... in
      // std::istream::getline ... in CGIAPP::GetInput. Capping the
      // read at the buffer's real size fixes the overflow; a field
      // that doesn't fit is then truncated (getline sets failbit
      // without extracting the delimiter), so the remainder up to the
      // real '&' is discarded here to keep the parser aligned with the
      // stream instead of misreading the leftover bytes as the next
      // field. See docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx.
      cin.getline(temp1,sizeof(temp1),'&');
      if (cin.fail()) {
        if (cin.eof())
          break;
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '&');
      }
      entry_count++;
      len=strlen(temp1);
      ContentLen=ContentLen-(len+1);
      for (y=0;temp1[y]!='=' && y<len;y++) {
        temp2[y]=temp1[y];
      }
      temp2[y]='\0';
      y++;
      for (z=0;y<len;y++) {
        temp3[z]=temp1[y];
        z++;
      }
      temp3[z]='\0';
      plustospace(temp3);
      unescape_url(temp3);
      name[x]=new CHR[strlen(temp2)+1];
      strcpy(name[x],temp2);

      // Trim trailing blanks off the string before we stick it into Value
      for (nn=(INT)strlen(temp3)-1;nn>=0;nn--) {
	if(temp3[nn] == ' ')
	  temp3[nn]='\0';
	else
	  break;
      }

      value[x]=new CHR[strlen(temp3)+1];
      strcpy(value[x],temp3);
    }
  } else {  // Get
    entry_count=0;
    y=0;
    z=0;
    plustospace(query);
    unescape_url(query);
    len=strlen(query);
    for (x=0;y<len;x++) {
      if (x >= CGI_MAXENTRIES) {
        break;
      }
      // BUGFIX #2: these two loops copied one query-string byte per
      // iteration into temp1/temp2 (each a fixed 256-byte stack array)
      // with no bound on z at all -- QUERY_STRING is the entire URL
      // query, fully attacker-controlled with no length limit enforced
      // before this code runs, so a single name or value segment over
      // 255 bytes overflowed the stack. Confirmed with a standalone
      // repro under ASan before fixing (a 400-byte field name):
      // AddressSanitizer: stack-buffer-overflow ... in
      // CGIAPP::GetInput. Unlike the POST case above, this is plain
      // in-memory array iteration (not a stream), so capping the write
      // -- while still advancing y through the whole field -- keeps y
      // correctly aligned with the '='/'&' delimiters; only the
      // written copy is truncated. See
      // docs/BUG_CATALOG.md#isearch-cgicgi-utilhxx.
      while ((query[y]!='=') && (query[y]!='&') && (y<len)) {
        if (z < (INT)sizeof(temp1)-1)
          temp1[z++]=query[y];
        y++;
      }
      temp1[z]='\0';
      z=0;
      if (query[y]=='=') {
        y++;
        while ((query[y]!='&') && (y<len)) {
          if (z < (INT)sizeof(temp2)-1)
            temp2[z++]=query[y];
          y++;
        }
      }
      y++;
      temp2[z]='\0';
      z = 0;  // dtw patch for GET ?
      if (temp2[0]=='\0') {
        name[x]=new CHR[1];
        strcpy(name[x],"");

	// Trim trailing blanks off the string before we stick it into Value
	for (nn=(INT)strlen(temp1)-1;nn>=0;nn--) {
	  if(temp1[nn] == ' ')
	    temp1[nn]='\0';
	  else
	    break;
	}

        value[x]=new CHR[strlen(temp1)+1];
        strcpy(value[x],temp1);
      } else {
        name[x]=new CHR[strlen(temp1)+1];
        strcpy(name[x],temp1);

	// Trim trailing blanks off the string before we stick it into Value
	for (nn=(INT)strlen(temp2)-1;nn>=0;nn--) {
	  if(temp2[nn] == ' ')
	    temp2[nn]='\0';
	  else
	    break;
	}

        value[x]=new CHR[strlen(temp2)+1];
        strcpy(value[x],temp2);
      }
      entry_count++;
      z=0;
    }
    entry_count = x;  // dtw patch for GET ?
  }  // Get
}


CGIAPP::CGIAPP() {
  INT i;
  for (i = 0; i < CGI_MAXENTRIES; i++) {
    name[i] = nullptr;
    value[i] = nullptr;
  }
  entry_count = 0;
  GetInput();
}

void CGIAPP::Display() {
  INT x;
  for (x=0;x<entry_count;x++) {
    cout << name[x] << " = " << value[x] << "<br>\n";
  }
}

PCHR CGIAPP::GetName(INT4 i) {
  return name[i];
}

PCHR CGIAPP::GetValue(INT4 i) {
  return value[i];
}

PCHR CGIAPP::GetValueByName(const CHR *field) {
  if ((field==nullptr) || (field[0]=='\0'))
    return nullptr;
  INT i;
  for (i=0;i<entry_count;i++) {
    if ((name[i]==nullptr)||(value[i]==nullptr))
      return nullptr;
    if (!strcmp(name[i], field)) {
      if (value[i] != nullptr) {
        return value[i];
      }
      return nullptr;
    }
  }
  return nullptr;

}

CGIAPP::~CGIAPP() {
  INT i;
  for (i=0;i<entry_count;i++) {
    delete [] name[i];
    delete [] value[i];
  }
}


CHR x2c(PCHR what) {
    CHR digit;
    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return(digit);
}

static INT ishex(CHR c) {
    return isdigit(static_cast<unsigned char>(c)) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

void unescape_url(PCHR url) {
    INT x,y;
    for(x=0,y=0;url[y];++x,++y) {
      if((url[x] = url[y]) == '%' &&
         url[y+1] != '\0' &&
         url[y+2] != '\0' &&
         ishex(url[y+1]) &&
         ishex(url[y+2])) {
        url[x] = x2c(&url[y+1]);
        y+=2;
      }
    }
    url[x] = '\0';
}

void plustospace(PCHR str) {
  INT x;
  for(x=0;str[x];x++)
    if(str[x] == '+')
      str[x] = ' ';
}

void spacetoplus(PCHR str) {
  INT x;
  for(x=0;str[x];x++)
    if(str[x] == ' ')
      str[x] = '+';
}

void escape_url(PCHR url, PCHR out) {
  INT x, y = 0;
  for(x = 0; url[x]; ++x) {
    if (isalnum(static_cast<unsigned char>(url[x])) || (url[x] == ' ')) {
      out[y++] = url[x];
    } else {
      const unsigned char byte = static_cast<unsigned char>(url[x]);
      snprintf(&out[y], 4, "%%%02X", byte);
      y += 3;
    }
  }
  out[y] = '\0';
  spacetoplus(out);
}
