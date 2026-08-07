# Analyze the Isearch2 tree and build the processing order

Follow the ANALYZE procedure defined in CLAUDE.md under "COMMANDS" and
"GIT."

Steps:
1. **Ensure branch + baseline (automatic, idempotent).** Check out
   `cleanup/isearch2`, creating it from current HEAD if it doesn't
   exist. If tag `pre-cleanup-baseline` doesn't exist yet: commit any
   currently-dirty tree state (this is the one place a broad commit is
   correct — see GIT in CLAUDE.md for why), tag it
   `pre-cleanup-baseline`, and push the tag. Skip anything already done.
2. Recursively scan `src/`, `doctype/`, and `Isearch-cgi/` for source
   files (`.c`, `.cxx`, `.h`, `.hxx` — `Isearch-cgi/` files are
   confirmed `.cxx`). Flag anything you find that doesn't match these
   extensions before deciding whether it's in scope.
3. For each file, check whether it already contains the
   `ISEARCH2-CLEANUP: processed` marker comment, OR already has a row in
   `docs/PROCESSING_STATUS.md` with status `generated`. Exclude both
   from the new ordering entirely — leave their existing row untouched.
   (`generated` files are machine-produced by another in-scope file and
   never carry the marker themselves — see the format section below.)
4. For the remaining (unmarked) files, build a `#include` dependency
   graph within the tree (ignore system/standard headers). Topologically
   sort so files that are `#include`d by the most other in-scope files
   come first. Break ties by directory precedence
   `src/` → `doctype/` → `Isearch-cgi/`, then alphabetically by path.
5. If `docs/PROCESSING_STATUS.md` doesn't exist yet, create it with the
   table header from CLAUDE.md. Append the newly ordered pending rows
   after any existing rows — don't renumber or reorder rows that are
   already `done`, and don't duplicate a row for a file already present
   in the table.
6. Do **not** modify, create, or delete anything under `src/`,
   `doctype/`, or `Isearch-cgi/`. This command only reads and only
   writes `docs/PROCESSING_STATUS.md`.
7. If `docs/PROCESSING_STATUS.md` changed, `git add docs/PROCESSING_STATUS.md`,
   commit with message `Isearch2 cleanup: update processing order
   (ANALYZE)`, and push (`git push origin cleanup/isearch2`, or
   `git push -u origin cleanup/isearch2` if this branch has no upstream
   tracking yet). If nothing changed, skip the commit.
8. Report back: how many files are newly queued, how many were already
   done and skipped, and the resulting order.
