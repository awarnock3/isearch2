# Document one named file, or an .hxx/.cxx pair

Target: $ARGUMENTS

A targeted sibling of `/document-next`/`/document-10` for when you want
one specific file (or its header/source pair) documented right now,
rather than whatever the queue would pick next. Same read-only-w.r.t.-
bug-fixing-status contract as the rest of the documentation commands —
never changes `docs/PROCESSING_STATUS.md` or `docs/BUG_CATALOG.md`.

1. **Ensure branch (automatic, idempotent).** Check out
   `cleanup/isearch2` (it already exists by the time documentation work
   starts).

2. **Resolve the target(s) from $ARGUMENTS:**
   - **Bare name, no extension** (e.g. `common`): look in `src/`,
     `doctype/`, and `Isearch-cgi/` for `<name>.cxx` together with
     `<name>.hxx` or `<name>.h` in that same directory. Whichever of
     those exist form the pair to document together. If the bare name
     matches in more than one of the three directories, stop and ask
     which one before doing anything else.
   - **One name with an extension** (e.g. `common.cxx`, or a full path
     like `doctype/dif.hxx`): locate that exact file (same
     ambiguous-bare-filename check as above if no directory was given).
     Then look for its natural sibling in the same directory — the
     `.cxx` for a `.hxx`/`.h`, or the `.hxx`/`.h` for a `.cxx` — sharing
     the same basename. If found, pull it into the same pass
     automatically (this mirrors what `/document-next` already does
     incidentally: documenting a `.cxx` writes the matching `.hxx`'s
     one-line declaration briefs too). If no sibling exists, proceed
     solo — plenty of files in this tree are header-only or have no
     separate header.
   - **Two names given** (e.g. `common.cxx common.hxx`): resolve each
     independently (same ambiguity check), and document them together
     as this invocation's pair. If their basenames don't actually
     match, ask for confirmation that pairing them anyway is
     intentional before proceeding.
   - If any given name can't be resolved to a file on disk, stop and
     report exactly which name(s) failed.

3. **Check each resolved file's row in `docs/PROCESSING_STATUS.md`:**
   - `generated` — refuse that file. Generated files are never
     hand-documented; whoever processes the *generator* regenerates and
     redocuments the output as part of that turn. Stop and say so.
   - Missing entirely, `pending`, or `blocked` — the bug-fixing pass on
     this file isn't done, so documentation now would likely just get
     redone once it changes. Stop and ask for confirmation before
     documenting it anyway.
   - `done` — proceed normally.
   - For a pair, apply this check to each file independently; only ask
     about the ones that actually need confirmation.

4. **Read the target file(s)**, their existing headers, and any
   existing `docs/DOCUMENTATION_STATUS.md` rows for them, in full,
   before writing anything.

5. **Write the documentation** — same standard as `/document-next` step
   6:
   - **File level**: one Doxygen `@file`/`@brief` block near the top of
     the `.cxx` if one's in scope.
   - **Per function**: a full `@brief`/`@param`/`@return` block above
     *every* function definition in the `.cxx`.
   - **Header declarations**: a one-line `@brief`-only comment above
     each matching declaration in the `.hxx`/`.h`, if one's in scope.
   - **Header-only target** (no `.cxx` sibling at all): put the
     `@file`/`@brief` block in the header itself, and give any inline
     function bodies defined there the same full
     `@brief`/`@param`/`@return` treatment a `.cxx` function would get
     — there's no separate definition site to carry it instead.
   - **If the target already carries doc comments from an earlier
     pass**: don't just fill in gaps for functions that don't have one
     yet. Re-read every existing block against that function's current
     body and signature, and rewrite any block whose `@brief`, prose,
     `@param`s, or `@return` no longer match — a function can have had
     its behavior change underneath an unchanged signature, leaving a
     stale comment that still looks complete. Only leave a block as-is
     if it still accurately describes what the function does today.
   - Same rules as `/document-next`: don't restate what's obvious from
     a well-named signature, don't invent behavior you haven't actually
     read, and call out anything genuinely subtle or surprising.

6. **Compile clean.** `make tests`.

7. **Update `docs/DOCUMENTATION_STATUS.md`**: for every file actually
   touched, set `Status` = `done` and `Last Documented` = today. If a
   touched file has no row yet, add one (`Order` matching its row in
   `docs/PROCESSING_STATUS.md`) rather than requiring a prior
   `/document-next`/`/document-10` sync.

8. **Commit, then push.** `git add --` the touched source file(s) and
   `docs/DOCUMENTATION_STATUS.md` — never `git add -A`. Commit message:
   - Solo file: `Isearch2 cleanup: documented <file> — N functions
     documented`.
   - Pair: `Isearch2 cleanup: documented <cxx-path> + <hxx-path> — N
     functions documented`.

   Push: `git push origin cleanup/isearch2` (or `-u` the first time).

9. **Stop.** Do not proceed to another file or pair in this same
   invocation.

10. Report back: which file(s) were documented, how many functions,
    compile status, and the commit hash.

See `.claude/commands/document-next.md` for the
`docs/DOCUMENTATION_STATUS.md` format.
