# Copilot instructions for Isearch2

## Build, test, and lint commands

This repository uses GNU make/autoconf and C++ sources (`.cxx`/`.hxx`).

- First-time setup (generates `src/conf.h` and `src/Makefile.000`):
  - `./configure`
- Build core engine + CGI tools:
  - `make`
- Clean build artifacts:
  - `make clean`
- Install CLI binaries (`Iindex`, `Isearch`, `Iutil`, `Iget`, `zsearch`, `zpresent`):
  - `make install`
  - custom path: `make install INSTALL=/your/bin/path`
- Build CGI tools only (after core build):
  - `cd Isearch-cgi && make`

There's no lint target, but there are two automated Makefile targets
now (see CLAUDE.md for the full cleanup-effort context both live in):
`make tests`/`make tests-asan` (Catch2 unit tests), and `make
smoke-test` (builds `isearch`/`isearch-cgi`, indexes the sample corpus,
and verifies each document is actually findable by search).

To run the smoke test by hand instead: note the sample `.txt` files
live under `data/TEXT/`, not directly in `data/`.

```bash
./bin/Iindex -d /tmp/ISEARCH_SMOKE ./data/TEXT/*.txt && ./bin/Isearch -d /tmp/ISEARCH_SMOKE dust
```

## High-level architecture

- **Entry points (`src/`)**: CLI programs (`Iindex`, `Isearch`, `Iutil`, `Iget`, `zsearch`, `zpresent`) are thin frontends that parse flags and delegate to DB/search classes.
- **Core DB/search engine (`src/`)**:
  - `IDB` manages database files, indexing queues, metadata tables, and result presentation.
  - `INDEX` executes term/boolean/date/numeric/spatial searches.
  - `SQUERY`, `IRSET`, `RSET`, `RESULT`, `RECORD` carry parsed query state and search results.
- **Document-type plugin layer (`doctype/`)**:
  - `DOCTYPE` is the base parser/presenter contract.
  - Specific doctypes (`html`, `mailfolder`, `usmarc`, etc.) implement field parsing and rendering behavior.
  - `dtconf` reads `doctype/dtconf.inf`, then generates `src/dtreg.cxx`/`src/dtreg.hxx` and `src/Makefile` from templates so selected doctypes are compiled into the engine.
- **Virtual database aggregation**:
  - `VIDB` wraps one or more physical `IDB` databases.
  - If `<db>.vdb` exists, each line points to a constituent DB, and searches merge results across all of them.
- **Web/CGI integration (`Isearch-cgi/`)**:
  - CGI executables (`isrch_srch`, `isrch_fetch`, `isrch_html`, `search_form`) link against `bin/libIsearch.a`.

## Key conventions

- **Use generated files as build outputs**:
  - Do not hand-edit `src/dtreg.cxx`, `src/dtreg.hxx`, or generated `src/Makefile`; update `doctype/dtconf.inf` and templates (`src/Makefile.000.in`) instead.
- **Database naming model**:
  - `-d` always takes a database root (path + stem), and the engine manages multiple files via known extensions (`.mdt`, `.inx`, `.dfd`, etc.).
- **Custom core types and string API**:
  - Prefer project types (`STRING`, `STRLIST`, `INT`, `GPTYPE`, `GDT_BOOLEAN`) over std replacements in core code.
  - Case-insensitive equality often uses `CaseEquals(...)` or `operator^=` on `STRING`.
- **Doctype hooks drive indexing and presentation**:
  - `BeforeIndexing`/`ParseRecords`/`ParseWords`/`ParseFields` affect indexed content.
  - `BeforeSearching`/`AfterSearching`/`Present` affect query interpretation and output formatting.
