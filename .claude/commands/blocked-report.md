# Generate a PDF report of blocked files

Produces a standalone PDF listing every file currently `blocked` in
`docs/PROCESSING_STATUS.md`, with the reason each one is blocked and the
specific action needed to clear it. Read-only with respect to the
cleanup tree itself — no commits, no status updates, no source changes.
Manual/on-demand only, like SMOKE-TEST.

1. **Gather blocked files.** Read `docs/PROCESSING_STATUS.md` and
   collect every row with `Status` = `blocked`. If there are none,
   don't skip the report — still generate a one-page PDF stating "No
   files are currently blocked" plus the generation timestamp; an empty
   result is still a useful answer.
2. **Pull the reason and required action for each one.** For every
   blocked file, find its `##` section in `docs/AUTOPILOT_LOG.md`
   (heading is the file's path, e.g. `## src/reclist.hxx`). From that
   section's prose, extract:
   - **Reason** — a concise 2–4 sentence summary of the actual defect
     or condition that triggered the block (don't just say "header
     signature change needed" — say what the bug is and why it matters,
     the way the log entry already explains it).
   - **Required action** — the specific decision or signature change
     needed to unblock it (e.g. "add `RECLIST(const RECLIST&)` and
     `operator=` for deep-copy semantics, or `= delete` both to make the
     class explicitly non-copyable"), plus the exact reprocess command:
     `` `/process <filename>` ``.
   - **Blocked since** — the date from the log entry (should match the
     `Last Processed` column in `docs/PROCESSING_STATUS.md`; if they
     disagree, flag the discrepancy in the report rather than silently
     picking one).
   If a blocked row has no matching `##` section in
   `docs/AUTOPILOT_LOG.md`, don't skip it — include it in the report
   with Reason/Required Action marked "no log entry found — needs
   investigation" so the inconsistency itself is visible, not hidden.
3. **Build the report as HTML first.** This system has no `pandoc` and
   no LaTeX installed (confirmed absent), so HTML → PDF via LibreOffice
   is the reliable path here, not a plain-text render. Structure:
   - Title, generation timestamp, and a one-line count ("N files
     blocked").
   - A summary table: File | Blocked Since | One-line Reason |
     Reprocess Command.
   - One detail section per file, in the same order as the table, each
     with the full Reason and Required Action text and a note pointing
     back to its `docs/AUTOPILOT_LOG.md` subsection for anyone who wants
     the complete original write-up.
   Keep the HTML plain (semantic `<table>`, minimal inline CSS) —
   LibreOffice's HTML import handles basic tables fine but is not a full
   browser rendering engine, so don't rely on modern CSS layout.
4. **Convert to PDF.** Write the HTML to a temp file, then run:
   ```
   soffice --headless --convert-to pdf --outdir <output-dir> \
     -env:UserInstallation=file:///tmp/isearch2-blocked-report-profile \
     <temp-html-file>
   ```
   The `-env:UserInstallation` override matters — without it, a
   concurrent or already-running LibreOffice instance on this machine
   can collide over the shared profile lock and the conversion silently
   fails. Confirm the output PDF exists and is non-empty before
   reporting success. If `soffice` errors or produces a zero-byte file,
   fall back to `groff -Tps -ms <file> | ps2pdf - <output>` (plain-text
   rendering, both tools confirmed present) rather than failing the
   command outright, and state in the final report which path was used.
5. **Save the final PDF** to `docs/reports/blocked-files-report-
   <YYYYMMDD-HHMMSS>.pdf` (create `docs/reports/` if it doesn't exist
   yet — mirrors the existing `tests/reports/` convention for generated,
   timestamped artifacts). Delete the intermediate HTML/temp file and
   the temporary LibreOffice profile directory afterward; don't leave
   them behind in `docs/reports/` or `/tmp`.
6. **Do not commit or push.** Unlike the per-file pipeline commands,
   this report is a disposable, point-in-time artifact, not tracked
   project state — same treatment as `tests/reports/`, which is already
   gitignored (`docs/reports/` was added alongside it for the same
   reason when this command was introduced).
7. **Report back**: the count of blocked files, the full path to the
   generated PDF, which conversion method was used (LibreOffice or the
   groff/ps2pdf fallback), and call out explicitly any blocked row that
   had no matching `docs/AUTOPILOT_LOG.md` entry.
