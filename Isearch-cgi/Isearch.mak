#
# CNIDR Isearch-cgi
#
# Answer a couple of questions and type 'make'
#

#
# Compiler and Compiler flags
#

!IFNDEF WIN16
!include <ntwin32.mak>
#CFLAGS=$(cflags) $(cvarsmt) $(cdebug) -Dstrcasecmp=stricmp -Dstrncasecmp=strnicmp -DNO_MMAP -DUNISOCK_WINSOCK
CFLAGS=$(cflags) $(cvarsmt) -DVERS=\"1.47g-NT\"
!ELSE
CFLAGS=-Alfw -F 4000 -G3 -Oceglot -W3 -Zi -Dstrcasecmp=stricmp -Dstrncasecmp=strnicmp -DNO_MMAP -DUNISOCK_WINSOCK -DFAR=_far
!ENDIF

# Isearch Source
# Where is your CNIDR Isearch code?
#
ISEARCH_DIR=..
ISEARCH_LIB=Isearch.lib
LIB_DIR=$(ISEARCH_DIR)\bin
BIN_DIR=$(ISEARCH_DIR)\bin

# cgi-bin Directory
# What is the path to your httpd server's cgi-bin directory?
#
CGIBIN_PATH=\server\cgi-bin

# Thats all!  Type 'make'
#
INC=-I$(ISEARCH_DIR)\src -I$(ISEARCH_DIR)\doctype
OBJ=cgi-util.obj
H=config.hxx
DIST=Isearch-cgi-1.47d-NT

all: isrch_srch.obj isrch_html.obj isrch_fetch.obj search_form.obj \
	cgi-util.obj isrch_srch.exe isrch_html.exe isrch_fetch.exe \
	search_form.exe done

cgi-util.obj: cgi-util.hxx cgi-util.cxx $(H)
	$(CC) $(CFLAGS) $(INC) /c cgi-util.cxx

isrch_srch.obj:isrch_srch.cxx $(H)
	$(CC) $(CFLAGS) $(INC) /c isrch_srch.cxx

isrch_html.obj:isrch_html.cxx $(H)
	$(CC) $(CFLAGS) $(INC) /c isrch_html.cxx

isrch_fetch.obj:isrch_fetch.cxx $(H)
	$(CC) $(CFLAGS) $(INC) /c isrch_fetch.cxx

search_form.obj:search_form.cxx $(H)
	$(CC) $(CFLAGS) $(INC) /c search_form.cxx

isrch_srch.exe:isrch_srch.obj $(H) $(OBJ)
!IFNDEF WIN16
#	$(link) $(conlflags) $(ldebug) -out:isrch_srch.exe isrch_srch.obj \
#		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
	$(link) $(conlflags) -out:isrch_srch.exe isrch_srch.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
!ELSE
	$(CC) $(CFLAGS) -o isrch_srch.exe isrch_srch.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib
!ENDIF

isrch_html.exe:isrch_html.obj $(H) $(OBJ)
!IFNDEF WIN16
#	$(link) $(conlflags) $(ldebug) -out:isrch_html.exe isrch_html.obj \
#		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
	$(link) $(conlflags) -out:isrch_html.exe isrch_html.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
!ELSE
	$(CC) $(CFLAGS) -o isrch_html.exe isrch_html.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib
!ENDIF

isrch_fetch.exe:isrch_fetch.obj $(H) $(OBJ)
!IFNDEF WIN16
#	$(link) $(conlflags) $(ldebug) -out:isrch_fetch.exe isrch_fetch.obj \
#		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
	$(link) $(conlflags) -out:isrch_fetch.exe isrch_fetch.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
!ELSE
	$(CC) $(CFLAGS) -o isrch_fetch.exe isrch_fetch.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib
!ENDIF

search_form.exe:search_form.obj $(H) $(OBJ)
!IFNDEF WIN16
#	$(link) $(conlflags) $(ldebug) -out:search_form.exe search_form.obj \
#		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
	$(link) $(conlflags) -out:search_form.exe search_form.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib libcimt.lib $(conlibs)
!ELSE
	$(CC) $(CFLAGS) -o search_form.exe search_form.obj \
		$(OBJ) $(LIB_DIR)\Isearch.lib
!ENDIF

done:
	@echo.
	@echo Welcome to CNIDR Isearch-cgi!
	@echo.
	@echo Read the README file for configuration and installation instructions
	@echo.

clean:
	del *~
	del *.bak
	del *.obj
	del *.pdb
	del isrch_srch.exe
	del isrch_html.exe
	del isrch_fetch.exe
	del search_form.exe
	del *.htm

realclean:
	del *~
	del *.bak
	del *.obj
	del *.pdb
	del isrch_srch.exe
	del isrch_html.exe
	del isrch_fetch.exe
	del search_form.exe
	del *.htm

install:
	cp isearch ifetch $(CGIBIN_PATH)

