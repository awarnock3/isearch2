#
# Compiling and Installing Isearch
#
# 1)  Type `make'
#
# 2)  Type `make install' (as root), to copy executables to /usr/local/bin/.
#       or `make install INSTALL=~/bin' to copy them to ~/bin/.
#       etc.
#
# 3)  Typing `make clean' will clean up .o files.
#
# The executables are
#
#	Iindex	- command line indexing utility
#	Isearch	- command line searching utility
#	Iutil - command line utilities for Isearch databases
#	Iget - command line document retrieval utility
#       zsearch - XML command line searching utility
#       zpresent - XML command line document retrieval utility
#
SHELL=/bin/sh

#
# Compiler
#
CC=c++

DTCC=c++
#CC=/sw/CenterLine/bin/CC

#
# Compiler flags?
#
# Select your platform and comment/uncomment the appropriate CFLAGS
# 
# -DVERBOSE makes Iindex tell you when it's parsing docs and fields
# -DMULTI compiles code to handle documents consisting of multiple files
#         in a single subdirectory
#
# for Solaris, SunOS, Ultrix, OSF
#
#CFLAGS=-O2 -DUNIX -DMULTI
#CFLAGS=-O2 -DUNIX

#
# for Linux
#
CFLAGS=-O2 -std=c++17 -DUNIX
#CFLAGS=-g -fwritable-strings -Wall -Wno-unused -DUNIX # -DVERBOSE -DDEBUG

#
# for HP
# - for some odd reason, compiling with optimization (-O2) implants
#   a bug in INDEX::TermSearch, so don't optimize.
#
#CFLAGS=-DUNIX -D_HP_ -DNO_MMAP

# 
# for AIX
#
#CFLAGS=-O2 -DUNIX -DNOCACHE -D_BSD

#
# for DG
# - same comment as for HP
#CFLAGS=-DUNIX

#
# for SGI C++
#
#CC=CC
#CFLAGS=-O2 -DUNIX -DSGI_CC -woff 3262 -32

#
# Install Directory
#
# Where should I install executables (make install)?
#
INSTALL=/usr/local/bin
CGI_INSTALL=/home/httpd/cgi-bin

#
# Executables Directory
#
# Where should I place executables?
#
BIN_DIR=bin

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
# Isearch-cgi Directory
#
# Where is the CGI gateway code?
#
CGI_DIR=Isearch-cgi

# 
# That should be all you need to configure
#

RM = rm -f
LDFLAGS=
VER=2.00
#DIST=Isearch-$(VER)
DIST=Isearch
BINDIST=$(DIST)-bin
freezename=`echo $(VER) | sed 's/\./-/g'`
OSNAME=`uname -s`
OSVER=`uname -r`
OS=$(OSNAME)_$(OSVER)

all: isearch isearch-cgi done

isearch::
	`if [ ! -f src/conf.h ] ; \
		then echo ./configure ; \
	fi`
	+cd $(DOCTYPE_DIR); make "CC=$(CC)" \
		CFLAGS="$(CFLAGS) -DVERS=\\\"$(VER)\\\" "
	+cd $(SRC_DIR); make "BIN_DIR=../$(BIN_DIR)" \
			"DOCTYPE_DIR=../$(DOCTYPE_DIR)" \
			CFLAGS="$(CFLAGS) -DVERS=\\\"$(VER)\\\" " \
			"CC=$(CC)" "DOCLIB=$(DOCLIB)" "LDFLAGS=$(LDFLAGS)"

isearch-cgi::
	+cd $(CGI_DIR); make "BIN_DIR=../$(BIN_DIR)" "VER=$(VER)" \
			"LIB_DIR=../$(BIN_DIR)" "ISEARCH_DIR=.." \
			CFLAGS="$(CFLAGS) -DVERS=\\\"$(VER)\\\"" \
			"CC=$(CC)" "DOCLIB=$(DOCLIB)" "LDFLAGS=$(LDFLAGS)"

done:
	@echo ""
	@echo "Welcome to CNIDR $(DIST), release $(VER)!"
	@echo ""
	@echo "Read the README file for configuration and installation instructions"
	@echo ""

clean:
	$(RM) *~ $(BIN_DIR)/Iindex $(BIN_DIR)/Isearch $(BIN_DIR)/Iutil \
		$(BIN_DIR)/Iget $(BIN_DIR)/libIsearch.a $(BIN_DIR)/core
	+cd $(SRC_DIR); make -i clean
	+cd $(DOCTYPE_DIR); make -i clean
	+cd $(CGI_DIR); make -i clean

realclean:
	$(RM) *~ $(BIN_DIR)/Iindex $(BIN_DIR)/Isearch $(BIN_DIR)/Iutil \
		$(BIN_DIR)/libIsearch.a $(BIN_DIR)/core config.* \
		$(BIN_DIR)/Iget Makefile.all
	+cd $(SRC_DIR); make -i realclean
	+cd $(DOCTYPE_DIR); make -i clean
	+cd $(CGI_DIR); make -i clean

distclean:
	$(RM) *~ $(BIN_DIR)/Iindex $(BIN_DIR)/Isearch $(BIN_DIR)/Iutil \
		$(BIN_DIR)/libIsearch.a $(BIN_DIR)/core config.* \
		$(BIN_DIR)/Iget Makefile.all
	+cd $(SRC_DIR); make -i distclean
	+cd $(DOCTYPE_DIR); make -i clean
	+cd $(CGI_DIR); make -i clean

binclean:
	$(RM) *~ $(BIN_DIR)/Iindex $(BIN_DIR)/Isearch $(BIN_DIR)/Iutil \
		$(BIN_DIR)/libIsearch.a $(BIN_DIR)/core $(BIN_DIR)/Iget 
	cd $(CGI_DIR);$(RM) isrch_fetch isrch_srch isrch_html search_form

