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
TEST_CXXFLAGS      := -std=c++17 -Wall -Wextra -DUNIX -DVERS=\"$(VER)\" -Isrc -Idoctype -IIsearch-cgi -I$(CATCH2_DIR)
TEST_CXXFLAGS_ASAN := $(TEST_CXXFLAGS) -fsanitize=address,undefined -g

# Real engine sources that processed files' tests link against directly,
# so tests exercise actual behavior instead of reimplementing it. Grows
# as each file's turn adds tests that need more of the engine; compiled
# separately from the production build (tests/obj/, TEST_CXXFLAGS) so the
# two builds never fight over the same .o.
TEST_ENGINE_SRCS := src/fc.cxx src/fct.cxx src/vlist.cxx src/df.cxx src/dft.cxx src/string.cxx src/common.cxx src/record.cxx src/rcache.cxx src/irset.cxx src/operand.cxx src/opobj.cxx src/rset.cxx src/result.cxx src/iresult.cxx src/attr.cxx src/attrlist.cxx src/mdtrec.cxx src/mdt.cxx src/dfd.cxx src/dfdt.cxx src/strlist.cxx src/defs.cxx src/opstack.cxx src/filemap.cxx src/hash.cxx src/termobj.cxx src/memcntl.cxx src/operator.cxx src/sterm.cxx src/marclib.cxx src/md5.cxx src/registry.cxx src/fprec.cxx
TEST_ENGINE_OBJS      := $(patsubst src/%.cxx,tests/obj/%.o,$(TEST_ENGINE_SRCS))
TEST_ENGINE_OBJS_ASAN := $(patsubst src/%.cxx,tests/obj-asan/%.o,$(TEST_ENGINE_SRCS))

# Same idea as TEST_ENGINE_SRCS above, for engine sources living outside
# src/ (e.g. Isearch-cgi/). Kept as a separate list/pattern rule because
# TEST_ENGINE_OBJS's patsubst assumes a src/ prefix.
TEST_ENGINE_CGI_SRCS := Isearch-cgi/config.cxx
TEST_ENGINE_CGI_OBJS      := $(patsubst Isearch-cgi/%.cxx,tests/obj/cgi-%.o,$(TEST_ENGINE_CGI_SRCS))
TEST_ENGINE_CGI_OBJS_ASAN := $(patsubst Isearch-cgi/%.cxx,tests/obj-asan/cgi-%.o,$(TEST_ENGINE_CGI_SRCS))

# BUGFIX (Isearch2 cleanup automation turn, see docs/BUG_CATALOG.md): the
# plain and ASan builds used to compile into the SAME object paths, with
# `tests-asan: TEST_CXXFLAGS += -fsanitize...` relying on a
# target-specific variable to change flags at recipe time. That can't
# change WHICH paths are prerequisites (TEST_OBJS is `:=`, expanded once
# at parse time), and Make's staleness check only looks at file
# timestamps, not flags -- so a plain object left over from `make tests`
# looked up-to-date to `make tests-asan` and got silently relinked
# uninstrumented (confirmed via `nm` showing no asan symbols). Hence the
# old clean-test-objs force-clean before every single build. Giving the
# ASan build entirely separate, textually-disjoint object paths fixes
# the root cause instead: `tests/obj-asan/` for engine objects (mirrors
# `tests/obj/`, same `cgi-` disambiguation trick already used below) and
# `.o.asan` -- deliberately NOT ending in `.o`, so it can never be
# mistaken for a plain-build target -- for in-place test-file objects.
# The two builds can now never collide, so Make's ordinary incremental
# rebuild just works, and neither one needs to force-clean anything.
TEST_OBJS      := $(TEST_SRCS:.cxx=.o) $(TEST_ENGINE_OBJS) $(TEST_ENGINE_CGI_OBJS) $(CATCH2_DIR)/catch_amalgamated.o
TEST_OBJS_ASAN := $(TEST_SRCS:.cxx=.o.asan) $(TEST_ENGINE_OBJS_ASAN) $(TEST_ENGINE_CGI_OBJS_ASAN) $(CATCH2_DIR)/catch_amalgamated-asan.o

