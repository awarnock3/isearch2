# Isearch2

Legacy C++ text-retrieval/indexing engine undergoing a full-tree cleanup:
refactor, document, and test every file in `src/`, `doctype/`, and
`Isearch-cgi/`, one file at a time, driven by operator commands from the
Claude Code tab in VS Code.

## History / status — read before the first ANALYZE

- The `STRING` module (`string.hxx`/`.cxx` + a scoped slice of
  `common.hxx`/`.cxx`) was previously cleaned up via a **parallel
  directory** (`isearch2-modern/`), kept separate from this tree so it
  could be verified in isolation. **Assumption, please confirm:** unless
  that work has already been folded back into this tree per its own
  README ("Toward the full tree" section), the copies of those files
  living in this tree's `src/` are still the *original, unfixed*
  versions, and ANALYZE will queue them like everything else. If you've
  already merged the fix back in, say so before the first ANALYZE.
- Everything from here on happens **in place**, file by file, on a
  dedicated branch, in your fork. This is intended to become the
  proposed change set your colleague reviews and merges into the main
  repository — see GIT below.
- **Git checkpointing, branching, and pushing are all automatic.**
  Claude commits before and after touching each file and pushes every
  commit to your fork. You never need to remember to commit yourself.

## Directory structure

```
src/            Core engine and CLI entry points
doctype/        Pluggable document parsers and presentation logic
Isearch-cgi/    CGI frontends: isrch_srch.cxx, isrch_fetch.cxx,
                isrch_html.cxx, search_form.cxx
data/           Sample text corpus for local smoke tests
bin/            Build output binaries and static library — never hand-edited
doc/, html/,    Reference docs and history — not part of the cleanup scope
TUTORIAL*, CHANGES, COPYRIGHT
tests/          NEW. Sibling to src/, not nested inside it. Mirrors the
                relative paths of src/, doctype/, Isearch-cgi/. Also
                holds tests/vendor/catch2/ (see TESTING) and
                tests/reports/.
docs/           NEW. PROCESSING_STATUS.md and BUG_CATALOG.md (see below).
```

Only `src/`, `doctype/`, and `Isearch-cgi/` are in scope for processing.
`data/`, `bin/`, `doc/`, `html/`, and the top-level reference files are
context, not targets.

## GIT — branch, automatic baseline, checkpointing, and push

**Working branch: `cleanup/isearch2`.** Every command below first
ensures you're on this branch — creating it from the current HEAD if it
doesn't exist yet, checking it out if it does. This keeps the whole
effort as one clean, reviewable set of commits in your fork, separate
from anything else going on there, and ready to hand to your colleague
as a proposed change set when you're done.

**One-time baseline (fully automatic — runs itself the first time any
command executes, no manual step needed):**
1. If tag `pre-cleanup-baseline` already exists, skip entirely.
2. Otherwise, if the tree has any uncommitted changes
   (`git status --porcelain` is non-empty), commit all of them now.
   This is the one deliberate exception to "never `git add -A`" used
   elsewhere in this file — the point here is to honestly capture
   whatever pre-existing state the tree was in, dirty or not, as the
   true starting line. Message: `Isearch2 cleanup: baseline before
   automated cleanup`.
3. Tag the resulting commit (or current HEAD, if nothing was dirty):
   `git tag pre-cleanup-baseline`.
4. Push it: `git push origin pre-cleanup-baseline`.

**Per-file checkpoint and processed commits** — unchanged from before,
still scoped `git add --` on only the relevant files (see GENERAL
below). **Every commit made by this workflow is immediately pushed:**
`git push origin cleanup/isearch2` if the branch already has upstream
tracking configured, otherwise `git push -u origin cleanup/isearch2`
the first time. Assumed remote name is `origin` pointing at your fork —
flag it if that's wrong.

Since your colleague only ever modifies the shared main repository, not
your fork, pushing your own fork's branch after every commit carries no
real collision risk — that's what makes auto-push safe to do by
default here.

## How a file gets marked "processed"

