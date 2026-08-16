# Run the integration smoke test

Builds the real production binaries (the full tree, processed and
not-yet-processed files together — not just the Catch2 subset) and
verifies they actually work: index the sample corpus, confirm each
document is findable by search. Complements the per-file unit tests;
doesn't replace them. Manual/on-demand — nothing else invokes this
automatically.

1. Run `make smoke-test`.
2. Report the per-document results plainly (`make smoke-test` already
   prints `OK`/`FAIL` per check) — don't just say "passed" or "failed"
   for the whole run, list which of the 5 checks (Watersheds/
   cgia-wswtemp, Oceanography/dds10, Dust/dust, glaciers/glaciers,
   Naval/goes_9_conus) succeeded and which didn't.
3. **If the build itself fails** (compile/link error, before any search
   checks run), that's a real integration break — report the exact
   error and which file it's in. This is exactly the kind of regression
   the per-file unit tests can't catch, since they only link a subset of
   the tree.
4. **If the build succeeds but a search check fails**, don't assume
   it's caused by whichever file was processed most recently. Do give
   the user a starting point: read `docs/PROCESSING_STATUS.md` and list
   the `done` rows with the most recent `Last Processed` dates (a
   handful, not the whole table) as the most likely places to look
   first, and say so explicitly as a starting point — not a diagnosis.
5. Don't attempt to fix a failure as part of this command — report it
   and stop. Investigating and fixing is a separate, deliberate task.
