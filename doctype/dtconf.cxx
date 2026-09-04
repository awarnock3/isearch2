/* $Id: dtconf.cxx,v 1.14 2000/08/15 03:30:51 cnidr Exp $ */
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1995.

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
File:		dtconf.cxx
Version:	1.00
$Revision: 1.14 $
Description:	Document Type configuration utility for Isearch
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

/*
  #if defined(_MSC_VER) && _MSC_VER > 1010
  #  include <iostream>
  #else
  #  include <iostream.h>
  #endif
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define EXIT_ERROR {fclose(stdout); fclose(stderr); exit (1);}
#define RETURN_ZERO {fclose(stdout); fclose(stderr); return (0);}

#define MAXDT 500
#define MAXSTR 80

static char DtName[MAXDT][MAXSTR];
static char DtFn[MAXDT][MAXSTR];

int main() {
  printf("\nConfiguring Isearch for the following document types (see dtconf.inf):");

  // Read configuration
  int x;
#ifndef DEV_STUDIO
  int y;
#endif
  int TotalDt = 0;
  char s[MAXSTR], t[MAXSTR+8], u[MAXSTR], v[MAXSTR];  // t: DtFn[] + ".hxx"
  char* p;
  char* pp;
  FILE* fp;
  FILE* fpi;
  fp = fopen("dtconf.inf", "r");
  if (!fp) {
    fprintf(stderr,"You need to create a doctype configuration file: dtconf.inf\n");
    EXIT_ERROR;
  }
  else {
    while ( fgets(s, MAXSTR, fp) ) {
      p = s;
      while ((isalnum(*p)) || *p == '_') {	// truncate after the first word
	p++;
      }
      *p = '\0';
      if (*s != '\0') {
	// BUGFIX #2 (docs/BUG_CATALOG.md#doctypedtconfcxx): DtName/DtFn
	// are fixed-size static arrays (MAXDT entries); TotalDt was
	// incremented with no bound on it at all, so a dtconf.inf with
	// more than MAXDT (500) entries would silently overflow both
	// static arrays. Not reachable with the current dtconf.inf (38
	// entries), but a real static-buffer overflow risk from
	// unbounded config-file input. Fixed by refusing to process any
	// further entries once the table is full, instead of writing
	// past it.
	if (TotalDt >= MAXDT) {
	  fprintf(stderr, "Too many document types configured (max %d); ignoring the rest of dtconf.inf.\n", MAXDT);
	  break;
	}
	strcpy(DtFn[TotalDt], s);
	// BUGFIX #1 (docs/BUG_CATALOG.md#doctypedtconfcxx): confirmed via
	// -Wall -Wextra ("sprintf output between 5 and 84 bytes into a
	// destination of size 80") -- DtFn[TotalDt] can be up to 79
	// characters (bounded only by fgets()'s own MAXSTR cap), and
	// appending ".hxx" (4 more bytes) plus the terminator can exceed
	// t's 80-byte buffer. Not reachable with any of the short
	// doctype names in the real dtconf.inf, but a real latent
	// overflow for a long enough one. Fixed with snprintf.
	snprintf(t, sizeof(t), "%s.hxx", DtFn[TotalDt]);	// append .hxx
	fpi = fopen(t, "r");
	if (fpi) {
	  x = 0;
	  while ( (fgets(u, MAXSTR, fpi)) && (!x) ) {
	    if (!strncmp(u, "class ", 6)) {
	      x = 1;
	      strcpy(v, u);
	      p = v + 6;
	      while (*p == ' ') {
		p++;
	      }
	      pp = p;
	      while (isalnum(*pp) || *pp == '_') {
		pp++;
	      }
	      *pp = '\0';
	      strcpy(DtName[TotalDt], p);
	    }
	  }
	  fclose(fpi);
	  printf("\t%s", DtName[TotalDt]);
	  fpi = fopen(t, "r");
	  if (fpi) {
	    x = 0;
	    while ( (fgets(u, MAXSTR, fpi)) && (!x) ) {
	      if (!strncmp(u, "Description:", 12)) {
		x = 1;
		if ( (p=strchr(u, '-')) ) {
		  printf(" %s", p);
		}
	      }
	    }
	    fclose(fpi);
	  } else {
	    printf("\n");
	  }
	  TotalDt++;
	} else {
	  printf("\t(File %s not found.)\n", t);
	}
      }
    }
    fclose(fp);
  }
  printf("\n");

  // Generate dtreg.hxx
#if defined(_MSDOS) || defined(_WIN32)
  printf("Creating ..\\src\\dtreg.hxx\n");
  fp = fopen("..\\src\\dtreg.hxx", "w");
  if (!fp) {
    perror("src\\dtreg.hxx");
    EXIT_ERROR;
  }
#else
  printf("Creating ../src/dtreg.hxx\n");
  fp = fopen("../src/dtreg.hxx", "w");
  if (!fp) {
    perror("src/dtreg.hxx");
    EXIT_ERROR;
  }
#endif
  fprintf(fp, "/*@@@\n");
  fprintf(fp, "File:\t\tdtreg.hxx\n");
  fprintf(fp, "Version:\t1.00\n");
  fprintf(fp, "Description:\tClass DTREG - Document Type Registry\n");
  fprintf(fp, "Author:\t\tNassib Nassar, nrn@cnidr.org\n");
  fprintf(fp, "@@@*/\n");
  fprintf(fp, "\n");
  fprintf(fp, "#ifndef DTREG_HXX\n");
  fprintf(fp, "#define DTREG_HXX\n");
  fprintf(fp, "\n");
  //	fprintf(fp, "#include \"defs.hxx\"\n");
  fprintf(fp, "#include \"../doctype/doctype.hxx\"\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "#include \"../doctype/%s.hxx\"\n", DtFn[x]);
  }
  fprintf(fp, "\n");
  fprintf(fp, "class DTREG {\n");
  fprintf(fp, "public:\n");
  fprintf(fp, "\tDTREG(PIDBOBJ DbParent);\n");
  fprintf(fp, "\tPDOCTYPE GetDocTypePtr(const STRING& DocType);\n");
  fprintf(fp, "\tvoid GetDocTypeList(PSTRLIST StringListBuffer) const;\n");
  fprintf(fp, "\t~DTREG();\n");
  fprintf(fp, "private:\n");
  fprintf(fp, "\tPIDBOBJ Db;\n");
  fprintf(fp, "\tPDOCTYPE DtDocType;\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "\tP%s Dt%s;\n", DtName[x], DtName[x]);
  }
  fprintf(fp, "};\n");
  fprintf(fp, "\n");
  fprintf(fp, "typedef DTREG* PDTREG;\n");
  fprintf(fp, "\n");
  fprintf(fp, "#endif\n");
  fclose(fp);
  
  // Generate dtreg.cxx