$(CATCH2_DIR)/catch_amalgamated.o: $(CATCH2_DIR)/catch_amalgamated.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

$(CATCH2_DIR)/catch_amalgamated-asan.o: $(CATCH2_DIR)/catch_amalgamated.cpp
	$(CXX) $(TEST_CXXFLAGS_ASAN) -c $< -o $@

tests/obj/%.o: src/%.cxx
	@mkdir -p tests/obj
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/obj-asan/%.o: src/%.cxx
	@mkdir -p tests/obj-asan
	$(CXX) $(TEST_CXXFLAGS_ASAN) -c $< -o $@

tests/obj/cgi-%.o: Isearch-cgi/%.cxx
	@mkdir -p tests/obj
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/obj-asan/cgi-%.o: Isearch-cgi/%.cxx
	@mkdir -p tests/obj-asan
	$(CXX) $(TEST_CXXFLAGS_ASAN) -c $< -o $@

tests/%.o: tests/%.cxx
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/%.o.asan: tests/%.cxx
	$(CXX) $(TEST_CXXFLAGS_ASAN) -c $< -o $@

tests: $(TEST_OBJS)
	@mkdir -p tests/reports
	$(CXX) $(TEST_CXXFLAGS) $(TEST_OBJS) -o tests/run_tests
	@REPORT=tests/reports/report-$$(date +%Y%m%d-%H%M%S).txt; \
	tests/run_tests | tee $$REPORT; \
	echo "Report saved to $$REPORT"

tests-asan: $(TEST_OBJS_ASAN)
	@mkdir -p tests/reports
	$(CXX) $(TEST_CXXFLAGS_ASAN) $(TEST_OBJS_ASAN) -o tests/run_tests-asan
	@REPORT=tests/reports/report-asan-$$(date +%Y%m%d-%H%M%S).txt; \
	tests/run_tests-asan | tee $$REPORT; \
	echo "Report saved to $$REPORT"

.PHONY: tests tests-asan

#
# Isearch2 cleanup: integration smoke test (see CLAUDE.md)
#
# Unlike `tests`/`tests-asan` above (which only link a growing subset of
# engine sources, TEST_ENGINE_SRCS), this builds the REAL production
# binaries -- the full tree, processed and not-yet-processed files
# together -- and exercises them the way an actual user would: index a
# real corpus, then confirm each document is actually findable by
# search. Complements the unit tests; doesn't replace them. Manual/
# on-demand only -- not run automatically by /process-next or
# /process-5.
#
# NB: `grep` in this environment is `ugrep`, which silently treats some
# of the sample FGDC-metadata .txt files as binary and skips them
# unless given `-a`/`--text` -- hence the `-a` below. Don't drop it.
SMOKE_DB := /tmp/ISEARCH_SMOKE

smoke-test: isearch isearch-cgi
	$(RM) -f $(SMOKE_DB).*
	./bin/Iindex -d $(SMOKE_DB) data/TEXT/*.txt
	@echo "--- verifying indexed documents are findable ---"
	@FAIL=0; \
	for pair in Watersheds:cgia-wswtemp Oceanography:dds10 Dust:dust \
	            glaciers:glaciers Naval:goes_9_conus; do \
	  term=$${pair%%:*}; expect=$${pair##*:}; \
	  out=$$(./bin/Isearch -d $(SMOKE_DB) -t "$$term"); \
	  if echo "$$out" | grep -aqi "$$expect"; then \
	    echo "  OK    $$term -> $$expect"; \
	  else \
	    echo "  FAIL  $$term -> expected '$$expect' in results, got:"; \
	    echo "$$out" | sed 's/^/        /'; \
	    FAIL=1; \
	  fi; \
	done; \
	exit $$FAIL

.PHONY: smoke-test
