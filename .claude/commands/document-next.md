# Document the next file in the Isearch2 documentation queue

Adds standard professional (Doxygen-style) documentation to one already
-cleaned-up file: a file-level summary and a full doc comment for every
function. A separate concern from `/process-next` — GENERAL step 6 only
adds light doc comments incidentally, in passing, while fixing bugs;
this command is a dedicated, complete documentation pass over a file
that's already `done`. Read-only with respect to bug-fixing status —
never changes `docs/PROCESSING_STATUS.md` or `docs/BUG_CATALOG.md`.

1. **Ensure branch (automatic, idempotent).** Check out
   `cleanup/isearch2` (it already exists by the time documentation work
   starts).
2. **Sync the queue against `docs/PROCESSING_STATUS.md`** (create
   `docs/DOCUMENTATION_STATUS.md` on first use — see format below):
   - For every `done` row in `docs/PROCESSING_STATUS.md` with no
     matching row yet in `docs/DOCUMENTATION_STATUS.md`, add one:
     same `Order`, `Status` = `pending`. (Files still `pending`/
     `blocked` in the bug-fixing pipeline aren't ready to document —
     they're still going to change.) Rows for `generated` files are
     skipped, same as `generated` is skipped elsewhere.
   - **Staleness check** — for every row already `done` here, find the
     most recent `Isearch2 cleanup: documented <file> ...` commit
     touching that path, and diff the file against it (same technique
     RESCAN-STATUS uses against `Isearch2 cleanup: processed ...`
     commits). Changed since then (typically a colleague's hand-edit,
     or a `/sync-upstream` merge) → flip the row back to `pending`,
     clear `Last Documented`. This is what makes redocumentation happen
     automatically when a documented file gets modified later — no
     separate rescan command needed for it. A `done` row with no
     discoverable documented-commit is left `done` and reported as
     inconclusive rather than guessed at.
   - If nothing changed, skip committing this file this step (commit
     it together with step 6 below instead, or on its own with message
     `Isearch2 cleanup: update documentation queue` if step 6 finds
     nothing to do).
3. If `docs/DOCUMENTATION_STATUS.md` has no `pending` rows after
   syncing, stop and tell the user there's nothing to document (either
   everything's current, or nothing's been marked `done` in
   `docs/PROCESSING_STATUS.md` yet).
4. Take the `pending` row with the lowest `Order` value. That file is
   the target.
5. **Read the file and its header (if any) in full.**
6. **Write the documentation.** Doxygen-style `/** ... */` blocks:
   - **File level**: one block near the top of the `.cxx` (below the
     `ISEARCH2-CLEANUP: processed` marker and any copyright header),
     using `@file`, `@brief` (one line), and enough prose to explain the
     file's role, its main class(es), and anything a new reader would
     need to orient themselves — the kind of context already being
     written into `docs/BUG_CATALOG.md` intros, just living in the
     source instead.
   - **Per function**: a full block directly above *every* function
     definition in the `.cxx` — `@brief` (one line), a paragraph if the
     behavior isn't obvious from the name, `@param` for each parameter,
     `@return` if non-void. Cover every function, not just ones with
     interesting bugs — this is a complete pass, unlike GENERAL step 6.
   - **Header declarations**: a one-line `@brief`-only comment above
     each matching declaration in the `.hxx` (full detail lives at the
     definition; the header just needs enough for a caller skimming the
     API). Skip this for files with no separate header.
   - **Re-documenting a file that already has doc comments** (this row
     was flipped back to `pending` by the staleness check in step 2,
     meaning the file changed since it was last documented): don't just
     fill in gaps for new functions. Re-read every existing block
     against that function's *current* body and signature, and rewrite
     any block whose `@brief`, prose, `@param`s, or `@return` no longer
     match — a function whose signature didn't change can still have
     had its behavior change underneath an now-stale comment. A comment
     is only left as-is if it still accurately describes what the
     function does today.
   - Don't restate what's already obvious from a well-named signature;
     don't invent behavior you haven't actually read in the function
     body. If a function's real behavior is subtle or surprising,
     that's exactly what the doc comment is for.
7. **Compile clean.** `make tests` — comments-only changes shouldn't
   break anything, but confirm rather than assume.
8. **Update `docs/DOCUMENTATION_STATUS.md`**: mark the row `done`, set
   `Last Documented` to today's date.
9. **Commit, then push.** `git add --` the target `.cxx`/`.hxx` and
   `docs/DOCUMENTATION_STATUS.md` — never `git add -A`. Commit message:
   `Isearch2 cleanup: documented <file> — N functions documented`.
   Push: `git push origin cleanup/isearch2` (or `-u` the first time).
10. **Stop.** Do not proceed to another file in this same invocation.
11. Report back: which file was documented, how many functions, compile
    status, and the commit hash.

## docs/DOCUMENTATION_STATUS.md — format

```markdown
| Order | File           | Status  | Last Documented |
|-------|----------------|---------|------------------|
| 1     | src/common.cxx | pending |                  |
```

`Status` is `pending`, `done`, or `generated` (generated files are
skipped, same treatment as `docs/PROCESSING_STATUS.md`). `Order` mirrors
the `Order` value for the same file in `docs/PROCESSING_STATUS.md`, so
the two queues stay in the same relative sequence.