#if defined(_MSDOS) || defined(_WIN32)
  printf("Creating ..\\src\\dtreg.cxx\n");
  fp = fopen("..\\src\\dtreg.cxx", "w");
  if (!fp) {
    perror("src\\dtreg.cxx");
    EXIT_ERROR;
  }
#else
  printf("Creating ../src/dtreg.cxx\n");
  fp = fopen("../src/dtreg.cxx", "w");
  if (!fp) {
    perror("src/dtreg.cxx");
    EXIT_ERROR;
  }
#endif
  fprintf(fp, "/*@@@\n");
  fprintf(fp, "File:\t\tdtreg.cxx\n");
  fprintf(fp, "Version:\t1.00\n");
  fprintf(fp, "Description:\tClass DTREG - Document Type Registry\n");
  fprintf(fp, "Author:\t\tNassib Nassar, nrn@cnidr.org\n");
  fprintf(fp, "@@@*/\n");
  fprintf(fp, "\n");
  fprintf(fp, "#include <stdlib.h>\n");
  fprintf(fp, "#include \"defs.hxx\"\n");
  fprintf(fp, "#include \"string.hxx\"\n");
  fprintf(fp, "#include \"vlist.hxx\"\n");
  fprintf(fp, "#include \"strlist.hxx\"\n");
  fprintf(fp, "#include \"attr.hxx\"\n");
  fprintf(fp, "#include \"attrlist.hxx\"\n");
  fprintf(fp, "#include \"mdtrec.hxx\"\n");
  fprintf(fp, "#include \"mdt.hxx\"\n");
  fprintf(fp, "#include \"fc.hxx\"\n");
  fprintf(fp, "#include \"fct.hxx\"\n");
  fprintf(fp, "#include \"df.hxx\"\n");
  fprintf(fp, "#include \"dfd.hxx\"\n");
  fprintf(fp, "#include \"dft.hxx\"\n");
  fprintf(fp, "#include \"dfdt.hxx\"\n");
  fprintf(fp, "#include \"result.hxx\"\n");
  fprintf(fp, "#include \"record.hxx\"\n");
  fprintf(fp, "#include \"idbobj.hxx\"\n");
  fprintf(fp, "#include \"iresult.hxx\"\n");
  fprintf(fp, "#include \"opobj.hxx\"\n");
  fprintf(fp, "#include \"rset.hxx\"\n");
  fprintf(fp, "#include \"operand.hxx\"\n");
  fprintf(fp, "#include \"irset.hxx\"\n");
  fprintf(fp, "#include \"opstack.hxx\"\n");
  fprintf(fp, "#include \"squery.hxx\"\n");
  fprintf(fp, "#include \"dtreg.hxx\"\n");
  fprintf(fp, "\n");
  fprintf(fp, "DTREG::DTREG(PIDBOBJ DbParent) {\n");
  fprintf(fp, "\tDb = DbParent;\n");
  fprintf(fp, "\tDtDocType = new DOCTYPE(Db);\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "\tDt%s = 0;\n", DtName[x]);
  }
  fprintf(fp, "}\n");
  fprintf(fp, "\n");
  fprintf(fp, "PDOCTYPE DTREG::GetDocTypePtr(const STRING& DocType) {\n");
  fprintf(fp, "\tif (DocType.Equals(\"\")) {\n");
  fprintf(fp, "\t\treturn DtDocType;\n");
  fprintf(fp, "\t}\n");
  fprintf(fp, "\tSTRING DocTypeID;\n");
  fprintf(fp, "\tDocTypeID = DocType;\n");
  fprintf(fp, "\tDocTypeID.UpperCase();\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "\tif (DocTypeID.Equals(\"%s\")) {\n", DtName[x]);
    fprintf(fp, "\t\tif (!Dt%s) {\n", DtName[x]);
    fprintf(fp, "\t\t\tDt%s = new %s(Db);\n", DtName[x], DtName[x]);
    fprintf(fp, "\t\t}\n");
    fprintf(fp, "\t\treturn Dt%s;\n", DtName[x]);
    fprintf(fp, "\t}\n");
  }
  //  fprintf(fp, "\treturn 0;\n");
  // Return default doctype if not recognized
  fprintf(fp, "\treturn DtDocType;\n");
  fprintf(fp, "}\n");
  fprintf(fp, "\n");
  fprintf(fp, "void DTREG::GetDocTypeList(PSTRLIST StringListBuffer) const {\n");
  fprintf(fp, "\tSTRING s;\n");
  fprintf(fp, "\tSTRLIST DocTypeList;\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "\ts = \"%s\";\n", DtName[x]);
    fprintf(fp, "\tDocTypeList.AddEntry(s);\n");
  }
  fprintf(fp, "\t*StringListBuffer = DocTypeList;\n");
  fprintf(fp, "}\n");
  fprintf(fp, "\n");
  fprintf(fp, "DTREG::~DTREG() {\n");
  fprintf(fp, "\tdelete DtDocType;\n");
  for (x=0; x<TotalDt; x++) {
    fprintf(fp, "\tif (Dt%s) {\n", DtName[x]);
    fprintf(fp, "\t\tdelete Dt%s;\n", DtName[x]);
    fprintf(fp, "\t}\n");
  }
  fprintf(fp, "}\n");
  fclose(fp);
  