Every processed file gets this comment block added directly below any
existing copyright header, and nowhere else:

```c
// ISEARCH2-CLEANUP: processed 2026-08-05
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.
```

This is the source of truth for status — not just `docs/PROCESSING_STATUS.md`.
Detecting status by grepping the tree itself means ANALYZE is safe to
re-run any time (new files get queued, already-marked files are left
alone) even if the manifest file is ever lost, edited by hand, or out
of sync.

## Shell command style

In the VS Code extension, any multi-line or multi-statement Bash
invocation — loops, heredocs, several commands chained across
lines — triggers a permission prompt regardless of allow-listing. A
single, one-line command doesn't. So:

- Prefer a single non-looping command over a `for`/`while` loop when
  there's a native equivalent — e.g. `grep -n 'pattern' file1 file2
  file3` instead of looping `grep` over each file one at a time.
- **Never use a Bash heredoc (`cat > file <<'EOF' ... EOF`) to create
  or overwrite a file.** Use the Write (or Edit) tool directly instead
  — it's a separate tool call, not a Bash invocation, so it doesn't
  hit this at all, and it's already allow-listed.
- When a multi-step shell sequence is genuinely unavoidable (e.g. the
  GIT baseline/checkpoint steps above), that's fine — accept the
  occasional prompt there rather than contorting the command. This
  guidance is about the common cases (writing a scratch file, grepping
  several files) that have a simple single-command alternative, not
  about eliminating every multi-step sequence.

## GENERAL — the per-file pipeline

Every PROCESS NEXT and PROCESS `<filename>` runs this same pipeline
against exactly one file:

1. **Ensure branch + baseline** — see GIT above. Idempotent; a near-
   instant no-op after the very first run.
2. **Checkpoint commit, then push** — see GIT above.
3. **Read the file and its corresponding header (if any) in full**
   before changing anything.
4. **Public header signatures are frozen by default.** If a bug
   genuinely can't be fixed without changing a declaration in a `.h`/
   `.hxx` file, don't apply it. This is the one deliberate exception to
   "just proceed": header changes ripple to every other file that
   includes it, the same reasoning that kept `string.hxx` untouched — it
   always needs a human, never an autopilot guess. If you're running
   interactively, stop and ask, naming the exact signature change and
   why. If you're running unattended (see AUTONOMY below), mark the row
   `blocked` instead of `pending`/`done`, log the proposed signature
   change and reasoning to `docs/AUTOPILOT_LOG.md`, commit and push just
   that status change, and end the pipeline for this file — this file is
   not `done`, skip the remaining steps below for it. Whether the
   invoking command then stops entirely or continues to another file is
   up to that command, not this pipeline.
5. **Catalog every bug found** as a new subsection under that file's
   entry in `docs/BUG_CATALOG.md` (create the file on first use — see
   template below). Each fix in the source gets a `// BUGFIX #n`
   comment cross-referencing that subsection.
6. **Apply standard modernization** while touching a function:
   `NULL` → `nullptr`, `sprintf`-family → `snprintf`, add file-level and
   per-function doc comments. Don't restyle code you're not otherwise
   touching just for style's sake.
7. **Don't assume STRING's 1-based-indexing convention applies here.**
   That was `string.hxx`'s own deliberate design, not a tree-wide rule.
   If a file being processed has its own indexing/sentinel convention,
   document it in that file's own comments; don't import STRING's.
8. **Write or update Catch2 tests** under `tests/`, mirroring the
   source file's path relative to the tree root — e.g.
   `doctype/htmlparser.cxx` → `tests/doctype/test_htmlparser.cxx`. See
   TESTING below.
9. **Compile clean** under `-Wall -Wextra` (`make tests`) and **run the
   test suite** under AddressSanitizer + UndefinedBehaviorSanitizer
   (`make tests-asan`) before considering the file done. For any bug
   you're fixing that looks like a real memory-safety issue (not just a
   style problem), reproduce the *original* broken behavior standalone
   under ASan first, the same way the STRING pass validated the
   `ReadFile` double-free, so "fixed" means "confirmed it was real,"
   not just "looks right."
