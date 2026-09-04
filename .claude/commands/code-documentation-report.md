# Generate a PDF code-documentation reference manual

Renders the Doxygen-style documentation written by `/document-next` and
`/document-10` as a standalone PDF: one section per file with its
file-level summary, then one entry per function with its
`@brief`/`@param`/`@return` documentation plus a caller/callee
cross-reference. Read-only — no commits, no status updates. Manual/
on-demand only, like the other `docs/reports/` generators.

Styled after the project's "reference manual" template (see
`TUTORIAL-reference-manual.pdf`), same visual language as
COMMANDS-REFERENCE, STATUS-REPORT, and BUG-CATALOG-REPORT.

Run the generator directly instead of rebuilding this by hand:

```
python3 .claude/tools/generate_code_documentation_report.py <isearch2_root_dir> [file]
```

`[file]` is optional — a specific `.cxx` (e.g. `doctype/dif.cxx`)
reports on just that file; omitted, it reports on every `done` row in
`docs/DOCUMENTATION_STATUS.md`. For a `.hxx` argument it automatically
looks for the paired `.cxx`, since that's where the full documentation
and function bodies actually live (the header only carries one-line
`@brief` comments).

What it does, for reference:

1. **Parses each target `.cxx`** for `/** ... */` Doxygen blocks
   immediately followed by a function definition: extracts `@brief`,
   the free-form description (including any `@code`/`@endcode` grammar
   or example blocks), `@param` entries, and `@return`, plus the
   function's own body text (via brace-depth matching that skips over
   string/char literals and comments, so a stray `{`/`}` inside a
   string doesn't throw off the boundary). The file-level `@file`/
   `@brief` block is parsed the same way for the file's own summary.

2. **Builds the caller/callee cross-reference**: for every function
   found, scans every *other* found function's body for a call to it
   (and vice versa) via a word-boundary regex — including self-calls,
   so direct recursion (e.g. `dif.cxx`'s `atomtail()` calling itself)
   shows up correctly. This is a text-scan scoped to the functions in
   the target file(s), **not** a compiler-based whole-program call
   graph — it can't resolve virtual dispatch, function pointers, or
   macro-expanded calls, and a call into another class/file won't
   appear. The report says so up front rather than presenting it as a
   verified analysis.

3. **Builds the report as HTML**: title + subtitle, an intro note about
   the cross-reference's scope/limits, a summary stat box, then one
   `§N` section per file — file summary, then one block per function
   with its brief (bold), description (including any rendered code/
   grammar block), a parameter table, the return-value note, and a
   Calls/Called-by box.

4. **Converts to PDF**, preferring headless Chrome for full CSS
   fidelity, falling back to LibreOffice, then `wkhtmltopdf`.

5. **Saves the final PDF** to `docs/reports/code-documentation-report-
   <YYYYMMDD-HHMMSS>.pdf` (creating `docs/reports/` if needed).

6. **Does not commit or push.** Disposable, gitignored output, same as
   the other three report generators.

7. **Report back**: files covered, total functions documented, full
   path to the generated PDF, and conversion method used (printed by
   the script).
