// $Id: nlatlon.cxx,v 1.3 2000/02/04 23:39:55 cnidr Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1994. 

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
File:		nlatlon.hxx
Version:	1.00
$Revision: 1.3 $
Description:	Utility functions for longitude and latitude
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-08
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

// Free functions with no callers anywhere in the current tree, and
// nlatlon.o isn't in src/Makefile's production OBJ list either -- dead
// code, though still processed per the standard pipeline.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nlatlon.hxx"

/*
NAME
       ParseLatToNum - convert a string to a latitude

SYNOPSIS
       #include "numlatlon.h"

       double ParseLatToNum(const char *nptr);

DESCRIPTION
       The  ParseLatToNum() function converts  the initial portion of
       the string pointed to by  nptr to float.  North  latitudes are
       returned as positive numbers between 0 and 90, south latitudes
       are returned as negative numbers between -90 and 0.

       Errors are returned as -99.
*/

double ParseLatToNum(char *Term)
{
  int i;
  double sign, value;
  char *accum,tmpnum[2];

  //  if (!(accum=(char*)malloc(strlen(Term)+1)))
  accum= new char [strlen(Term)+1];
  if (!accum)
    return(LatERROR);
  i=0;
  sign=1.0;
  strcpy(accum,"");
  tmpnum[1]='\0';

  while (Term[i] != '\0') {
    if (Term[i] == '-')
      sign = -1.0;
    // BUGFIX #3: isdigit() is only defined for values representable as
    // unsigned char (or EOF); passing a plain (possibly signed) char
    // with the high bit set is undefined behavior. Cast explicitly.
    else if (isdigit((unsigned char)Term[i])) {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if (Term[i] == '.') {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if ((Term[i] == 'N') || (Term[i] == 'n'))
      sign = 1.0;
    else if ((Term[i] == 'S') || (Term[i] == 's'))
      sign = -1.0;
    else {
      // BUGFIX #1: this early return used to skip freeing `accum`,
      // leaking it on every input containing a character outside
      // [-0-9.NnSs] (confirmed via a standalone leak check before
      // fixing; see docs/BUG_CATALOG.md#srcnlatloncxx).
      delete [] accum;
      return(LatERROR);
    }
    i++;
  }
  value = atof(accum);
  //  free(accum);
  delete [] accum;
  if (value > 90.0)
    return(LatERROR);
  value = sign * value;
  return(value);
}

/*
NAME
       ParseLonToNum - convert a string to a latitude

SYNOPSIS
       #include "numlatlon.h"

       double ParseLonToNum(const char *nptr);

DESCRIPTION
       The  ParseLonToNum() function converts  the initial portion of
       the string pointed to by nptr to float.  Longitude is returned
       as a floating point number between 0 and 360, increasing towards
       the east, so western longitudes will be values between 180 and
       360.  The routine will handle either signed numbers or numbers
       suffixed with "E" and "W".  Negative longitudes will be assumed
       to be the same as west longitudes.

       Errors are returned as -99 (LonERROR in nlatlon.hxx; this
       comment previously said -999, which doesn't match the macro).
*/

double ParseLonToNum(char *Term)
{
  int i;
  double sign, value;
  char *accum,tmpnum[2];

  //  if (!(accum=(char*)malloc(strlen(Term)+1)))
  accum= new char [strlen(Term)+1];
  if (!accum)
    return(LonERROR);
  i=0;
  sign=1.0;
  strcpy(accum,"");
  tmpnum[1]='\0';

  while (Term[i] != '\0') {
    if (Term[i] == '-')
      sign = -1.0;
    // BUGFIX #3: see the identical fix in ParseLatToNum above.
    else if (isdigit((unsigned char)Term[i])) {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if (Term[i] == '.') {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if ((Term[i] == 'E') || (Term[i] == 'e'))
      sign = 1.0;
    else if ((Term[i] == 'W') || (Term[i] == 'w'))
      sign = -1.0;
    else {
      // BUGFIX #2: same leak-on-early-return as ParseLatToNum's
      // BUGFIX #1, same fix.
      delete [] accum;
      return(LonERROR);
    }
    i++;
  }
  value = atof(accum);
  //  free(accum);
  delete [] accum;
  if (value > 360.0)
    return(LonERROR);
  if (sign < 0.0)
    value = 360.0 - value;
  return(value);
}

