#
RM=del

#
# Compiler and Compiler flags
#

!IFNDEF WIN16
!include <ntwin32.mak>
CFLAGS=$(cflags) $(cvarsmt) -DVERS=\"1.47j-NT\"
!ELSE
CFLAGS=-Alfw -F 4000 -G3 -Gt8192 -Ocot -W3 -Zi -DVERS=\"1.47j-NT\"
!ENDIF

#
# Install Directory
#
# Where should I install executables (make install)?
#
INSTALL=/usr/local/bin

#
# Executables Directory
#
# Where should I place executables?
#
BIN_DIR=..\exe

#
# Document Type Directory
#
# Where are document type sources located?
#
DOCTYPE_DIR=..\doctype
DOCLIB=

#
# Resulting Library
#
ISEARCH_LIB=$(BIN_DIR)\Isearch.lib

#
# Librarian
#
# Select your platform and comment/uncomment the appropriate ARFLAGS
#
AR=lib
!IFNDEF WIN16
ARFLAGS=-out:$(ISEARCH_LIB)
!ELSE
ARFLAGS=/pagesize:128 $(ISEARCH_LIB) +
!ENDIF

# 
# That should be all you need to configure
#

OBJ= defs.obj vlist.obj registry.obj opobj.obj operand.obj operator.obj \
	termobj.obj sterm.obj opstack.obj squery.obj hash.obj fprec.obj \
	fpt.obj strlist.obj dtreg.obj idb.obj string.obj common.obj \
	result.obj rset.obj iresult.obj irset.obj record.obj reclist.obj \
	mdt.obj mdtrec.obj index.obj attr.obj attrlist.obj dfd.obj \
	dfdt.obj dft.obj df.obj fc.obj fct.obj soundex.obj tokengen.obj \
	strstack.obj infix2rpn.obj glist.obj gstack.obj merge.obj \
	marc.obj marclib.obj memcntl.obj mergeunit.obj multiterm.obj \
	intlist.obj intfield.obj nlist.obj nfield.obj nfldmgr.obj \
	nlatlon.obj rcache.obj filemap.obj numsearch.obj datesearch.obj \
	geosearch.obj date.obj vidb.obj thesaurus.obj \
###DTOBJ###

H= gdt.h vlist.hxx sw.hxx opobj.hxx operand.hxx operator.hxx termobj.hxx \
	sterm.hxx opstack.hxx squery.hxx registry.hxx fprec.hxx fpt.hxx \
	strlist.hxx dtreg.hxx idb.hxx string.hxx common.hxx result.hxx \
	rset.hxx iresult.hxx irset.hxx record.hxx reclist.hxx mdt.hxx \
        mdtrec.hxx index.hxx attr.hxx attrlist.hxx dfd.hxx dfdt.hxx \
	dft.hxx df.hxx fc.hxx fct.hxx defs.hxx idbobj.hxx soundex.hxx \
        tokengen.hxx strstack.hxx infix2rpn.hxx glist.hxx gstack.hxx \
	merge.hxx marc.hxx marcdefs.hxx marclib.hxx memcntl.hxx \
	intfield.hxx intlist.hxx filemap.hxx mergeunit.hxx nlist.hxx \
	nfield.hxx nfldmgr.hxx nlatlon.hxx rcache.hxx date.hxx vidb.hxx \
	thesaurus.hxx \
###DTHXX###


INC= /I$(DOCTYPE_DIR) /I ..\src 

all: conf.h $(OBJ) $(ISEARCH_LIB) Iindex.obj Isearch.obj Iutil.obj \
     Iget.obj zsearch.obj zpresent.obj \
     $(BIN_DIR)\Iindex.exe $(BIN_DIR)\Isearch.exe $(BIN_DIR)\Iutil.exe \
     $(BIN_DIR)\Iget.exe $(BIN_DIR)\zpresent.exe $(BIN_DIR)\zsearch.exe

conf.h :
	copy confwin.h conf.h

Iindex.obj:$(H) Iindex.cxx
        $(CC) $(CFLAGS) $(INC) -c Iindex.cxx

