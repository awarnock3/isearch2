# Process up to 10 files from the Isearch2 cleanup queue

Batch version of `/process-next`: runs the same per-file pipeline up to
10 times in one invocation instead of stopping after one file. Intended
for unattended runs now that prompts are suppressed (see AUTONOMY in
CLAUDE.md) — a blocked file doesn't halt the batch, it just gets skipped
and logged, same as it would in a standalone `/process-next`.

1. **Ensure branch + baseline (automatic, idempotent).** Check out
   `cleanup/isearch2`, creating it from current HEAD if needed. If tag
   `pre-cleanup-baseline` doesn't exist yet, create it per GIT in
   CLAUDE.md (commit any dirty tree state, tag, push). Skip if already
   done. Do this once, before the loop below — not once per file.
2. Read `docs/PROCESSING_STATUS.md`. If it doesn't exist or has no
   `pending` rows, stop and tell the user to run `/analyze` first.
3. **Loop up to 10 times**, stopping early if `pending` rows run out.
   Each iteration:
   a. Take the `pending` row with the lowest `Order` value.
   b. **Checkpoint commit, then push.** `git add --` only the target
      file (and its header, only if about to touch it) — never
      `git add -A`. Commit message: `Isearch2 cleanup: checkpoint before
      processing <file>`. Skip the commit if there's nothing to commit.
      Push: `git push origin cleanup/isearch2` (or `-u` the first time).
   c. Run the full GENERAL pipeline on that one file exactly as defined
      in CLAUDE.md (read file + header, freeze public header signatures
      per GENERAL step 4 — unattended, mark `blocked` and log to
      `docs/AUTOPILOT_LOG.md` per AUTONOMY, don't stop and ask since
      there's nobody to answer — catalog bugs into `docs/BUG_CATALOG.md`
      with `BUGFIX #n` comments, modernize `NULL`→`nullptr` and
      `sprintf`→`snprintf`, add doc comments, write or update the Catch2
      test under the mirrored `tests/` path, compile clean via
      `make tests`, run `make tests-asan`, add the
      `ISEARCH2-CLEANUP: processed` marker with today's date).
   d. **If GENERAL blocked the file:** it already committed and pushed
      the `blocked` status change and its `docs/AUTOPILOT_LOG.md` entry
      itself. Record it for the batch summary and move straight to the
      next iteration — a blocked file does not consume a "processed"
      slot's worth of pipeline work, but it does count toward this
      batch's 10-file loop budget (so a run of nothing but blocked files
      still terminates after 10, rather than spinning).
   e. **Otherwise:** update that file's row in
      `docs/PROCESSING_STATUS.md` to `done`, set `Last Processed`, link
      the relevant `docs/BUG_CATALOG.md` subsection. **Processed commit,
      then push.** `git add --` the target file, its header (only if
      changed), the new/updated test file, `docs/PROCESSING_STATUS.md`,
      and `docs/BUG_CATALOG.md`. Commit message: `Isearch2 cleanup:
      processed <file> — N bugs fixed, tests added`. Push:
      `git push origin cleanup/isearch2`. Record it for the batch
      summary.
4. **Stop** after 10 iterations (processed + blocked combined) or when
   `docs/PROCESSING_STATUS.md` has no `pending` rows left, whichever
   comes first. Do not proceed beyond that in this same invocation.
5. **Report a batch summary**, not just the last file: for each file
   touched this run, whether it was processed (bugs found/fixed, tests
   added, compile/sanitizer status) or blocked (why), plus every commit
   hash from the run. If the batch ended early because the queue ran
   out, say so and suggest `/analyze` if more files are expected.
