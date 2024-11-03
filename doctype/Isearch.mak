RM=del

#
# Compiler and Compiler Flags
#
# Uncomment the appropriate entry
#

!IFNDEF WIN16
!include <ntwin32.mak>
#CFLAGS=$(cflags) $(cvarsdll)
CFLAGS=-GX $(cflags) $(cvarsmt)
!ELSE
CFLAGS=-Alfw -F 4000 -G3 -Oceglot -W3 -Zi
!ENDIF
  
#
# Source Files Directory
#
# Where are search engine sources located?
#
SRC_DIR=..\src

# 
# That should be all you need to configure
#

OBJ= dtconf.obj

H=

INC= /I ..\src

all: $(OBJ) dtconf.exe ..\src\dtreg.hxx

..\src\dtreg.hxx: dtconf.inf
        dtconf.exe

dtconf.obj:$(H) dtconf.cxx
        $(CC) $(CFLAGS) $(INC) /c dtconf.cxx

dtconf.exe:$(OBJ) dtconf.cxx
!IFNDEF WIN16
#	$(link) $(conlflags) -out:dtconf.exe $(OBJ) $(conlibsdll)
	$(link) $(conlflags) -out:dtconf.exe $(OBJ) $(conlibsmt) libcimt.lib
!ELSE
	$(CC) $(CFLAGS) -o dtconf.exe $(OBJ)
!ENDIF
        dtconf.exe

clean:
	$(RM) *.bak
	$(RM) *~
	$(RM) *.obj
	$(RM) *.pch
	$(RM) *.pdb
	$(RM) dtconf.exe

realclean:
	$(RM) *.bak
	$(RM) *~
	$(RM) *.obj
	$(RM) *.pch
	$(RM) *.pdb
	$(RM) dtconf.exe
	$(RM) ..\src\dtreg.hxx
	$(RM) ..\src\dtreg.cxx
	$(RM) ..\src\Isearch.mak