Isearch.obj:$(H) Isearch.cxx
        $(CC) $(CFLAGS) $(INC) -c Isearch.cxx

Iutil.obj:$(H) Iutil.cxx
        $(CC) $(CFLAGS) $(INC) -c Iutil.cxx

Iget.obj:$(H) Iget.cxx
        $(CC) $(CFLAGS) $(INC) -c Iget.cxx

zsearch.obj:$(H) zsearch.cxx
        $(CC) $(CFLAGS) $(INC) -c zsearch.cxx

zpresent.obj:$(H) zpresent.cxx
        $(CC) $(CFLAGS) $(INC) -c zpresent.cxx

defs.obj:$(H) defs.cxx
        $(CC) $(CFLAGS) $(INC) -c defs.cxx

vlist.obj:$(H) vlist.cxx
        $(CC) $(CFLAGS) $(INC) -c vlist.cxx

registry.obj:$(H) registry.cxx
        $(CC) $(CFLAGS) $(INC) -c registry.cxx

opobj.obj:$(H) opobj.cxx
        $(CC) $(CFLAGS) $(INC) -c opobj.cxx

opstack.obj:$(H) opstack.cxx
        $(CC) $(CFLAGS) $(INC) -c opstack.cxx

operand.obj:$(H) operand.cxx
        $(CC) $(CFLAGS) $(INC) -c operand.cxx

operator.obj:$(H) operator.cxx
        $(CC) $(CFLAGS) $(INC) -c operator.cxx

termobj.obj:$(H) termobj.cxx
        $(CC) $(CFLAGS) $(INC) -c termobj.cxx

sterm.obj:$(H) sterm.cxx
        $(CC) $(CFLAGS) $(INC) -c sterm.cxx

fprec.obj:$(H) fprec.cxx
        $(CC) $(CFLAGS) $(INC) -c fprec.cxx

fpt.obj:$(H) fpt.cxx
        $(CC) $(CFLAGS) $(INC) -c fpt.cxx

strlist.obj:$(H) strlist.cxx
        $(CC) $(CFLAGS) $(INC) -c strlist.cxx

dtreg.obj:$(H) dtreg.cxx
        $(CC) $(CFLAGS) $(INC) -c dtreg.cxx

idb.obj:$(H) idb.cxx
        $(CC) $(CFLAGS) $(INC) -c idb.cxx

vidb.obj:$(H) vidb.cxx
        $(CC) $(CFLAGS) $(INC) -c vidb.cxx

thesaurus.obj:$(H) thesaurus.cxx
	$(CC) $(CFLAGS) $(INC) -c thesaurus.cxx

string.obj:$(H) string.cxx
        $(CC) $(CFLAGS) $(INC) -c string.cxx

strstack.obj:$(H) strstack.cxx
        $(CC) $(CFLAGS) $(INC) -c strstack.cxx
         
tokengen.obj:$(H) tokengen.cxx
        $(CC) $(CFLAGS) $(INC) -c tokengen.cxx
                  
infix2rpn.obj:$(H) infix2rpn.cxx
        $(CC) $(CFLAGS) $(INC) -c infix2rpn.cxx

common.obj:$(H) common.cxx
        $(CC) $(CFLAGS) $(INC) -c common.cxx

result.obj:$(H) result.cxx
        $(CC) $(CFLAGS) $(INC) -c result.cxx

merge.obj:$(H) merge.cxx
        $(CC) $(CFLAGS) $(INC) -c merge.cxx

rset.obj:$(H) rset.cxx
        $(CC) $(CFLAGS) $(INC) -c rset.cxx

iresult.obj:$(H) iresult.cxx
        $(CC) $(CFLAGS) $(INC) -c iresult.cxx

irset.obj:$(H) irset.cxx
        $(CC) $(CFLAGS) $(INC) -c irset.cxx

squery.obj:$(H) squery.cxx
        $(CC) $(CFLAGS) $(INC) -c squery.cxx

record.obj:$(H) record.cxx
        $(CC) $(CFLAGS) $(INC) -c record.cxx

