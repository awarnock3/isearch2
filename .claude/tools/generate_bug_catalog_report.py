#!/usr/bin/env python3
"""
Generate a PDF bug-catalog report for the Isearch2 cleanup project.

Styled after the project's "reference manual" template (see
TUTORIAL-reference-manual.pdf): serif headings, sage-green note/stat
boxes, and numbered sections in a left gutter — same visual language as
generate_commands_reference.py and generate_status_report.py.

Reads docs/BUG_CATALOG.md (one "## <file>" section per cataloged file,
each with a numbered list of bugs, plus occasional "### ..." subsections
holding bullet-point findings deferred to other files' turns) and
cross-references every cataloged file against its *current* row in
docs/PROCESSING_STATUS.md. That cross-reference matters because
RESCAN-STATUS can flip a "done" file back to "pending" (e.g. after a
sync-upstream merge changes it) without touching BUG_CATALOG.md, so a
catalog entry's "Fixed" claim can outlive the code it was fixed against.
Files in that state are surfaced in their own section rather than left
to look as settled as everything else.

Also does a best-effort text scan of the catalog for "dead" code/file
mentions (the project's own recurring term for unreferenced functions
and files found during cleanup) to build a Dead Files & Functions table.
This is a heuristic over prose, not a static-analysis result — flagged
as such in the report.
"""

import os
import re
import sys
import html
import shutil
import textwrap
import tempfile
import subprocess
from datetime import datetime


FENCE_RE = re.compile(r'( *)```([a-zA-Z0-9]*)\n(.*?)\n\1```', re.DOTALL)
DEAD_WORD_RE = re.compile(r'\bdead\b', re.IGNORECASE)
NEG_NEAR_RE = re.compile(r"\b(not|n't|never)\b", re.IGNORECASE)
FENCE_TOKEN_RE = re.compile(r'\x01FENCE_START\x01(.*?)\x01(.*?)\x01FENCE_END\x01', re.DOTALL)


def parse_processing_status(status_file):
    """Parse PROCESSING_STATUS.md and return {path: {status, order, last_processed}}."""
    by_path = {}
    with open(status_file, 'r') as f:
        lines = f.readlines()

    for line in lines[3:]:
        line = line.strip()
        if not line or line.startswith('|---|'):
            continue
        parts = [p.strip() for p in line.split('|')]
        if len(parts) < 5:
            continue
        order, file_path, status, last_processed = parts[1], parts[2], parts[3], parts[4]
        if file_path and status in ('done', 'blocked', 'pending', 'generated'):
            by_path[file_path] = {
                'status': status,
                'order': order,
                'last_processed': last_processed,
            }
    return by_path


def parse_bug_catalog(catalog_path):
    """Parse docs/BUG_CATALOG.md into an ordered list of per-file sections.

    Returns [(file_path, intro_text, [(num, item_text), ...], [deferred_text, ...])].

    Fenced ``` code blocks are protected before line-based parsing (so an
    embedded blank line inside a fence doesn't split a list item in two),
    then restored as \\x01FENCE_START\\x01<lang>\\x01<code>\\x01FENCE_END\\x01
    tokens for the renderer to turn into <pre> blocks.
    """
    with open(catalog_path, 'r') as f:
        content = f.read()

    fences = []

    def stash_fence(m):
        indent, lang, code = m.group(1), m.group(2), m.group(3)
        fences.append((lang, code))
        return f'{indent}\x00FENCE{len(fences) - 1}\x00'

    protected = FENCE_RE.sub(stash_fence, content)

    def restore_fences(text):
        def sub(m):
            idx = int(m.group(1))
            lang, code = fences[idx]
            return f'\n\x01FENCE_START\x01{lang}\x01{code}\x01FENCE_END\x01\n'
        return re.sub(r'\x00FENCE(\d+)\x00', sub, text)

    parts = re.split(r'\n(?=## )', protected)
    sections = []
    for p in parts:
        m = re.match(r'## (.+?)\n(.*)', p, re.DOTALL)
        if m:
            sections.append((m.group(1).strip(), m.group(2)))

    results = []
    for path, body in sections:
        intro, items, deferred = _parse_section_body(body)
        intro = restore_fences(intro)
        items = [(num, restore_fences(text)) for num, text in items]
        deferred = [restore_fences(text) for text in deferred]
        results.append((path, intro, items, deferred))
    return results


