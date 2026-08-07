# Reprocess every blocked file, deciding each signature change as we go

Walks through every file currently `blocked` in `docs/PROCESSING_STATUS.md`,
one at a time, asking you the specific GENERAL step 4 decision each one is
waiting on, applying whichever you choose, then running the rest of the
GENERAL pipeline to completion on that file — continuing until every file
that was blocked when this command started is either processed or you
explicitly defer it.

**This command never guesses at a signature change.** GENERAL step 4 in
CLAUDE.md reserves that decision for a human in every case, unattended or
not — this command is the interactive branch of step 4, run once per
blocked file in sequence, not a way around it. "Reprocess until success"
here means "keep walking the list, asking you at each stop," not "loop
automatically until they clear on their own" — re-running `/process` on a
file whose underlying decision nobody has made yet will just block it
again, identically, forever. If you want a no-decisions dry sweep instead
(e.g. to pick up files you already fixed by hand elsewhere), use `/process
<filename>` per file — `/process-5`/`/process-10` already skip blocked
rows without asking, they just won't unblock anything on their own either.

1. **Ensure branch + baseline (automatic, idempotent)** — same as every
   other command; see GIT in CLAUDE.md.
2. **Snapshot the queue.** Read `docs/PROCESSING_STATUS.md` and collect
   every row with `Status` = `blocked` *right now*, in `Order`. This list
   is fixed for the rest of the run — don't re-query mid-loop, and don't
   pick up any file that becomes newly blocked as a side effect of work
   done later in this same run (that's `/process-next`'s or
   `/process-5`'s job on a later invocation, not this one's).
   - If the snapshot is empty, report that and stop — nothing to do.
3. **For each file in the snapshot, in order:**
   a. Read its `##` section in `docs/AUTOPILOT_LOG.md` for the full
      reason and the specific decision it's waiting on (the same content
      `/blocked-report` surfaces) — don't re-derive this from scratch,
      the log entry already has it.
   b. **Present the decision to the user in the chat**: the file, a
      concise restatement of the bug, and the concrete options (e.g.
      "deep-copy `Table`/`TotalEntries`/`MaxEntries`" vs. "`= delete`
      both to make it non-copyable"), plus any secondary consequence
      already flagged in the log (e.g. `attrlist.hxx`'s non-copyable
      path needing `OPERAND`'s and `DFD`'s copy sites re-audited). Ask
      which to apply. This is GENERAL step 4's "interactively, stop and
      ask, naming the exact signature change and why" — do it for real,
      wait for an actual answer, don't assume a default or pick the one
      that sounds safer.
   c. **If the user answers with a decision:** apply it (add the
      declaration(s) to the header, implement the body — deep-copy or
      `= delete` — per their choice), then run the remaining GENERAL
      steps on this file exactly as `/process <filename>` would: bug
      catalog entry for this fix alongside any others found in the file,
      standard modernization, Catch2 test under the mirrored `tests/`
      path, `make tests` / `make tests-asan`, the
      `ISEARCH2-CLEANUP: processed` marker, the
      `docs/PROCESSING_STATUS.md` row set to `done`, and the checkpoint +
      processed commits and pushes per GIT. Record the outcome for the
      batch summary.
   d. **If the user declines or wants to skip it for now:** leave the
      row `blocked`, don't touch the file, note in the batch summary
      that it was explicitly deferred (not attempted) this run, and move
      on — don't ask about it again later in this same invocation.
   e. **If GENERAL blocks the file again for a *different* reason** than
      the one just decided (e.g. a second, unrelated signature question
      surfaces once you're inside the file) — that's a new step 4 case;
      ask about *that* one the same way as step b, don't silently carry
      the first answer over to a question it wasn't about.
4. **Stop** once every file in the step 2 snapshot has been either
   processed or explicitly deferred. Do not loop back over deferred files
   in this same invocation — re-run this command later once you're ready
   to decide on them.
5. **Report a batch summary**: for each file in the snapshot, whether it
   was unblocked and processed (bugs found/fixed, tests added, sanitizer
   status, both commit hashes) or deferred (and that it's still
   `blocked`). If every file cleared, say so plainly — that's the "until
   success" case fully realized for this run.