reclist.obj:$(H) reclist.cxx
        $(CC) $(CFLAGS) $(INC) -c reclist.cxx

mdt.obj:$(H) mdt.cxx
        $(CC) $(CFLAGS) $(INC) -c mdt.cxx

mdtrec.obj:$(H) mdtrec.cxx
        $(CC) $(CFLAGS) $(INC) -c mdtrec.cxx

index.obj:$(H) index.cxx
        $(CC) $(CFLAGS) $(INC) -c index.cxx

attr.obj:$(H) attr.cxx
        $(CC) $(CFLAGS) $(INC) -c attr.cxx

attrlist.obj:$(H) attrlist.cxx
        $(CC) $(CFLAGS) $(INC) -c attrlist.cxx

dfd.obj:$(H) dfd.cxx
        $(CC) $(CFLAGS) $(INC) -c dfd.cxx

dfdt.obj:$(H) dfdt.cxx
        $(CC) $(CFLAGS) $(INC) -c dfdt.cxx

dft.obj:$(H) dft.cxx
        $(CC) $(CFLAGS) $(INC) -c dft.cxx

df.obj:$(H) df.cxx
        $(CC) $(CFLAGS) $(INC) -c df.cxx

fc.obj:$(H) fc.cxx
        $(CC) $(CFLAGS) $(INC) -c fc.cxx

fct.obj:$(H) fct.cxx
        $(CC) $(CFLAGS) $(INC) -c fct.cxx

soundex.obj:$(H) soundex.cxx
        $(CC) $(CFLAGS) $(INC) -c soundex.cxx

glist.obj:$(H) glist.cxx
        $(CC) $(CFLAGS) $(INC) -c glist.cxx

gstack.obj:$(H) gstack.cxx
        $(CC) $(CFLAGS) $(INC) -c gstack.cxx

dictionary.obj:$(H) dictionary.cxx
        $(CC) $(CFLAGS) $(INC) -c dictionary.cxx

marc.obj:$(H) marc.cxx
        $(CC) $(CFLAGS) $(INC) -c marc.cxx

marclib.obj:$(H) marclib.cxx
        $(CC) $(CFLAGS) $(INC) -c marclib.cxx

memcntl.obj:$(H) memcntl.cxx
        $(CC) $(CFLAGS) $(INC) -c memcntl.cxx

mergeunit.obj:$(H) mergeunit.cxx
        $(CC) $(CFLAGS) $(INC) -c mergeunit.cxx

multiterm.obj:$(H) multiterm.cxx
        $(CC) $(CFLAGS) $(INC) -c multiterm.cxx

intlist.obj:$(H) intlist.cxx
        $(CC) $(CFLAGS) $(INC) -c intlist.cxx

intfield.obj:$(H) intfield.cxx
        $(CC) $(CFLAGS) $(INC) -c intfield.cxx

nlist.obj:$(H) nlist.cxx
        $(CC) $(CFLAGS) $(INC) -c nlist.cxx

nfield.obj:$(H) nfield.cxx
        $(CC) $(CFLAGS) $(INC) -c nfield.cxx

nfldmgr.obj:$(H) nfldmgr.cxx
        $(CC) $(CFLAGS) $(INC) -c nfldmgr.cxx

nlatlon.obj:$(H) nlatlon.cxx
        $(CC) $(CFLAGS) $(INC) -c nlatlon.cxx

rcache.obj:$(H) rcache.cxx
        $(CC) $(CFLAGS) $(INC) -c rcache.cxx

filemap.obj:$(H) filemap.cxx
        $(CC) $(CFLAGS) $(INC) -c filemap.cxx

hash.obj:$(H) hash.cxx
        $(CC) $(CFLAGS) $(INC) -c hash.cxx

numsearch.obj:$(H) numsearch.cxx
        $(CC) $(CFLAGS) $(INC) -c numsearch.cxx

datesearch.obj:$(H) datesearch.cxx
        $(CC) $(CFLAGS) $(INC) -c datesearch.cxx

geosearch.obj:$(H) geosearch.cxx
        $(CC) $(CFLAGS) $(INC) -c geosearch.cxx