def _parse_section_body(body):
    """Line-based parse of one '## file' section body.

    Handles: numbered bug items (bold titles that can wrap across lines,
    and non-bold-led items like "3. Unconditional **X** ..."), '### ...'
    subheadings (deferred-findings, build-infra asides, etc. — content
    under them can be either more numbered items or '- **' bullets),
    top-level '- **' deferred bullets, and trailing unindented prose
    (Modernization notes etc., which aren't part of any single bug and
    are dropped rather than mis-attributed to the last item).
    """
    lines = body.split('\n')
    numbered_items, deferred_bullets, intro_chunks = [], [], []
    cur_kind, cur_lines = None, []
    seen_item = False
    in_subsection = False

    def flush():
        nonlocal cur_kind, cur_lines
        text = '\n'.join(cur_lines).strip('\n')
        if cur_kind == 'item' and text:
            m = re.match(r'^(\d+)\.\s+(.*)', text, re.DOTALL)
            if m:
                numbered_items.append((m.group(1), m.group(2).strip()))
        elif cur_kind == 'deferred' and text:
            m = re.match(r'^-\s+(.*)', text, re.DOTALL)
            if m:
                deferred_bullets.append(m.group(1).strip())
        elif cur_kind == 'intro' and text:
            intro_chunks.append(text)
        cur_kind, cur_lines = None, []

    for line in lines:
        is_blank = line.strip() == ''
        is_indented = len(line) > 0 and line[0] in (' ', '\t')
        starts_numbered = re.match(r'^\d+\.\s', line)
        starts_bullet = line.startswith('- ')
        starts_h3 = line.startswith('### ')

        if starts_h3:
            flush()
            in_subsection = True
            continue
        if starts_numbered:
            flush()
            cur_kind, cur_lines = 'item', [line]
            seen_item = True
            continue
        if starts_bullet:
            flush()
            cur_kind, cur_lines = 'deferred', [line]
            continue
        if is_blank:
            if cur_kind is not None:
                cur_lines.append(line)
            continue
        if is_indented and cur_kind in ('item', 'deferred', 'intro'):
            cur_lines.append(line)
            continue
        if not seen_item and not in_subsection:
            if cur_kind != 'intro':
                flush()
                cur_kind = 'intro'
            cur_lines.append(line)
            continue
        # Unindented prose with no list marker, after the list has
        # started: trailing boilerplate (Modernization:, Incidental fix
        # notes, etc.) — not part of any single cataloged bug, dropped.
        flush()

    flush()
    return '\n'.join(intro_chunks), numbered_items, deferred_bullets


def _flatten(text):
    return re.sub(r'\s+', ' ', text)


def _is_negated(text, idx, window=18):
    return bool(NEG_NEAR_RE.search(text[max(0, idx - window):idx]))


def _sentence_around(text, idx):
    # Only '. ' (period + space) counts as a sentence boundary, so a bare
    # period inside a filename/identifier (e.g. "nfldmgr.o") doesn't get
    # mistaken for one.
    start = 0
    for m in re.finditer(r'\.\s', text[:idx]):
        start = m.end()
    m = re.search(r'\.\s', text[idx:])
    end = idx + m.end() if m else len(text)
    return text[start:end].strip()


def find_dead_code(catalog_sections):
    """Heuristic scan for 'dead' file/function mentions, negation-filtered.

    Returns (dead_files, dead_code) where dead_files is [(path, snippet)]
    for whole-file dead-code intros, and dead_code is [(path, snippet)]
    for narrower dead-function/branch/variable findings inside individual
    bug entries.
    """
    dead_files, dead_code = [], []
    for path, intro, items, deferred in catalog_sections:
        flat_intro = _flatten(intro)
        m = DEAD_WORD_RE.search(flat_intro)
        if m and not _is_negated(flat_intro, m.start()):
            dead_files.append((path, _sentence_around(flat_intro, m.start())))

        for num, text in items:
            flat = _flatten(text)
            m = DEAD_WORD_RE.search(flat)
            if m and not _is_negated(flat, m.start()):
                dead_code.append((path, _sentence_around(flat, m.start())))
        for text in deferred:
            flat = _flatten(text)
            m = DEAD_WORD_RE.search(flat)
            if m and not _is_negated(flat, m.start()):
                dead_code.append((path, _sentence_around(flat, m.start())))

    return dead_files, dead_code


