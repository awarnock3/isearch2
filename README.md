# Isearch2

Isearch2 is a C++ search/indexing engine and toolset for local document collections. It supports full-text and fielded search, Boolean queries, ranked results, and multiple document parsers ("doctypes"), plus optional CGI programs for web-based search.

## Current status

- Modernized to build with **ISO C++17** defaults in this repository.
- Includes a built-in **Markdown doctype** (`doctype/markdown.cxx`), enabled in `doctype/dtconf.inf`.

## Repository layout

- `src/` - core engine and CLI entry points.
- `doctype/` - pluggable document parsers and presentation logic.
- `Isearch-cgi/` - CGI frontends (`isrch_srch`, `isrch_fetch`, `isrch_html`, `search_form`).
- `data/` - sample text corpus for local smoke tests.
- `bin/` - build output binaries and static library.
- `doc/`, `html/`, `TUTORIAL*`, `CHANGES`, `COPYRIGHT` - reference docs and history.

## Build prerequisites

- Linux/Unix-like environment
- `make`
- C/C++ toolchain (`gcc`/`g++` or compatible)
- shell tools used by the build system

## Build and clean

```bash
make
```

Builds:
- core static library: `bin/libIsearch.a`
- CLI tools: `bin/Iindex`, `bin/Isearch`, `bin/Iutil`, `bin/Iget`, `bin/zsearch`, `bin/zpresent`
- CGI tools in `Isearch-cgi/`

Clean targets:

```bash
make clean
make realclean
```

## Install

Default install path:

```bash
make install
```

Custom install path:

```bash
make install INSTALL=/your/bin/path
```

## Quick start (local smoke test)

Index sample files and run a query:

```bash
./bin/Iindex -d /tmp/ISEARCH_SMOKE ./data/*.txt
./bin/Isearch -d /tmp/ISEARCH_SMOKE dust
```

## Core CLI tools

- `Iindex` - build/update indexes from source documents.
- `Isearch` - execute full-text and fielded searches.
- `Iutil` - maintenance/inspection operations on databases.
- `Iget` - record/document retrieval utility.
- `zsearch`, `zpresent` - XML-oriented search/presentation tools.

## Doctype system

Doctypes determine how documents are parsed, indexed, and presented. Enabled doctypes are configured in:

- `doctype/dtconf.inf`

The doctype config generator (`doctype/dtconf`) creates build artifacts in `src/`:

- `src/dtreg.cxx`
- `src/dtreg.hxx`
- `src/Makefile`

Treat those generated files as outputs; edit `doctype/dtconf.inf` and templates instead.

## CGI tools

`Isearch-cgi/` provides CGI programs and scripts for browser-based querying. Typical flow:

1. Build core engine (`make` at repo root).
2. Build CGI binaries (`cd Isearch-cgi && make` or from top-level `make`).
3. Configure/deploy CGI scripts per your web server setup.

See `Isearch-cgi/README` for legacy CGI deployment details and parameters.

## Database model

Commands use a database root via `-d` (path + stem), and Isearch manages related files (`.mdt`, `.inx*`, `.dfd`, etc.) using that root name.

## Historical credits

Isearch was originally developed through contributions from MCNC/CNIDR, Etymon, and community contributors, with support from the National Science Foundation (NCR-9216963).
