# Generate a PDF reference guide for all Isearch2 cleanup commands

Produces a standalone PDF listing every available command documented in
`CLAUDE.md` under the COMMANDS section, with usage syntax, full
description, and key details. Useful as a quick reference guide or for
sharing project workflows with team members.

Styled after the project's "reference manual" template (see
`TUTORIAL-reference-manual.pdf`): serif headings, sage-green note/code
boxes, a "Quick Index" summary block, and numbered `§N` sections with a
left-margin gutter — rather than a generic bordered-table report.

Run the generator directly instead of rebuilding this by hand:

```
python3 .claude/tools/generate_commands_reference.py <isearch2_root_dir>
```

What it does, for reference:

1. **Parses CLAUDE.md** to extract the COMMANDS section — locates
   `## COMMANDS` and reads every `- **NAME** — description` bullet until
   the next major section (`When you're ready to hand...`).

2. **Extracts, per command**: name, usage/invocation syntax (e.g.
   `/analyze`, `/process <filename>`), and the full description text.
   Inline `` `code` `` spans and `**bold**` markup in the description
   carry through to the rendered HTML.

3. **Builds the reference as HTML**: title + subtitle, an intro note
   box, an Audience/Format/Commands/Generated metadata line, a Quick
   Index box (usage + one-line summary per command), then one `§N`
   section per command with its full description.

4. **Converts to PDF**, preferring headless Chrome for full CSS
   fidelity (flexbox, `print-color-adjust`), falling back to LibreOffice
   and then `wkhtmltopdf` if Chrome isn't available.

5. **Saves the final PDF** to
   `docs/reports/commands-reference-<YYYYMMDD-HHMMSS>.pdf` (creating
   `docs/reports/` if needed) and cleans up temp files afterward.

6. **Does not commit or push.** This is a disposable reference artifact,
   same treatment as STATUS-REPORT and BLOCKED-REPORT.

7. **Report back**: the command count, full path to the generated PDF,
   and conversion method used (printed by the script).
