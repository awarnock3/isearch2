#
# CNIDR Isearch-cgi
#
# Answer a couple of questions and type 'make'
#
VER=1.47g

# Compiler  
# Must be an ANSI c++ compiler
#
CC=g++

# Compiler Flags
# Nothing fancy
#
#CFLAGS=-g -std=c++17 -Wall
CFLAGS=-O2 -std=c++17 -W -DUNIX -DVERS=\\\"$(VER)\\\"

# Isearch Source
# Where is your CNIDR Isearch code?
#
ISEARCH_DIR=..
ISEARCH_LIB=libIsearch.a
LIB_DIR=$(ISEARCH_DIR)/bin
BIN_DIR=$(ISEARCH_DIR)/bin
BIN_PARENT=$(BIN_DIR)/..

# cgi-bin Directory
# What is the path to your httpd server's cgi-bin directory?
#
# for Redhat Linux
CGIBIN_PATH=/home/httpd/cgi-bin

# for Debian Linux
#CGIBIN_PATH=/usr/lib/cgi-bin
# NCSA httpd default
CGIBIN_PATH=/usr/local/etc/httpd/cgi-bin

# Thats all!  Type 'make'
#
LDFLAGS=
INC=-I$(ISEARCH_DIR)/src -I$(ISEARCH_DIR)/doctype
LIB=-lIsearch -lm
OBJ=cgi-util.o config.o
H=config.hxx
DIST=Isearch-cgi-$(VER)

# all: $(LIB_DIR)/$(ISEARCH_LIB) isrch_srch.o isrch_fetch.o \
#     search_form.o cgi-util.o isrch_srch isrch_fetch search_form done

all: isrch_srch isrch_fetch isrch_html search_form isrch_api done

config.o: config.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c config.cxx

cgi-util.o: cgi-util.hxx cgi-util.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c cgi-util.cxx

isrch_srch.o:isrch_srch.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c isrch_srch.cxx

isrch_fetch.o:isrch_fetch.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c isrch_fetch.cxx

isrch_html.o:isrch_html.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c isrch_html.cxx

isrch_api.o:isrch_api.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c isrch_api.cxx

search_form.o:search_form.cxx $(H)
	$(CC) $(CFLAGS) $(INC) -o $@ -c search_form.cxx

api_config.o: api_config.cxx api_config.hxx
	$(CC) $(CFLAGS) $(INC) -o $@ -c api_config.cxx

api_response.o: api_response.cxx api_response.hxx
	$(CC) $(CFLAGS) $(INC) -o $@ -c api_response.cxx

api_request.o: api_request.cxx api_request.hxx
	$(CC) $(CFLAGS) $(INC) -o $@ -c api_request.cxx

api_search.o: api_search.cxx api_search.hxx
	$(CC) $(CFLAGS) $(INC) -o $@ -c api_search.cxx

api_endpoints.o: api_endpoints.cxx api_endpoints.hxx
	$(CC) $(CFLAGS) $(INC) -o $@ -c api_endpoints.cxx

isrch_srch:isrch_srch.o $(H) $(OBJ) $(LIB_DIR)/$(ISEARCH_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ isrch_srch.o $(OBJ) \
		-L$(LIB_DIR) $(LIB)

isrch_fetch:isrch_fetch.o $(H) $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ isrch_fetch.o $(OBJ) \
		-L$(LIB_DIR) $(LIB)

isrch_html:isrch_html.o $(H) $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ isrch_html.o $(OBJ) \
		-L$(LIB_DIR) $(LIB)

isrch_api:isrch_api.o api_config.o api_response.o api_request.o api_search.o api_endpoints.o cgi-util.o config.o $(LIB_DIR)/$(ISEARCH_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ isrch_api.o api_config.o api_response.o api_request.o api_search.o api_endpoints.o cgi-util.o config.o \
		-L$(LIB_DIR) $(LIB)

search_form:search_form.o $(H) $(OBJ) # $(LIB_DIR)/$(ISEARCH_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ search_form.o $(OBJ) \
		-L$(LIB_DIR) $(LIB)

$(ISEARCH_LIB):$(LIB_DIR)/$(ISEARCH_LIB)
	@echo ""

$(LIB_DIR)/$(ISEARCH_LIB):
	@echo "You need to make Isearch"

done:
	@echo ""
	@echo "Welcome to CNIDR Isearch-cgi version $(VER)!"
	@echo ""
	@echo "Read the README file for configuration and installation instructions"
	@echo ""

srcdist:
	cp README ../$(DIST).README;make clean;\
		cd ..;tar cvf $(DIST).tar $(DIST);gzip $(DIST).tar

clean:
	rm -f *.o *~ isrch_srch isrch_fetch isrch_html isrch_api search_form \
		isearch ifetch ihtml *.html

install:
	@echo "Run the Configure script in the Isearch-cgi subdirectory"
	@echo "to generate the CGI scripts."
	@echo ""
