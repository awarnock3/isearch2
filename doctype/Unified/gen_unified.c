// ISEARCH2-CLEANUP: processed 2026-08-09
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdio.h>
#include <string.h>
// BUGFIX #1 (docs/BUG_CATALOG.md#doctypeunifiedgen_unifiedc): this file
// called exit()/atoi() and isspace()/isdigit()/tolower() with neither
// <stdlib.h> nor <ctype.h> included -- confirmed via -Wall -Wextra:
// every one of those calls fell back to an implicit int-returning
// declaration (a hard error under strict C99+, a warning under GNU
// dialects). Harmless by luck on this platform since the assumed
// signatures happen to match, but real undefined behavior per the C
// standard, and the same "header not self-contained" defect already
// fixed throughout this project (e.g. src/fc.hxx's BUGFIX #1).
#include <stdlib.h>
#include <ctype.h>

#define COMMENT_CHAR '#'

char prefix[] = "_S";
char misc[] = "other_fields";
char message[] = "/* Don't edit this file, it is produced automaticaly from %s */\n";

/* Note: if START_COUNT is negative it counts down */
#ifndef START_COUNT
# define START_COUNT -9000 /* Start "private" numbers here */
#endif

FILE *myopen(const char *ext)
{
  char buf[256];
  char rootname[] = "unified";

  snprintf(buf, sizeof(buf), "%s.%s", rootname, ext);
  return fopen(buf, "w");
}

int main(int argc, char **argv)
{
 char buf[256];
 char temp[256];
 int i, value;
 int count=START_COUNT;
 int num = 0;

 FILE *fp1 = myopen("h");
 FILE *fp2 = myopen("c");
 FILE *fp3 = myopen("inc");

 // BUGFIX #2 (docs/BUG_CATALOG.md#doctypeunifiedgen_unifiedc): argv[1]
 // not existing/being unreadable left freopen() failing silently --
 // its return value was never checked, so stdin was left unusable and
 // the fgets() loop below would simply never run, silently producing
 // near-empty (header-only) unified.h/.c/.inc instead of reporting the
 // real problem.
 if (argc != 1 && freopen(argv[1], "r", stdin) == NULL) {
   printf("ERROR: could not open input file %s\n", argv[1]);
   exit(-1);
 }

 // BUGFIX #3 (docs/BUG_CATALOG.md#doctypeunifiedgen_unifiedc): fp3 (the
 // .inc file) was never checked for NULL here, unlike fp1/fp2 -- if
 // opening it specifically failed (e.g. a pre-existing unified.inc
 // that isn't writable), it would reach the unconditional
 // fprintf(fp3, ...) below as a null-pointer dereference.
 if (fp1 == NULL || fp2 == NULL || fp3 == NULL) {
   printf("ERROR\n");
   exit(-1);
 }

 fprintf(fp1, message, argc != 1 ? argv[1] : "<stdin>");
 fprintf(fp2, message, argc != 1 ? argv[1] : "<stdin>");
 fprintf(fp3, message, argc != 1 ? argv[1] : "<stdin>");
 fprintf(fp1, "#ifndef _UNIFIED_H\n#define _UNIFIED_H\n");
 fprintf(fp2, "const char *_UnifiedName(int id)\n{\n  switch (id) {\n");
 fprintf(fp3, "#ifndef _UNIFIED_INC\n#define _UNIFIED_INC\n");
 while (fgets(buf, 255, stdin) != NULL) {
  char *tcp;

  if ((tcp = strchr(buf, COMMENT_CHAR)) != NULL)
    *tcp = '\0';

  value = 0;
  for (i=0; buf[i]; i++)
    if (buf[i] == '-') temp[i] = '_';
    else if (isspace(buf[i])) {
	temp[i] = buf[i] = 0;
	if (isdigit(buf[i+1]))
	  value = atoi(&buf[i+1]); /* Have a Bib-1 number */
    } else temp[i] = tolower(buf[i]);
  temp[i]=0;
  if (temp[0] == 0) continue;
  if (value == 0)
    value=(START_COUNT > 0 ? count++ : count--); /* Private numbers */
  fprintf(fp1, "# define %s%s _UnifiedName(%d)\n", prefix, temp, value);
  fprintf(fp2, "  case %5d: return \"%s\";\n", value, buf);
  fprintf(fp3, "# define %s%s \"%s\"\n", prefix, temp, buf);
  num++;
 }
 fprintf (fp1, "# define %s%s _UnifiedName(%d)\n", prefix, misc, count);
 fprintf (fp2, "  default:\n\treturn \"%s\";\n  }\n}", misc);
 fprintf (fp3, "# define %s%s \"%s\"\n", prefix, misc, misc);
 fprintf(fp1, "\
\n\n\nextern \n\
#ifdef __cplusplus\n\
  \"C\" {\n\
#endif\n\
const char *_UnifiedName(int);\n\
#ifdef __cplusplus\n\
};\n\
#endif\n"); 
 fprintf(fp1, "#endif\n");
 fprintf(fp3, "#endif\n");
 printf("Processed %d entries.\n", num);
 return (0);
}

