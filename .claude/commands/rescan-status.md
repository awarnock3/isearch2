# Rescan the fork for new or updated files and requeue what needs (re)processing

Supersets `/analyze`: ANALYZE only ever queues files it has **never** seen
before (anything already carrying the `ISEARCH2-CLEANUP: processed` marker
is deliberately excluded and left alone, per CLAUDE.md). This command adds
the case ANALYZE intentionally skips — a file that was already marked
`done`, but whose content has since changed underneath that marker (most
likely via `/sync-upstream` merging in a colleague's edit, or a hand-edit),
and needs to go back through the pipeline. Read-only with respect to
`src/`, `doctype/`, `Isearch-cgi/`, and `docs/BUG_CATALOG.md` — same
contract as ANALYZE. This command only ever writes
`docs/PROCESSING_STATUS.md`. It does not find or catalog bugs itself; it
only decides what belongs in the queue. Bug-cataloging happens the normal
way, automatically, whenever `/process` or `/process-next` later actually
processes a file this command queues (GENERAL step 5 in CLAUDE.md).

1. **Ensure branch + baseline (automatic, idempotent).** Same as every
   other command; see GIT in CLAUDE.md.
2. Recursively scan `src/`, `doctype/`, and `Isearch-cgi/` for source files
   (`.c`, `.cxx`, `.h`, `.hxx`) — same scope as ANALYZE step 2. Flag
   anything found that doesn't match these extensions before deciding
   whether it's in scope.
3. For each file found, look up its row in `docs/PROCESSING_STATUS.md` by
   the `File` column:
   - **No row exists at all (brand-new file).** Queue it exactly like
     ANALYZE does: build/extend the `#include` dependency graph across the
     newly-discovered files (ignore system/standard headers), topologically
     sort so files `#include`d by the most other in-scope files come
     first, ties broken by directory precedence `src/` → `doctype/` →
     `Isearch-cgi/` then alphabetically, and append as new `pending` rows
     after the existing ones — don't renumber or reorder anything already
     in the table. `Last Processed` and `Bug Catalog` blank.
   - **Row exists, `Status` = `pending`.** No change needed — it's already
     queued. Leave the row untouched.
   - **Row exists, `Status` = `generated`.** Leave untouched. Regenerated
     by its generator file's own turn, never queued directly (see CLAUDE.md).
   - **Row exists, `Status` = `blocked`.** Leave untouched. Unblocking is a
     human signature decision (`/reprocess-blocked` or `/process
     <filename>`), never a silent status flip by a scan.
   - **Row exists, `Status` = `done`.** This is the "updated file" case —
     determine whether the file has changed since it was marked done:
     1. `git log --format='%H %s' -- "<file>"` (already scoped to commits
        touching this exact path). Scan the results (newest first) for the
        most recent one whose subject contains `Isearch2 cleanup:
        processed` — that's the commit that last marked this file done.
        Note some processed commits cover a header+source pair together
        (e.g. `processed src/attr.hxx, src/attr.cxx — ...`), so match on
        the `processed` substring, not an exact `processed <file>` prefix.
     2. If no such commit is found (a `done` row predating this
        convention, or history was rewritten), don't guess — leave the row
        as `done` and list it separately in the final report as
        "inconclusive, needs a manual look."
     3. Otherwise run `git diff --quiet <that-commit-sha> HEAD -- "<file>"`.
        Exit 0 (no diff) → unchanged since processing; leave the row alone.
        Exit non-zero (diff exists) → the file was modified after being
        marked done. **Flip that row's `Status` to `pending`**, blank its
        `Last Processed` and `Bug Catalog` cells (matching the normal
        pending-row format), and keep its `Order` number exactly as it
        was — don't renumber. Do **not** touch `docs/BUG_CATALOG.md`; the
        file's existing `##` section there stays as the historical record
        of what was already found and fixed, and picks up new subsections
        the next time it's actually processed.
4. Do not modify, create, or delete anything under `src/`, `doctype/`,
   `Isearch-cgi/`, or `docs/BUG_CATALOG.md`. Only `docs/PROCESSING_STATUS.md`
   is written.
5. If `docs/PROCESSING_STATUS.md` changed, `git add docs/PROCESSING_STATUS.md`,
   commit `Isearch2 cleanup: update processing order (RESCAN)`, and push
   (`git push origin cleanup/isearch2`, or `git push -u origin
   cleanup/isearch2` if no upstream tracking yet). Skip the commit if
   nothing changed.
6. Report back, in these four buckets: how many brand-new files were
   queued (and their order), which `done` files were detected as changed
   and flipped back to `pending` (list each, with the processed-commit SHA
   it was diffed against), how many `done` files were checked and
   confirmed unchanged, and how many `done` rows were inconclusive and
   left alone pending a manual look.