#ifdef UNIX
  // Generate Makefile
  printf("Creating ../src/Makefile\n");
  fpi = fopen("../src/Makefile.000", "r");
  if (!fpi) {
    perror("src/Makefile.000");
    EXIT_ERROR;
  }
  fp = fopen("../src/Makefile", "w");
  if (!fp) {
    perror("src/Makefile");
    EXIT_ERROR;
  }
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#####                                                                   #####\n");
  fprintf(fp, "##### NOTE: This Makefile was generated by dtconf/autoconf.  To make    #####\n");
  fprintf(fp, "#####       changes to the Makefile, modify the file Makefile.000.in    #####\n");
  fprintf(fp, "#####       instead of this file.  The dtconf utility uses Makefile.000 #####\n");
  fprintf(fp, "#####       to generate this file, and Makefile.000 is generated from   #####\n");
  fprintf(fp, "#####       Makefile.000.in by Gnu Autoconf.                            #####\n");
  fprintf(fp, "#####                                                                   #####\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n\n");
  if (fpi) {
    while ( fgets(s, MAXSTR, fpi) ) {
      y = 0;
      if (!strncmp(s, "###DTOBJ###", 11)) {
	y = 1;
	fprintf(fp, "\tdoctype.o");
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, " \\\n\t%s.o", DtFn[x]);
	}
	fprintf(fp, "\n\n");
      }
      if (!strncmp(s, "###DTHXX###", 11)) {
	y = 1;
	fprintf(fp, "\t$(DOCTYPE_DIR)/doctype.hxx");
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, " \\\n\t$(DOCTYPE_DIR)/%s.hxx", DtFn[x]);
	}
	fprintf(fp, "\n\n");
      }
      if (!strncmp(s, "###DTMAKE###", 12)) {
	y = 1;
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, "%s.o:$(H) $(DOCTYPE_DIR)/%s.cxx\n", DtFn[x], DtFn[x]);
	  fprintf(fp, "\t$(CC) $(CFLAGS) $(INC) -o $@ -c $(DOCTYPE_DIR)/%s.cxx\n", DtFn[x]);
	  //	  fprintf(fp, "\t$(CC) $(CFLAGS) $(INC) -o $@ -c $(DOCTYPE_DIR)/%s.cxx;\\\n", DtFn[x]);
	  //	  fprintf(fp, "\t$(AR) $(ARFLAGS) $(BIN_DIR)/$(LIB) $@;\n");
	  fprintf(fp, "\n");
	}
      }
      if (!y) {
	fprintf(fp, "%s", s);
      }
    }
    fclose(fpi);
  }
  fclose(fp);
  
  printf("\n");
