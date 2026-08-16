# Generate a comprehensive PDF status report

Produces a standalone PDF report summarizing the Isearch2 cleanup progress
with statistics on processed, blocked, and remaining files, including a
function/subroutine count for each file. Read-only with respect to the
cleanup tree itself — no commits, no status updates, no source changes.
Manual/on-demand only, like BLOCKED-REPORT and SMOKE-TEST.

Styled after the project's "reference manual" template (see
`TUTORIAL-reference-manual.pdf`), same visual language as
COMMANDS-REFERENCE: serif headings, sage-green note/stat boxes, and
numbered `§N` sections — rather than a generic bordered-table report.

Run the generator directly instead of rebuilding this by hand:

```
python3 .claude/tools/generate_status_report.py <isearch2_root_dir>
```

What it does, for reference:

1. **Parses `docs/PROCESSING_STATUS.md`** and classifies every row by
   status (`done`, `blocked`, `pending`, `generated`), tracking file path,
   order, and last-processed date.

2. **Counts functions/subroutines per file** (skipping `generated` ones)
   via regex pattern-matching over the actual source on disk — declarations,
   preprocessor lines, and typedefs are excluded so the count reflects real
   callable routines.

3. **Builds summary statistics**: done/blocked/pending counts and
   percentages, total functions, average functions per file, and average
   functions per done file.

4. **Builds the report as HTML**: title + subtitle, an intro note box
   summarizing the snapshot, an Audience/Format/Files/Generated metadata
   line, a Summary Statistics box, then one `§N` section per status
   category (`Processed (Done)`, `Blocked`, `Pending`, `Generated`) with a
   styled table — file paths in monospace, numeric columns right-aligned,
   table headers that repeat across page breaks.

5. **Converts to PDF**, preferring headless Chrome for full CSS fidelity
   (flexbox, `print-color-adjust`, repeating `<thead>` on page breaks),
   falling back to LibreOffice, then `wkhtmltopdf`, then `weasyprint` if
   none of those are available.

6. **Saves the final PDF** to
   `docs/reports/status-report-<YYYYMMDD-HHMMSS>.pdf` (creating
   `docs/reports/` if needed) and cleans up temp files afterward.

7. **Does not commit or push.** Like BLOCKED-REPORT and SMOKE-TEST, this is
   a point-in-time artifact, disposable and gitignored.

8. **Report back**: summary statistics (done/blocked/pending counts and
   percentages), full path to the generated PDF, and conversion method used
   (printed by the script).
