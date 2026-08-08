# Sync the fork's cleanup branch with upstream

Fetches changes from `upstream` (the colleague's repository) and merges
them into `cleanup/isearch2`, then re-scans for newly-queueable files.
There's no automatic trigger for this — invoke it deliberately whenever
you want to check for upstream changes, e.g. before a big batch run.

1. **Ensure branch + baseline (automatic, idempotent).** Check out
   `cleanup/isearch2`, creating it from current HEAD if needed. Same as
   GIT in CLAUDE.md. Skip if already on the branch with baseline tagged.
2. `git fetch upstream`.
3. `git merge upstream/main`. Two outcomes:
   - **Conflict-free** — continue to step 4.
   - **Conflicts** — STOP here. Resolving a real code conflict is the
     same category of judgment call as GENERAL step 4's header-signature
     freeze in CLAUDE.md, and just as inappropriate to guess at
     unattended. Report exactly which files conflicted (`git status`
     after the merge attempt lists them). Leave the merge in progress —
     don't auto-abort or auto-resolve. Tell the user to resolve by hand
     (edit the conflicted files, `git add` each, then `git commit` to
     complete the merge — or `git merge --abort` to cancel entirely).
     Once the merge is committed, re-run this command from step 4.
4. Tag the sync point: `git tag upstream-sync-<YYYY-MM-DD>-<7-char short
   SHA of the merge commit>` — includes the short SHA, not just the
   date, so re-running this more than once in a day doesn't collide.
   Push it: `git push origin upstream-sync-<date>-<shortsha>`.
5. Push the merge commit itself: `git push origin cleanup/isearch2`.
   Do this before step 6 so the merge is safely on the remote
   regardless of what happens next.
6. **Rerun `/rescan-status`, as a mandatory last step — every time, even
   when the merge was fully conflict-free.** This queues any brand-new
   files the merge introduced (what `/analyze` alone used to catch —
   skipping this step is exactly how 11 files went unqueued after the
   previous manual sync, `49e7b2d`, 2026-08-05, before that command
   existed), and additionally catches the case unique to syncing: a file
   that was already `done` in this fork but that the merge just changed
   again (upstream editing a file you'd already cleaned up). Those rows
   get flipped back to `pending` automatically — see RESCAN-STATUS in
   CLAUDE.md. Plain `/analyze` would silently skip that file forever,
   since it only excludes-and-leaves-alone anything already carrying the
   `processed` marker; it doesn't know how to reopen one.
7. Report: what was fetched (commit range from `upstream/main`), whether
   the merge was clean or conflicted (and if conflicted, which files,
   and that it's waiting on manual resolution — stop here in that case),
   the sync tag, how many new files `/rescan-status` queued in step 6,
   and how many previously-`done` files it reopened to `pending` because
   the merge changed them.