date.obj:$(H) date.cxx
        $(CC) $(CFLAGS) $(INC) -c date.cxx

###DTMAKE###

doctype.obj:$(H) $(DOCTYPE_DIR)\doctype.cxx
        $(CC) $(CFLAGS) $(INC) -c $(DOCTYPE_DIR)\doctype.cxx

$(ISEARCH_LIB):$(OBJ)
	$(RM) $(ISEARCH_LIB)
        $(AR) $(ARFLAGS) $(OBJ)

Iindex:$(BIN_DIR)\Iindex.exe
        @echo.

$(BIN_DIR)\Iindex.exe:$(ISEARCH_LIB) Iindex.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\Iindex.exe \
		Iindex.obj setargv.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\Iindex.exe \
		Iindex.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

Isearch:$(BIN_DIR)\Isearch.exe
        @echo.

$(BIN_DIR)\Isearch.exe:$(ISEARCH_LIB) Isearch.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\Isearch.exe \
		Isearch.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\Isearch.exe \
		Isearch.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

Iutil:$(BIN_DIR)\Iutil.exe
        @echo.

$(BIN_DIR)\Iutil.exe:$(ISEARCH_LIB) Iutil.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\Iutil.exe \
		Iutil.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\Iutil.exe \
		Iutil.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

Iget:$(BIN_DIR)\Iget.exe
        @echo.

$(BIN_DIR)\Iget.exe:$(ISEARCH_LIB) Iget.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\Iget.exe \
		Iget.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\Iget.exe \
		Iget.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

zsearch:$(BIN_DIR)\zsearch.exe
        @echo.

$(BIN_DIR)\zsearch.exe:$(ISEARCH_LIB) zsearch.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\zsearch.exe \
		zsearch.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\zsearch.exe \
		zsearch.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

zpresent:$(BIN_DIR)\zpresent.exe
        @echo.

$(BIN_DIR)\zpresent.exe:$(ISEARCH_LIB) zpresent.obj
!IFNDEF WIN16
	$(link) $(conlflags) -out:$(BIN_DIR)\zpresent.exe \
		zpresent.obj $(ISEARCH_LIB) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o $(BIN_DIR)\zpresent.exe \
		zpresent.obj $(ISEARCH_LIB) /link /segments:256
!ENDIF

clean:
        $(RM) *~
        $(RM) *.pdb
        $(RM) *.bak
        $(RM) *.obj
	$(RM) *.pch
        $(RM) $(BIN_DIR)\Iindex.exe
        $(RM) $(BIN_DIR)\Isearch.exe
        $(RM) $(BIN_DIR)\Iutil.exe
        $(RM) $(BIN_DIR)\Iget.exe
        $(RM) $(BIN_DIR)\zsearch.exe
        $(RM) $(BIN_DIR)\zpresent.exe
        $(RM) $(ISEARCH_LIB)

realclean:
        $(RM) *~
        $(RM) *.pdb
	$(RM) *.bak
	$(RM) *.obj
	$(RM) *.pch
	$(RM) $(BIN_DIR)\Iindex.exe
	$(RM) $(BIN_DIR)\Isearch.exe
	$(RM) $(BIN_DIR)\Iutil.exe
	$(RM) $(BIN_DIR)\Iget.exe
	$(RM) $(BIN_DIR)\zsearch.exe
	$(RM) $(BIN_DIR)\zpresent.exe
	$(RM) $(BIN_DIR)\$(ISEARCH_LIB)
	$(RM) conf.h

install:
        @echo "*** Copying Isearch executables to $(INSTALL)/. ***"
        cp $(BIN_DIR)/Iindex $(INSTALL)/.
        cp $(BIN_DIR)/Isearch $(INSTALL)/.
        cp $(BIN_DIR)/Iutil $(INSTALL)/.
        cp $(BIN_DIR)/Iget $(INSTALL)/.
        cp $(BIN_DIR)/zsearch $(INSTALL)/.
        cp $(BIN_DIR)/zpresent $(INSTALL)/.