def _render_prose(text):
    """Escape HTML, then turn **bold**/`code` spans into tags and collapse
    Markdown line-wrap whitespace (rejoining hyphen/slash-wrapped tokens
    with no space, everything else with one)."""
    escaped = html.escape(text, quote=False)
    escaped = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', escaped)
    escaped = re.sub(r'\*([^*\n]+)\*', r'<em>\1</em>', escaped)
    escaped = re.sub(r'`([^`]+)`', r'<code>\1</code>', escaped)
    escaped = re.sub(r'[ \t]*\n[ \t]*', '\n', escaped)
    escaped = re.sub(r'([-/])\n', r'\1', escaped)
    escaped = escaped.replace('\n', ' ')
    escaped = re.sub(r' {2,}', ' ', escaped)
    return escaped.strip()


def render_item_html(text):
    """Render one bug-entry's raw text (prose + optional fenced code
    blocks) to HTML, dedenting fenced code and leaving prose to
    _render_prose."""
    pieces = []
    last = 0
    for m in FENCE_TOKEN_RE.finditer(text):
        if m.start() > last:
            pieces.append(_render_prose(text[last:m.start()]))
        lang, code = m.group(1), m.group(2)
        code = textwrap.dedent(code)
        pieces.append(f'<pre class="codeblock">{html.escape(code, quote=False)}</pre>')
        last = m.end()
    if last < len(text):
        pieces.append(_render_prose(text[last:]))
    return ''.join(pieces)


STATUS_LABEL = {
    'done': 'done',
    'pending': 'pending',
    'blocked': 'blocked',
    'generated': 'generated',
    'unknown': 'not tracked',
}


def generate_html_report(catalog_sections, status_by_path):
    """Generate HTML report content in the reference-manual style."""
    files_cataloged = len(catalog_sections)
    total_bugs = sum(len(items) for _, _, items, _ in catalog_sections)
    total_deferred = sum(len(deferred) for _, _, _, deferred in catalog_sections)

    dead_files, dead_code = find_dead_code(catalog_sections)

    reopened = []
    for path, intro, items, deferred in catalog_sections:
        info = status_by_path.get(path)
        status = info['status'] if info else 'unknown'
        if status != 'done':
            reopened.append((path, len(items), status, info.get('last_processed', '') if info else ''))

    now = datetime.now()
    generated_date = now.strftime('%Y-%m-%d')
    generated_full = now.strftime('%Y-%m-%d %H:%M:%S')

    stat_rows = [
        ('Files Cataloged', f'{files_cataloged}'),
        ('Bugs Fixed (primary)', f'{total_bugs}'),
        ('Deferred Findings', f'{total_deferred}'),
        ('Dead Code Findings', f'{len(dead_files) + len(dead_code)}'),
        ('Reopened Since Cataloged', f'{len(reopened)}'),
    ]
    stat_rows_html = '\n'.join(
        f'      <div class="idx-row"><span class="idx-label">{html.escape(label)}</span>'
        f'<span class="idx-value">{html.escape(value)}</span></div>'
        for label, value in stat_rows
    )

    # --- Dead Files & Functions table ---
    dead_rows = []
    for path, snippet in dead_files:
        dead_rows.append((path, 'Whole file', snippet))
    for path, snippet in dead_code:
        dead_rows.append((path, 'Function / code', snippet))
    dead_rows.sort(key=lambda r: r[0])

    if dead_rows:
        dead_body = '\n'.join(
            f'        <tr><td class="mono">{html.escape(path)}</td>'
            f'<td>{html.escape(scope)}</td>'
            f'<td>{_render_prose(snippet)}</td></tr>'
            for path, scope, snippet in dead_rows
        )
    else:
        dead_body = '        <tr><td class="empty" colspan="3">No dead-code mentions found.</td></tr>'

    # --- Reopened Since Cataloged table ---
    reopened_sorted = sorted(reopened, key=lambda r: r[0])
    if reopened_sorted:
        reopened_body = '\n'.join(
            f'        <tr><td class="mono">{html.escape(path)}</td>'
            f'<td class="num">{count}</td>'
            f'<td><span class="badge badge-{status}">{html.escape(STATUS_LABEL.get(status, status))}</span></td>'
            f'<td class="num">{html.escape(last_processed)}</td></tr>'
            for path, count, status, last_processed in reopened_sorted
        )
    else:
        reopened_body = '        <tr><td class="empty" colspan="4">None &mdash; every cataloged file is still <code>done</code>.</td></tr>'

    # --- Per-file catalog listing ---
    file_blocks = []
    for path, intro, items, deferred in catalog_sections:
        info = status_by_path.get(path)
        status = info['status'] if info else 'unknown'
        last_processed = info.get('last_processed', '') if info else ''

        intro_html = f'<p class="file-intro">{render_item_html(intro)}</p>' if intro.strip() else ''

        items_html = '\n'.join(
            f'        <li>{render_item_html(text)}</li>'
            for _num, text in items
        )
        items_block = f'      <ol class="bug-list">\n{items_html}\n      </ol>' if items else ''

        deferred_block = ''
        if deferred:
            deferred_html = '\n'.join(
                f'        <li>{render_item_html(text)}</li>'
                for text in deferred
            )
            deferred_block = f"""      <div class="deferred-box">
        <p class="deferred-label">Found but out of scope for this file (deferred to their own turns)</p>
        <ul class="bug-list">
{deferred_html}
        </ul>
      </div>"""

        status_note = ''
        if status != 'done':
            status_note = (
                '      <p class="section-note">&#9888; Reopened since these bugs were cataloged '
                '&mdash; current status is '
                f'<span class="badge badge-{status}">{html.escape(STATUS_LABEL.get(status, status))}</span>. '
                'Treat the entries below as provisional until this file is reprocessed.</p>'
            )

        file_blocks.append(f"""    <div class="file-block">
      <h3><span class="mono">{html.escape(path)}</span>
        <span class="badge badge-{status}">{html.escape(STATUS_LABEL.get(status, status))}</span></h3>
      <p class="section-meta">{len(items)} bug(s){f', {len(deferred)} deferred finding(s)' if deferred else ''}{f' &middot; last processed {html.escape(last_processed)}' if last_processed else ''}</p>
{status_note}
{intro_html}
{items_block}
{deferred_block}
    </div>""")

    file_blocks_html = '\n'.join(file_blocks)

    html_doc = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Isearch2 Bug Catalog Report</title>
