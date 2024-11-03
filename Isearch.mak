#
# Compiling and Installing Isearch
#
# 1)  Type `make'
#
# 2)  Typing `make clean' will clean up .obj files.
#
# The executables are
#
#	Iindex	- command line indexing utility
#	Isearch	- command line searching utility
#	Iutil   - command line utilities for Isearch databases
#

DIST=Isearch
VER=1.47j-NT

RM = del

#
# Executables Directory
#
# Where should I place executables?
#
BIN_DIR=exe
LIB_DIR=exe

#
# Document Type Directory
#
# Where are document type sources located?
#
DOCTYPE_DIR=doctype

#
# Doctype dependent libraries (-lgdbm, etc)
DOCLIB=

#
# Source Directory
#
# Where are search engine sources located?
#
SRC_DIR=src

#
# Where is Isearch-cgi?
#
CGI_DIR=Isearch-cgi

# 
# That should be all you need to configure
#

all: isearch done

isearch:
	cd $(DOCTYPE_DIR)
	$(MAKE) -$(MAKEFLAGS) -f Isearch.mak
	cd ..
	cd $(SRC_DIR)
	$(MAKE) -$(MAKEFLAGS) -f Isearch.mak \
		"BIN_DIR=..\$(BIN_DIR)" \
		"DOCTYPE_DIR=..\$(DOCTYPE_DIR)" \
		"DOCLIB=$(DOCLIB)"
	cd ..

cgi:
	cd $(CGI_DIR)
	$(MAKE) /I -$(MAKEFLAGS) -f Isearch.mak \
		BIN_DIR="..\$(BIN_DIR)" \
		LIB_DIR="..\$(LIB_DIR)" \
		DOCTYPE_DIR="..\$(DOCTYPE_DIR)"
	cd ..


done:
	@echo.
	@echo Welcome to CNIDR $(DIST), release $(VER)!
	@echo.
	@echo Read the README file for configuration and installation instructions
	@echo.

clean:
	$(RM) *~
	$(RM) *.bak
	$(RM) $(BIN_DIR)\Iindex.exe
	$(RM) $(BIN_DIR)\Isearch.exe
	$(RM) $(BIN_DIR)\Iutil.exe
	$(RM) $(BIN_DIR)\Iget.exe
	$(RM) $(BIN_DIR)\zsearch.exe
	$(RM) $(BIN_DIR)\zpresent.exe
	$(RM) $(BIN_DIR)\Isearch.lib
	cd $(SRC_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch.mak clean
	cd ..
	cd $(DOCTYPE_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch.mak clean
	cd ..
	cd $(CGI_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch.mak clean
	cd ..

realclean:
	$(RM) *~
	$(RM) *.bak
	$(RM) $(BIN_DIR)\Iindex.exe
	$(RM) $(BIN_DIR)\Isearch.exe
	$(RM) $(BIN_DIR)\Iutil.exe
	$(RM) $(BIN_DIR)\Iget.exe
	$(RM) $(BIN_DIR)\zsearch.exe
	$(RM) $(BIN_DIR)\zpresent.exe
	$(RM) $(BIN_DIR)\Isearch.lib
	cd $(SRC_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch0.mak realclean
	cd ..
	cd $(DOCTYPE_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch.mak realclean
	cd ..
	cd $(CGI_DIR)
	$(MAKE) -$(MAKEFLAGS) -i -f Isearch.mak realclean