10. **Add the `ISEARCH2-CLEANUP: processed` marker** (see above).
11. **Update `docs/PROCESSING_STATUS.md`**: mark the row Done, set the
    date, link the `docs/BUG_CATALOG.md` subsection.
12. **Processed commit, then push** — see GIT above.
13. **Stop.** Do not move on to another file. Report a short summary:
    bugs found/fixed, tests added, warnings/sanitizer status, and both
    commit hashes.

## docs/PROCESSING_STATUS.md — format

A single Markdown table, one row per file in scope:

```markdown
| Order | File                          | Status  | Last Processed | Bug Catalog |
|-------|-------------------------------|---------|-----------------|-------------|
| 1     | src/common.cxx                | pending |                 |             |
| 2     | src/string.cxx                | pending |                 |             |
| 3     | doctype/htmlparser.cxx        | pending |                 |             |
```

`Status` is one of `pending`, `done`, `generated`, or `blocked`. `Order`
is only meaningful for `pending` rows; it's what PROCESS NEXT reads to
pick the next file — `blocked` rows are skipped the same as `done`/
`generated` ones, so a blocked file never gets immediately re-selected
and looped on. `generated` marks a file that's machine-produced by
another in-scope file (e.g. `src/dtreg.cxx`/`.hxx`, generated by
`doctype/dtconf.cxx`) — it's never hand-edited or run through the
GENERAL pipeline directly; instead, whoever processes the *generator*
regenerates and recommits the output as part of that turn. ANALYZE skips
`generated` rows the same as `done` ones. `blocked` marks a file GENERAL
couldn't finish unattended (see AUTONOMY below) — it needs a human look
before it can become `pending` again.

The `Bug Catalog` column does double duty: for `done` rows it links to
that file's `docs/BUG_CATALOG.md` subsection; for `blocked` rows it
links to that file's `docs/AUTOPILOT_LOG.md` entry instead.

## docs/BUG_CATALOG.md — format

One running file, one `##` section per source file processed, so it
stays greppable instead of exploding into per-file documents:

```markdown
## src/common.cxx

1. **`IsFile`'s Windows branch** — constant-vs-bitwise-AND typo caused
   directories to pass as files. Fixed; see `BUGFIX #1` in source.
```

## AUTONOMY — unattended runs, blocked files, docs/AUTOPILOT_LOG.md

Permission prompts are suppressed for this project (`.claude/settings.json`,
`permissions.defaultMode: "bypassPermissions"`) so `/process-next` and
`/process-5` can run through many files without anyone watching. That
changes what "ask the user" can mean:

- The **only** judgment call in the whole pipeline that still needs a
  human is GENERAL step 4 (public header signature changes). There is
  no "recommended option" to auto-pick there — it's deliberately not
  something to guess at unattended. Everything else in the pipeline
  (bug fixes, modernization, test-writing, commit messages) already has
  a fully-specified procedure elsewhere in this file; there's nothing
  else left over that needs a choice made up on the spot.
- When GENERAL step 4 blocks a file, log it to `docs/AUTOPILOT_LOG.md` —
  same convention as `docs/BUG_CATALOG.md`: one running file, one `##`
  section per file:

```markdown
## Isearch-cgi/example.cxx

**2026-08-07** — blocked at GENERAL step 4. Fixing the `NULL`-deref bug
in `Parse()` requires changing `PDOCTYPE Parse(const STRING&)` to
`PDOCTYPE Parse(const STRING&, bool)` in `example.hxx`, which is
included by 6 other files. Row set to `blocked`; needs a signature-change
decision before reprocessing.
```

- A `blocked` row is picked back up the same way as any other reprocess:
  once a human decides the header change (or a workaround), run
  `/process <filename>` — GENERAL doesn't special-case `blocked` as an
  input state, it just runs the normal pipeline again.

## TESTING — Catch2, vendored, no CMake

