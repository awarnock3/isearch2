# Process (or reprocess) one named file

Target file: $ARGUMENTS

1. **Ensure branch + baseline (automatic, idempotent).** Check out
   `cleanup/isearch2`, creating it from current HEAD if needed. If tag
   `pre-cleanup-baseline` doesn't exist yet, create it per GIT in
   CLAUDE.md (commit any dirty tree state, tag, push). Skip if already
   done.
2. Locate the file in `src/`, `doctype/`, or `Isearch-cgi/`. Accept
   either a full relative path or a bare filename. If the bare filename
   matches files in more than one of those directories, stop and ask
   the user which one they mean before doing anything else.
3. This runs regardless of the file's current status in
   `docs/PROCESSING_STATUS.md` — pending, done, or not yet listed at
   all. This is the reprocess path, typically used after the user has
   hand-edited an already-processed file.
4. **Checkpoint commit first, before touching anything, then push.**
   `git add --` only the target file (and its header, only if you're
   about to touch it) — never `git add -A`. Commit message: `Isearch2
   cleanup: checkpoint before processing <file>`. Skip the commit if
   there's nothing to commit. Push: `git push origin cleanup/isearch2`
   (or `git push -u origin cleanup/isearch2` if no upstream tracking
   yet). This is what makes reprocessing safe even when the on-disk
   state is a hand-edit that was never committed — whatever's there
   right now gets preserved and pushed before Claude changes it.
5. Run the full GENERAL pipeline on that one file exactly as defined in
   CLAUDE.md (same steps as `/process-next`: read file + header, freeze
   public header signatures per GENERAL step 4 — interactively, stop and
   ask; unattended, mark `blocked` and log to `docs/AUTOPILOT_LOG.md` per
   AUTONOMY — catalog bugs into `docs/BUG_CATALOG.md` with `BUGFIX #n`
   comments, modernize `NULL`→`nullptr` and `sprintf`→`snprintf`, add doc
   comments, write or update the Catch2 test under the mirrored `tests/`
   path, compile clean via `make tests`, run `make tests-asan`, refresh
   the `ISEARCH2-CLEANUP: processed` marker date).
6. **If GENERAL blocked the file** (step 4), it already committed and
   pushed the `blocked` status change and its `docs/AUTOPILOT_LOG.md`
   entry — skip steps 6–7 below, go straight to step 8/9 and report the
   block. This is exactly how a previously-`blocked` file gets picked
   back up: run `/process <filename>` once the signature decision is
   made.
7. Otherwise, update or create that file's row in
   `docs/PROCESSING_STATUS.md` (status `done`, refreshed
   `Last Processed` date, updated `docs/BUG_CATALOG.md` link). If it
   wasn't in the table before, add it rather than requiring a prior
   `/analyze` run.
8. **Processed commit, then push.** `git add --` the target file, its
   header (only if changed), the test file, `docs/PROCESSING_STATUS.md`,
   and `docs/BUG_CATALOG.md`. Commit message: `Isearch2 cleanup:
   processed <file> — N bugs fixed, tests added`. Push:
   `git push origin cleanup/isearch2`.
9. **Stop** after this one file.
10. Report back: what changed since the last pass (if any) or why it
    was blocked, bugs found/fixed this time, test results,
    compile/sanitizer status, and both commit hashes.