#else
  // Skip making the Makefile if we're in Developer's Studio
#ifndef DEV_STUDIO
  // Generate Makefile
  printf("Creating ..\\src\\Isearch.mak\n");
  fpi = fopen("..\\src\\Isearch0.mak", "r");
  if (!fpi) {
    perror("src\\Isearch0.mak");
    EXIT_ERROR;
  }
  fp = fopen("..\\src\\Isearch.mak", "w");
  if (!fp) {
    perror("src\\Isearch.mak");
    EXIT_ERROR;
  }
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#####                                                                   #####\n");
  fprintf(fp, "##### NOTE: This Makefile was generated by dtconf.  To make changes     #####\n");
  fprintf(fp, "#####       to the Makefile, modify the file Isearch0.mak instead of    #####\n");
  fprintf(fp, "#####       this file.  The dtconf utility uses Isearch0.mak to         #####\n");
  fprintf(fp, "#####       generate this file.                                         #####\n");
  fprintf(fp, "#####                                                                   #####\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n");
  fprintf(fp, "#############################################################################\n\n");
  if (fpi) {
    while ( fgets(s, MAXSTR, fpi) ) {
      y = 0;
      if (!strncmp(s, "###DTOBJ###", 11)) {
	y = 1;
	fprintf(fp, "\tdoctype.obj");
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, " \\\n\t%s.obj", DtFn[x]);
	}
	fprintf(fp, "\n\n");
      }
      if (!strncmp(s, "###DTHXX###", 11)) {
	y = 1;
	fprintf(fp, "\t$(DOCTYPE_DIR)\\doctype.hxx");
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, " \\\n\t$(DOCTYPE_DIR)\\%s.hxx", DtFn[x]);
	}
	fprintf(fp, "\n\n");
      }
      if (!strncmp(s, "###DTMAKE###", 12)) {
	y = 1;
	for (x=0; x<TotalDt; x++) {
	  fprintf(fp, "%s.obj:$(H) $(DOCTYPE_DIR)\\%s.cxx\n", DtFn[x], DtFn[x]);
	  fprintf(fp, "\t$(CC) $(CFLAGS) $(INC) -c $(DOCTYPE_DIR)\\%s.cxx\n", DtFn[x]);
	  fprintf(fp, "\n");
	}
      }
      if (!y) {
	fprintf(fp, "%s", s);
      }
    }
    fclose(fpi);
  }
  fclose(fp);
  
  printf("\n");
#endif
#endif
  RETURN_ZERO;
}