<style>
  @page {{ size: letter; margin: 0.9in 0.85in; }}
  * {{ box-sizing: border-box; }}
  html, body {{
    -webkit-print-color-adjust: exact;
    print-color-adjust: exact;
  }}
  body {{
    margin: 0;
    font-family: -apple-system, BlinkMacSystemFont, 'Noto Sans', 'Liberation Sans', Arial, sans-serif;
    color: #3a423c;
    font-size: 13px;
    line-height: 1.6;
  }}
  h1 {{
    font-family: Georgia, 'Noto Serif', 'Liberation Serif', serif;
    font-size: 30px;
    font-weight: 700;
    color: #1a211c;
    margin: 0 0 10px 0;
  }}
  h2 {{
    font-family: Georgia, 'Noto Serif', 'Liberation Serif', serif;
    font-size: 16px;
    font-weight: 700;
    color: #1a211c;
    margin: 0 0 2px 0;
  }}
  h3 {{
    font-family: Georgia, 'Noto Serif', 'Liberation Serif', serif;
    font-size: 13.5px;
    font-weight: 700;
    color: #1a211c;
    margin: 0 0 2px 0;
  }}
  .subtitle {{
    font-size: 14px;
    color: #4b564d;
    margin: 0 0 20px 0;
    max-width: 36em;
  }}
  code {{
    background: #dce6de;
    color: #2e5940;
    font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace;
    padding: 1px 5px;
    border-radius: 3px;
    font-size: 0.9em;
  }}
  pre.codeblock {{
    background: #eef2ec;
    border-left: 3px solid #2e5940;
    border-radius: 3px;
    padding: 10px 14px;
    margin: 8px 0;
    font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace;
    font-size: 10.5px;
    color: #2e5940;
    white-space: pre-wrap;
    word-wrap: break-word;
    break-inside: avoid;
  }}
  .note {{
    background: #dce6de;
    border-radius: 6px;
    padding: 16px 20px;
    margin: 20px 0;
    color: #4b564d;
  }}
  .note strong {{ color: #1a211c; }}
  .meta {{
    display: flex;
    flex-wrap: wrap;
    gap: 28px;
    padding: 14px 0;
    margin-bottom: 4px;
  }}
  .meta-item {{ font-size: 12.5px; }}
  .meta-label {{ color: #4b564d; font-weight: 700; }}
  .meta-dash {{ color: #7c8a7c; margin: 0 4px; }}
  .meta-value {{ color: #7c8a7c; }}
  hr {{
    border: none;
    border-top: 1px solid #d9ded9;
    margin: 0 0 24px 0;
  }}
  .codebox {{
    background: #eef2ec;
    border-radius: 4px;
    padding: 16px 20px;
    margin: 18px 0 32px 0;
  }}
  .idx-row {{
    display: flex;
    justify-content: space-between;
    gap: 18px;
    padding: 3px 0;
    font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace;
    font-size: 11.5px;
    break-inside: avoid;
  }}
  .idx-label {{ color: #4b564d; }}
  .idx-value {{ color: #2e5940; font-weight: 700; }}
  .index-heading {{
    font-family: Georgia, 'Noto Serif', 'Liberation Serif', serif;
    font-size: 13px;
    font-weight: 700;
    color: #527560;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    margin: 0 0 10px 0;
  }}
  .section {{
    display: flex;
    gap: 22px;
    margin: 0 0 12px 0;
  }}
  .secnum {{
    flex: 0 0 30px;
    color: #527560;
    font-size: 12px;
    padding-top: 3px;
  }}
  .secbody {{ flex: 1; min-width: 0; }}
  .section-meta {{
    font-size: 12px;
    color: #7c8a7c;
    margin: 0 0 8px 0;
  }}
  .section-note {{
    font-size: 12px;
    color: #8a5a1f;
    background: #f3e6cf;
    border-radius: 4px;
    padding: 8px 12px;
    margin: 0 0 10px 0;
  }}
  table {{
    width: 100%;
    border-collapse: collapse;
    margin: 0 0 34px 0;
    font-size: 11.5px;
  }}
  thead {{ display: table-header-group; }}
  th {{
    text-align: left;
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 700;
    color: #527560;
    padding: 6px 10px;
    border-bottom: 1px solid #2e5940;
  }}
  th.num {{ text-align: right; }}
  td {{
    padding: 6px 10px;
    border-bottom: 1px solid #e3e8e3;
    color: #3a423c;
    vertical-align: top;
  }}
  tr {{ break-inside: avoid; }}
  tbody tr:nth-child(even) td {{ background: #f5f8f5; }}
  td.mono {{ font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace; font-size: 11px; white-space: nowrap; }}
  td.num {{ text-align: right; white-space: nowrap; color: #4b564d; font-variant-numeric: tabular-nums; }}
  td.empty {{ color: #7c8a7c; font-style: italic; }}
  .badge {{
    display: inline-block;
    font-size: 9px;
    text-transform: uppercase;
    letter-spacing: 0.04em;
    font-weight: 700;
    padding: 2px 7px;
    border-radius: 9px;
    vertical-align: middle;
  }}
  .badge-done {{ background: #dce6de; color: #2e5940; }}
  .badge-pending {{ background: #f3e6cf; color: #8a5a1f; }}
  .badge-blocked {{ background: #f3d9d9; color: #8a2f2f; }}
  .badge-generated {{ background: #e5e5e5; color: #555; }}
  .badge-unknown {{ background: #e5e5e5; color: #666; }}
  .file-block {{
    margin: 0 0 26px 0;
    padding-bottom: 4px;
    break-inside: avoid-page;
  }}
  .file-intro {{
    color: #4b564d;
    font-style: italic;
    margin: 4px 0 10px 0;
  }}
  ol.bug-list, ul.bug-list {{
    margin: 6px 0 6px 0;
    padding-left: 22px;
  }}
  ol.bug-list li, ul.bug-list li {{
    margin: 0 0 10px 0;
    break-inside: avoid;
  }}
  .deferred-box {{
    background: #f5f8f5;
    border-radius: 4px;
    padding: 10px 16px;
    margin: 10px 0 0 0;
  }}
  .deferred-label {{
    font-size: 11px;
    color: #7c8a7c;
    font-style: italic;
    margin: 0 0 6px 0;
  }}
  .footer {{
    border-top: 1px solid #d9ded9;
    margin-top: 8px;
    padding-top: 14px;
    color: #7c8a7c;
    font-size: 11px;
    font-style: italic;
  }}
</style>
</head>
<body>
  <h1>Isearch2 Bug Catalog Report</h1>
  <p class="subtitle">Every bug found and fixed while processing <code>src/</code>, <code>doctype/</code>, and <code>Isearch-cgi/</code>, cross-referenced against each file's current status &mdash; since rescan-status can reopen a cataloged file for reprocessing after this catalog was written.</p>

  <div class="note">
    <strong>Note.</strong> {total_bugs} bugs cataloged across {files_cataloged} files
    ({total_deferred} additional findings deferred to other files' turns).
    {len(reopened)} file(s) have been reopened (<code>pending</code>/<code>blocked</code>)
    since their last catalog entry and should be treated as provisional
    until reprocessed &mdash; see &sect;2.
  </div>

  <div class="meta">
    <span class="meta-item"><span class="meta-label">Audience</span><span class="meta-dash">&mdash;</span><span class="meta-value">operators &amp; developers</span></span>
    <span class="meta-item"><span class="meta-label">Format</span><span class="meta-dash">&mdash;</span><span class="meta-value">bug catalog</span></span>
    <span class="meta-item"><span class="meta-label">Files Cataloged</span><span class="meta-dash">&mdash;</span><span class="meta-value">{files_cataloged}</span></span>
    <span class="meta-item"><span class="meta-label">Bugs</span><span class="meta-dash">&mdash;</span><span class="meta-value">{total_bugs}</span></span>
    <span class="meta-item"><span class="meta-label">Generated</span><span class="meta-dash">&mdash;</span><span class="meta-value">{generated_date}</span></span>
  </div>
  <hr>

  <p class="index-heading">Summary Statistics</p>
  <div class="codebox">
{stat_rows_html}
  </div>

  <div class="section">
    <div class="secnum">&sect;1</div>
    <div class="secbody">
      <h2>Dead Files &amp; Functions</h2>
      <p class="section-meta">{len(dead_rows)} finding(s) &middot; heuristic scan of this catalog's own text for the word &ldquo;dead&rdquo; (negation-filtered) &mdash; a starting point for verification, not a static-analysis-verified list. See &sect;3 for full entries.</p>
      <table>
        <thead><tr><th>File</th><th>Scope</th><th>Finding</th></tr></thead>
        <tbody>
{dead_body}
        </tbody>
      </table>
    </div>
  </div>

  <div class="section">
    <div class="secnum">&sect;2</div>
    <div class="secbody">
      <h2>Reopened Since Cataloged</h2>
      <p class="section-meta">Cataloged files whose <code>docs/PROCESSING_STATUS.md</code> row is no longer <code>done</code> &mdash; typically rescan-status reopening a file that changed underneath it (e.g. via sync-upstream).</p>
      <table>
        <thead><tr><th>File</th><th class="num">Cataloged Bugs</th><th>Current Status</th><th class="num">Last Processed</th></tr></thead>
        <tbody>
{reopened_body}
        </tbody>
      </table>
    </div>
  </div>

  <div class="section">
    <div class="secnum">&sect;3</div>
    <div class="secbody">
      <h2>Bug Catalog</h2>
      <p class="section-meta">One entry per file, in the order processed. Each numbered item is a cataloged bug; deferred findings (spotted while processing this file, but living in another file) are listed separately beneath it.</p>
{file_blocks_html}
    </div>
  </div>

  <div class="footer">Isearch2 Bug Catalog Report &mdash; generated {generated_full} from docs/BUG_CATALOG.md and docs/PROCESSING_STATUS.md. Read-only, disposable output; not committed to the repository.</div>
</body>
</html>
"""

    return html_doc


def _find_chrome():
    """Locate a headless-capable Chrome/Chromium binary, if any."""
    for name in ('google-chrome', 'google-chrome-stable', 'chromium-browser', 'chromium'):
        path = shutil.which(name)
        if path:
            return path
    return None


def convert_to_pdf(html_file, output_pdf):
    """Convert HTML to PDF.

    Prefers headless Chrome, which is what produced the reference-manual
    template this report's style is modeled on (full flexbox + print-color
    support, plus repeating table headers across page breaks). Falls back
    to LibreOffice, then wkhtmltopdf.
    """
    chrome = _find_chrome()
    if chrome:
        temp_profile = tempfile.mkdtemp(prefix='isearch2-bug-catalog-chrome-')
        try:
            subprocess.run(
                [
                    chrome, '--headless=new', '--disable-gpu', '--no-sandbox',
                    f'--user-data-dir={temp_profile}',
                    f'--print-to-pdf={output_pdf}',
                    '--no-pdf-header-footer',
                    '--virtual-time-budget=8000',
                    f'file://{os.path.abspath(html_file)}',
                ],
                capture_output=True,
                timeout=90,
                text=True,
            )
            if os.path.exists(output_pdf) and os.path.getsize(output_pdf) > 0:
                return 'Chrome'
        except (subprocess.TimeoutExpired, OSError):
            pass
        finally:
            shutil.rmtree(temp_profile, ignore_errors=True)

    temp_profile = tempfile.mkdtemp(prefix='isearch2-bug-catalog-')
    output_dir = os.path.dirname(output_pdf)
    try:
        subprocess.run(
            [
                'soffice', '--headless', '--convert-to', 'pdf',
                '--outdir', output_dir,
                f'-env:UserInstallation=file://{temp_profile}',
                html_file
            ],
            capture_output=True,
            timeout=120,
            text=True
        )
        html_basename = os.path.basename(html_file)
        pdf_basename = html_basename.replace('.html', '.pdf')
        temp_pdf = os.path.join(output_dir, pdf_basename)

        if os.path.exists(temp_pdf) and os.path.getsize(temp_pdf) > 0:
            try:
                os.rename(temp_pdf, output_pdf)
            except OSError:
                shutil.copy2(temp_pdf, output_pdf)
                os.remove(temp_pdf)
            return 'LibreOffice'
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError):
        pass
    finally:
        shutil.rmtree(temp_profile, ignore_errors=True)

    try:
        result = subprocess.run(
            ['wkhtmltopdf', '--quiet', html_file, output_pdf],
            capture_output=True,
            timeout=60
        )
        if result.returncode == 0 and os.path.exists(output_pdf) and os.path.getsize(output_pdf) > 0:
            return 'wkhtmltopdf'
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    return None


def cleanup_temp_files(reports_dir):
    """Remove temporary PDF files from previous failed conversions."""
    try:
        for filename in os.listdir(reports_dir):
            if filename.startswith('tmp') and filename.endswith('.pdf'):
                filepath = os.path.join(reports_dir, filename)
                try:
                    os.remove(filepath)
                except OSError:
                    pass
    except OSError:
        pass


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: generate_bug_catalog_report.py <isearch2_root_dir>")
        sys.exit(1)

    root_dir = sys.argv[1]
    catalog_file = os.path.join(root_dir, 'docs', 'BUG_CATALOG.md')
    status_file = os.path.join(root_dir, 'docs', 'PROCESSING_STATUS.md')

    if not os.path.exists(catalog_file):
        print(f"Error: {catalog_file} not found")
        sys.exit(1)
    if not os.path.exists(status_file):
        print(f"Error: {status_file} not found")
        sys.exit(1)

    catalog_sections = parse_bug_catalog(catalog_file)
    if not catalog_sections:
        print("Error: No cataloged files found in docs/BUG_CATALOG.md")
        sys.exit(1)

    status_by_path = parse_processing_status(status_file)

    html_content = generate_html_report(catalog_sections, status_by_path)

    with tempfile.NamedTemporaryFile(mode='w', suffix='.html', delete=False) as f:
        html_file = f.name
        f.write(html_content)

    reports_dir = os.path.join(root_dir, 'docs', 'reports')
    os.makedirs(reports_dir, exist_ok=True)

    timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    output_pdf = os.path.join(reports_dir, f'bug-catalog-report-{timestamp}.pdf')

    conversion_method = convert_to_pdf(html_file, output_pdf)

    try:
        os.remove(html_file)
    except OSError:
        pass

    cleanup_temp_files(reports_dir)

    if conversion_method:
        total_bugs = sum(len(items) for _, _, items, _ in catalog_sections)
        reopened = sum(
            1 for path, _, _, _ in catalog_sections
            if status_by_path.get(path, {}).get('status', 'unknown') != 'done'
        )
        print(f"✓ Bug catalog report generated successfully")
        print(f"  Path: {output_pdf}")
        print(f"  Conversion method: {conversion_method}")
        print(f"  Files cataloged: {len(catalog_sections)}")
        print(f"  Bugs: {total_bugs}")
        print(f"  Reopened since cataloged: {reopened}")
    else:
        print(f"✗ Failed to generate PDF (Chrome, LibreOffice, and wkhtmltopdf all unavailable)")
        if os.path.exists(output_pdf):
            os.remove(output_pdf)
        sys.exit(1)


if __name__ == '__main__':
    main()
