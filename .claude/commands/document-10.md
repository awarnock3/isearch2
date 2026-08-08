# Document up to 10 files from the Isearch2 documentation queue

Batch version of `/document-next`: runs the same per-file documentation
pass up to 10 times in one invocation instead of stopping after one
file. Same read-only-w.r.t.-bug-fixing-status contract — never touches
`docs/PROCESSING_STATUS.md` or `docs/BUG_CATALOG.md`.

1. **Ensure branch (automatic, idempotent).** Check out
   `cleanup/isearch2`.
2. **Sync the queue against `docs/PROCESSING_STATUS.md`, once, before
   the loop below** — not once per file. Same two steps `/document-next`
   does:
   - Add a `pending` row (matching `Order`) to
     `docs/DOCUMENTATION_STATUS.md` for every `done`
     `docs/PROCESSING_STATUS.md` row not yet present there (skip
     `generated` rows).
   - **Staleness check**: for every row already `done` here, diff the
     file against its last `Isearch2 cleanup: documented <file> ...`
     commit; if changed since, flip back to `pending` and clear
     `Last Documented`. This is how a colleague's later edit to an
     already-documented file gets it redocumented automatically,
     without a separate rescan command.
3. If `docs/DOCUMENTATION_STATUS.md` has no `pending` rows after
   syncing, stop and tell the user there's nothing to document.
4. **Loop up to 10 times**, stopping early if `pending` rows run out.
   Each iteration:
   a. Take the `pending` row with the lowest `Order` value.
   b. Read the file and its header (if any) in full.
   c. **Write the documentation** — same standard as `/document-next`
      step 6: a `@file`/`@brief` block near the top of the `.cxx`; a
      full `@brief`/`@param`/`@return` block above *every* function
      definition; a one-line `@brief` above each matching `.hxx`
      declaration. Complete coverage, not just the interesting
      functions.
   d. **Compile clean.** `make tests`.
   e. Update the row in `docs/DOCUMENTATION_STATUS.md` to `done`, set
      `Last Documented` to today's date.
   f. **Commit, then push.** `git add --` the target `.cxx`/`.hxx` and
      `docs/DOCUMENTATION_STATUS.md` — never `git add -A`. Commit
      message: `Isearch2 cleanup: documented <file> — N functions
      documented`. Push: `git push origin cleanup/isearch2` (or `-u`
      the first time). Record it for the batch summary.
5. **Stop** after 10 files or when the queue runs out, whichever comes
   first. Do not proceed beyond that in this same invocation.
6. **Report a batch summary**: for each file touched this run, how many
   functions were documented and the compile status, plus every commit
   hash from the run. If the batch ended early because the queue ran
   out, say so.

See `.claude/commands/document-next.md` for the
`docs/DOCUMENTATION_STATUS.md` format.