Catch2's **amalgamated** distribution (`catch_amalgamated.hpp` +
`catch_amalgamated.cpp`) lives at `tests/vendor/catch2/`, downloaded
once and checked into git. This keeps real Catch2 assertions without
needing CMake or any network access at build time, ever — the build
stays plain `g++`/`make` end to end, matching the rest of the tree.

One-time setup (do this once, not per file):
```sh
mkdir -p tests/vendor/catch2
curl -L -o tests/vendor/catch2/catch_amalgamated.hpp \
  https://github.com/catchorg/Catch2/releases/latest/download/catch_amalgamated.hpp
curl -L -o tests/vendor/catch2/catch_amalgamated.cpp \
  https://github.com/catchorg/Catch2/releases/latest/download/catch_amalgamated.cpp
git add tests/vendor/catch2 && git commit -m "Isearch2 cleanup: vendor Catch2 amalgamated"
git push origin cleanup/isearch2
```

Test files live under `tests/`, mirroring `src/`, `doctype/`, and
`Isearch-cgi/` by relative path, e.g. `tests/doctype/test_htmlparser.cxx`.

## BUILD — the original Makefile, adapted (no CMake)

Add a `tests` target to the existing top-level Makefile:

```makefile
CATCH2_DIR := tests/vendor/catch2
TEST_SRCS  := $(shell find tests -name '*.cxx')
TEST_OBJS  := $(TEST_SRCS:.cxx=.o) $(CATCH2_DIR)/catch_amalgamated.o
TEST_CXXFLAGS := -std=c++17 -Wall -Wextra -Isrc -Idoctype -IIsearch-cgi -I$(CATCH2_DIR)

$(CATCH2_DIR)/catch_amalgamated.o: $(CATCH2_DIR)/catch_amalgamated.cpp
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests/%.o: tests/%.cxx
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

tests: $(TEST_OBJS)
	@mkdir -p tests/reports
	$(CXX) $(TEST_OBJS) -o tests/run_tests
	@REPORT=tests/reports/report-$$(date +%Y%m%d-%H%M%S).txt; \
	tests/run_tests | tee $$REPORT; \
	echo "Report saved to $$REPORT"

tests-asan: TEST_CXXFLAGS += -fsanitize=address,undefined -g
tests-asan: tests
```

Everything else in the Makefile — building the actual engine, CGI
binaries, etc. — is unchanged; this only adds the `tests`/`tests-asan`
targets.

## COMMANDS

Documented here conceptually; the actual invokable slash commands live
in `.claude/commands/` (see the files provided alongside this one) so
you can type `/analyze`, `/process-next`, and `/process <filename>`
directly in the Claude Code tab.

- **ANALYZE** — ensures branch + baseline (see GIT), scans `src/`,
  `doctype/`, `Isearch-cgi/`; builds a `#include` dependency graph;
  excludes anything already carrying the `ISEARCH2-CLEANUP: processed`
  marker; topologically sorts the rest (least-depended-upon-by files
  first, ties broken by directory precedence
  `src/` → `doctype/` → `Isearch-cgi/`, then alphabetically); writes or
  updates the pending rows in `docs/PROCESSING_STATUS.md`, then commits
  and pushes that file (message `Isearch2 cleanup: update processing
  order (ANALYZE)`, skipped if nothing changed). **Never modifies a
  source file.** Safe to re-run any time.
- **PROCESS NEXT** — reads `docs/PROCESSING_STATUS.md`, takes the
  lowest-`Order` `pending` row, runs the GENERAL pipeline (including
  both commits and both pushes) on that one file, then stops.
- **PROCESS `<filename>`** — runs the GENERAL pipeline on the named
  file regardless of its current status (the reprocess path, typically
  after you've hand-edited an already-done file). The checkpoint commit
  is what makes this safe — even an uncommitted hand-edit gets captured
  and pushed before Claude touches it. If the name is ambiguous
  (matches files in more than one directory), ask which one before
  proceeding. Updates that file's row and re-timestamps it, then stops.