build:
	`if [ -e src/conf.h ] ; \
		then make -i realclean; \
		else true; \
	fi`
	make all

install:
	@echo "*** Copying Isearch executables to $(INSTALL) ***"
	cp $(BIN_DIR)/Iindex $(INSTALL)
	cp $(BIN_DIR)/Isearch $(INSTALL)
	cp $(BIN_DIR)/Iutil $(INSTALL)
	cp $(BIN_DIR)/Iget $(INSTALL)
	cp $(BIN_DIR)/zsearch $(INSTALL)
	cp $(BIN_DIR)/zpresent $(INSTALL)
	@echo ""
	@echo "To install Isearch-cgi, cd into the Isearch-cgi directory"
	@echo "Then run the configure script"

dist:
	make -i distclean
	rm -f *~ $(SRC_DIR)/*~ $(DOCTYPE_DIR)/*~ \
		*.bak $(SRC_DIR)/*.bak $(DOCTYPE_DIR)/*.bak \
		$(SRC_DIR)/ISEARCH.*
	ls -F $(SRC_DIR)
	rm -fr ../$(DIST)
	mkdir ../$(DIST)
#	cd $(SRC_DIR); rcsfreeze $(freezename); co -r$(freezename) RCS/*,v
	find . -name \* -print | grep -v '^RCS/' > .tmpdist-$(VER)
	tar cf - `cat .tmpdist-$(VER)` | (cd ../$(DIST); tar xf -)
	cd ..; tar cf $(DIST).tar $(DIST)
	rm -f ../$(DIST).tar.gz
	cd ..; gzip $(DIST).tar
	cp ./CHANGES ../.

srcdist:
	make -i distclean;cd ..;tar cvf $(DIST)-$(VER).tar $(DIST);gzip $(DIST)-$(VER).tar

bindist:
	cd ..; \
	strip $(DIST)/bin/Iindex; \
	strip $(DIST)/bin/Isearch; \
	strip $(DIST)/bin/Iutil; \
	strip $(DIST)/bin/Iget; \
	strip $(DIST)/bin/zsearch; \
	strip $(DIST)/bin/zpresent; \
	strip $(DIST)/Isearch-cgi/isrch_fetch; \
	strip $(DIST)/Isearch-cgi/isrch_srch; \
	strip $(DIST)/Isearch-cgi/isrch_html; \
	strip $(DIST)/Isearch-cgi/search_form; \
	tar cvf $(DIST)-$(VER)_$(OS)$(LDFLAGS).tar \
		$(DIST)/bin/Iindex \
		$(DIST)/bin/Isearch \
		$(DIST)/bin/Iutil \
		$(DIST)/bin/Iget \
		$(DIST)/bin/zsearch \
		$(DIST)/bin/zpresent \
		$(DIST)/Isearch-cgi/Configure \
		$(DIST)/Isearch-cgi/README \
		$(DIST)/Isearch-cgi/isrch_fetch \
		$(DIST)/Isearch-cgi/isrch_srch \
		$(DIST)/Isearch-cgi/isrch_html \
		$(DIST)/Isearch-cgi/search_form \
		$(DIST)/doc \
		$(DIST)/README \
		$(DIST)/CHANGES \
		$(DIST)/html  \
		$(DIST)/COPYRIGHT; \
		gzip $(DIST)-$(VER)_$(OS)$(LDFLAGS).tar

#
# Isearch2 cleanup: Catch2 unit tests (see CLAUDE.md TESTING)
#
CATCH2_DIR := tests/vendor/catch2
TEST_SRCS  := $(shell find tests -name '*.cxx')
TEST_CXXFLAGS := -std=c++17 -Wall -Wextra -DUNIX -Isrc -Idoctype -IIsearch-cgi -I$(CATCH2_DIR)

# Real engine sources that processed files' tests link against directly,
# so tests exercise actual behavior instead of reimplementing it. Grows
# as each file's turn adds tests that need more of the engine; compiled
# separately from the production build (tests/obj/, TEST_CXXFLAGS) so the
# two builds never fight over the same .o.
TEST_ENGINE_SRCS := src/fc.cxx src/string.cxx src/common.cxx
TEST_ENGINE_OBJS := $(patsubst src/%.cxx,tests/obj/%.o,$(TEST_ENGINE_SRCS))

TEST_OBJS := $(TEST_SRCS:.cxx=.o) $(TEST_ENGINE_OBJS) $(CATCH2_DIR)/catch_amalgamated.o

$(CATCH2_DIR)/catch_amalgamated.o: $(CATCH2_DIR)/catch_amalgamated.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/obj/%.o: src/%.cxx
	@mkdir -p tests/obj
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/%.o: tests/%.cxx
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests: $(TEST_OBJS)
	@mkdir -p tests/reports
	$(CXX) $(TEST_CXXFLAGS) $(TEST_OBJS) -o tests/run_tests
	@REPORT=tests/reports/report-$$(date +%Y%m%d-%H%M%S).txt; \
	tests/run_tests | tee $$REPORT; \
	echo "Report saved to $$REPORT"

tests-asan: TEST_CXXFLAGS += -fsanitize=address,undefined -g
tests-asan: tests

# "tests" collides with the tests/ directory that already exists on
# disk, and "tests-asan" only differs from "tests" by a variable
# addition make can't see in file timestamps -- without .PHONY, make
# considers both satisfied as soon as tests/ exists and TEST_OBJS look
# up to date, and silently skips rebuilding/relinking.
.PHONY: tests tests-asan
