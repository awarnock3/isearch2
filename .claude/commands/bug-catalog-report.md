# Generate a PDF bug-catalog report

Produces a standalone PDF rendering of `docs/BUG_CATALOG.md` — every bug
found and fixed while processing `src/`, `doctype/`, and `Isearch-cgi/` —
cross-referenced against each file's *current* row in
`docs/PROCESSING_STATUS.md`. Read-only with respect to the cleanup tree
itself — no commits, no status updates, no source changes. Manual/on-demand
only, like STATUS-REPORT and COMMANDS-REFERENCE.

Styled after the project's "reference manual" template (see
`TUTORIAL-reference-manual.pdf`), same visual language as STATUS-REPORT
and COMMANDS-REFERENCE: serif headings, sage-green note/stat boxes, and
numbered `§N` sections.

Run the generator directly instead of rebuilding this by hand:

```
python3 .claude/tools/generate_bug_catalog_report.py <isearch2_root_dir>
```

What it does, for reference:

1. **Parses `docs/BUG_CATALOG.md`** into one section per cataloged file:
   the intro prose, the numbered list of bugs found and fixed in that
   file's own turn, and any `### ...`-subsectioned findings deferred to
   another file's turn. Handles the format's real variation — bold
   titles that wrap across lines, items not led by a bold span, fenced
   ` ``` ` code/ASan-output blocks nested in a list item — rather than
   assuming one rigid shape.

2. **Cross-references every cataloged file against `docs/PROCESSING_STATUS.md`.**
   This is the part that matters most to get right: RESCAN-STATUS can
   flip a `done` row back to `pending` (e.g. after `sync-upstream` merges
   in a change) without touching `BUG_CATALOG.md`, so a catalog entry's
   "Fixed" claim can silently outlive the code it was fixed against.
   Every cataloged file that is no longer `done` is called out — both in
   its own `§3` entry (a highlighted note) and in a dedicated `§2
   Reopened Since Cataloged` table — so those entries read as provisional
   rather than as settled fact.

3. **Scans the catalog's own text for "dead" file/function mentions**
   (the project's recurring term for code with no callers, or a file
   missing from `src/Makefile`'s production `OBJ` list) to build a
   `§1 Dead Files & Functions` table, filtering out negated mentions
   (e.g. "this one is **not** dead code"). This is a heuristic text scan,
   not a static-analysis result, and the report says so — treat it as a
   starting point for verification, not a verified list.

4. **Builds the report as HTML**: title + subtitle, an intro note box,
   an Audience/Format/Files/Bugs/Generated metadata line, a Summary
   Statistics box, then `§1` Dead Files & Functions, `§2` Reopened Since
   Cataloged, and `§3` Bug Catalog — one sub-block per file with a
   status badge (`done`/`pending`/`blocked`), its numbered bugs, and any
   deferred findings.

5. **Converts to PDF**, preferring headless Chrome for full CSS fidelity
   (flexbox, `print-color-adjust`, repeating `<thead>` on page breaks),
   falling back to LibreOffice, then `wkhtmltopdf`.

6. **Saves the final PDF** to
   `docs/reports/bug-catalog-report-<YYYYMMDD-HHMMSS>.pdf` (creating
   `docs/reports/` if needed) and cleans up temp files afterward.

7. **Does not commit or push.** Like STATUS-REPORT, BLOCKED-REPORT, and
   COMMANDS-REFERENCE, this is a point-in-time artifact, disposable and
   gitignored.

8. **Report back**: files cataloged, total bugs, how many are reopened
   since cataloging, full path to the generated PDF, and conversion
   method used (printed by the script).
