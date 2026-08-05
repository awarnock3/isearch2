# Process the next file in the Isearch2 cleanup queue

1. **Ensure branch + baseline (automatic, idempotent).** Check out
   `cleanup/isearch2`, creating it from current HEAD if needed. If tag
   `pre-cleanup-baseline` doesn't exist yet, create it per GIT in
   CLAUDE.md (commit any dirty tree state, tag, push). Skip if already
   done.
2. Read `docs/PROCESSING_STATUS.md`. If it doesn't exist or has no
   `pending` rows, stop and tell the user to run `/analyze` first.
3. Take the `pending` row with the lowest `Order` value. That file is
   the target for this command.
4. **Checkpoint commit, then push.** `git add --` only the target file
   (and its header, only if you're about to touch it) — never
   `git add -A`, there may be unrelated uncommitted work elsewhere in
   the tree. Commit message: `Isearch2 cleanup: checkpoint before
   processing <file>`. Skip the commit if there's nothing to commit.
   Push: `git push origin cleanup/isearch2` (or `git push -u origin
   cleanup/isearch2` if no upstream tracking yet).
5. Run the full GENERAL pipeline on that one file exactly as defined in
   CLAUDE.md (read file + header, freeze public header signatures
   unless you stop to ask, catalog bugs into `docs/BUG_CATALOG.md` with
   `BUGFIX #n` comments at each fix site, modernize `NULL`→`nullptr` and
   `sprintf`→`snprintf`, add file/function doc comments, write or update
   the Catch2 test under the mirrored path in `tests/`, compile clean
   via `make tests`, run `make tests-asan`, add the
   `ISEARCH2-CLEANUP: processed` marker with today's date).
6. Update that file's row in `docs/PROCESSING_STATUS.md` to `done`, set
   `Last Processed`, and link the relevant `docs/BUG_CATALOG.md`
   subsection.
7. **Processed commit, then push.** `git add --` the target file, its
   header (only if changed), the new/updated test file,
   `docs/PROCESSING_STATUS.md`, and `docs/BUG_CATALOG.md`. Commit
   message: `Isearch2 cleanup: processed <file> — N bugs fixed, tests
   added`. Push: `git push origin cleanup/isearch2`.
8. **Stop.** Do not proceed to another file in this same invocation,
   even if there are more pending rows.
9. Report back: which file was processed, bugs found/fixed, tests
   added, compile/sanitizer status, and both commit hashes.
