#!/usr/bin/env python3
"""
Generate a PDF code-documentation reference manual for the Isearch2
cleanup project.

Styled after the project's "reference manual" template (see
TUTORIAL-reference-manual.pdf): serif headings, sage-green note/stat
boxes, and numbered sections in a left gutter — same visual language as
the other three docs/reports/ generators.

Parses the Doxygen-style documentation written by /document-next and
/document-10 (a @file/@brief block near the top of each .cxx, plus a
@brief/@param/@return block above every function definition), and adds
a static-analysis caller/callee cross-reference: for each function,
which other functions (in the same file) it calls, and which call it.
This is a text-scan over the function bodies already extracted for
documentation, not a compiler-based analysis, so it's scoped to the
target file's own functions and can't resolve virtual dispatch,
function pointers, or macro-expanded calls -- flagged as such in the
report rather than silently presented as complete.
"""

import os
import re
import sys
import html
import shutil
import tempfile
import subprocess
from datetime import datetime


DOXYGEN_BLOCK_RE = re.compile(r'/\*\*(.*?)\*/\s*\n', re.DOTALL)
# A function signature: optional return type/qualifiers, then
# ClassName::name(...) or (for free functions) a bare name(...),
# possibly spanning multiple lines, ending in '{'.
SIGNATURE_RE = re.compile(
    r'(?:[A-Za-z_][\w:<>,\*&\s]*?\s+)?(?:([A-Za-z_]\w*)::)?(~?[A-Za-z_]\w*)\s*'
    r'\(([^;{]*?)\)\s*(?:const)?\s*\{',
    re.DOTALL,
)


def _strip_code_noise(text):
    """Blank out string/char literals and comments so brace-counting and
    call-scanning don't get confused by braces or identifiers inside
    them. Preserves length/newlines so offsets stay meaningful."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            j = n if j == -1 else j
            out.append(' ' * (j - i))
            i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j == -1 else j + 2
            seg = text[i:j]
            out.append(re.sub(r'[^\n]', ' ', seg))
            i = j
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                if text[j] == '\\':
                    j += 1
                j += 1
            j = min(j + 1, n)
            seg = text[i:j]
            out.append(re.sub(r'[^\n]', ' ', seg))
            i = j
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                if text[j] == '\\':
                    j += 1
                j += 1
            j = min(j + 1, n)
            seg = text[i:j]
            out.append(re.sub(r'[^\n]', ' ', seg))
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def _find_matching_brace(clean_text, open_pos):
    """clean_text must already have strings/comments blanked out."""
    depth = 0
    i = open_pos
    n = len(clean_text)
    while i < n:
        if clean_text[i] == '{':
            depth += 1
        elif clean_text[i] == '}':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return n - 1


def _parse_doxygen_text(raw):
    """Split a /** ... */ block's inner text into (brief, description,
    params:[(name, desc)], return_doc, is_file_level).

    @brief ends at the first blank line or the next @tag, whichever
    comes first (not just the next @tag) -- otherwise a @brief with no
    @param/@return after it (e.g. a void, no-arg function, or the
    file-level block) swallows its entire following description
    paragraph as if it were all one "brief" line.
    """
    lines = [re.sub(r'^\s*\*\s?', '', l) for l in raw.split('\n')]
    text = '\n'.join(lines).strip('\n')

    is_file = bool(re.search(r'@file\b', text))
    text = re.sub(r'^@file\s+\S+\s*\n?', '', text)

    param_matches = list(re.finditer(r'@param\s+(\S+)\s+(.*?)(?=\n\s*@\w|\Z)', text, re.DOTALL))
    return_match = re.search(r'@return\s+(.*?)(?=\n\s*@\w|\Z)', text, re.DOTALL)
    brief_match = re.search(r'@brief\s+(.*?)(?=\n\s*\n|\n\s*@\w|\Z)', text, re.DOTALL)

    params = [(m.group(1), ' '.join(m.group(2).split())) for m in param_matches]
    return_doc = ' '.join(return_match.group(1).split()) if return_match else ''
    brief = ' '.join(brief_match.group(1).split()) if brief_match else ''

    # Description: whatever's left between the end of the brief and the
    # next *metadata* tag (@param/@return) -- @code/@endcode blocks stay
    # part of the description, handled specially at render time.
    desc_text = ''
    if brief_match:
        remainder = text[brief_match.end():]
        desc_text = re.split(r'\n\s*@(?:param|return)\b', remainder, maxsplit=1)[0].strip()

    return brief, desc_text, params, return_doc, is_file


def parse_source_file(path):
    """Returns (file_doc:{brief,description}, [function dicts])."""
    with open(path, 'r', errors='ignore') as f:
        content = f.read()

    clean = _strip_code_noise(content)
    functions = []
    file_doc = {'brief': '', 'description': ''}

    for m in DOXYGEN_BLOCK_RE.finditer(content):
        brief, desc, params, return_doc, is_file = _parse_doxygen_text(m.group(1))

        if is_file:
            file_doc = {'brief': brief, 'description': desc}
            continue

        # Skip whitespace (and any blanked-out comments, which are all
        # -whitespace after _strip_code_noise) between the doc comment
        # and the signature, using the *clean* text so a stray // aside
        # in between doesn't break the match.
        ws_m = re.match(r'\s*', clean[m.end():])
        sig_start = m.end() + ws_m.end()

        sig_m = SIGNATURE_RE.match(clean[sig_start:])
        if not sig_m:
            # Doc comment not immediately followed by a recognizable
            # function signature (e.g. a member-variable comment) -- skip.
            continue

        class_name = sig_m.group(1) or ''
        func_name = sig_m.group(2)

        # Locate the '{' that SIGNATURE_RE matched, in the *original*
        # document's coordinates, then find its matching '}'.
        open_brace_pos = sig_start + sig_m.end(0) - 1
        close_brace_pos = _find_matching_brace(clean, open_brace_pos)

        body = content[open_brace_pos + 1:close_brace_pos]
        signature_text = ' '.join(content[sig_start:open_brace_pos].split())

        functions.append({
            'name': func_name,
            'qualified_name': f'{class_name}::{func_name}' if class_name else func_name,
            'class_name': class_name,
            'brief': brief,
            'description': desc,
            'params': params,
            'return_doc': return_doc,
            'signature': signature_text,
            'body': body,
            'line': content[:m.start()].count('\n') + 1,
        })

    return file_doc, functions


def build_xref(functions):
    """Adds 'calls' and 'called_by' (lists of short names) to each
    function dict, scoped to this same function set."""
    names = [f['name'] for f in functions]
    name_res = {n: re.compile(r'(?<![\w:])' + re.escape(n) + r'\s*\(') for n in set(names)}

    by_name = {}
    for f in functions:
        by_name.setdefault(f['name'], []).append(f)

    for f in functions:
        callees = set()
        for n in set(names):
            if name_res[n].search(f['body']):
                callees.add(n)
        callees.discard(f['name']) if False else None  # keep self-recursion
        f['calls'] = sorted(callees)

    for f in functions:
        f['called_by'] = sorted({
            other['name'] for other in functions
            if f['name'] in other['calls'] and other is not f
        } | ({f['name']} if f['name'] in f['calls'] else set()))


def _esc(text):
    return html.escape(str(text), quote=False)


def _render_inline(text):
    """Single-line rendering (brief, @return, @param descriptions): just
    escape + backtick-to-<code>, plus Doxygen's @p word (parameter
    cross-reference) rendered the same way, no paragraph/block handling."""
    escaped = html.escape(text, quote=False)
    escaped = re.sub(r'`([^`]+)`', r'<code>\1</code>', escaped)
    escaped = re.sub(r'@p\s+(\w+)', r'<code>\1</code>', escaped)
    return escaped


def _render_block(text):
    """Multi-paragraph rendering (descriptions): splits on blank lines
    into <p> tags, and renders @code ... @endcode spans as <pre> blocks
    instead of collapsing them into a single run-on line."""
    if not text.strip():
        return ''

    pieces = []
    last = 0
    code_re = re.compile(r'@code\s*\n(.*?)\n\s*@endcode', re.DOTALL)
    for m in code_re.finditer(text):
        before = text[last:m.start()].strip()
        if before:
            for para in re.split(r'\n\s*\n', before):
                para = para.strip()
                if para:
                    pieces.append(f'<p class="func-desc">{_render_inline(" ".join(para.split()))}</p>')
        code_text = m.group(1)
        pieces.append(f'<pre class="codeblock">{html.escape(code_text, quote=False)}</pre>')
        last = m.end()

    tail = text[last:].strip()
    if tail:
        for para in re.split(r'\n\s*\n', tail):
            para = para.strip()
            if para:
                pieces.append(f'<p class="func-desc">{_render_inline(" ".join(para.split()))}</p>')

    return '\n'.join(pieces)


def generate_html_report(files):
    """files: [(source_path, file_doc, functions)]"""
    now = datetime.now()
    generated_date = now.strftime('%Y-%m-%d')
    generated_full = now.strftime('%Y-%m-%d %H:%M:%S')

    total_functions = sum(len(fns) for _, _, fns in files)
    total_calls = sum(len(fn['calls']) for _, _, fns in files for fn in fns)

    stat_rows = [
        ('Files', f'{len(files)}'),
        ('Functions Documented', f'{total_functions}'),
        ('Call-Graph Edges Found', f'{total_calls}'),
    ]
    stat_rows_html = '\n'.join(
        f'      <div class="idx-row"><span class="idx-label">{_esc(l)}</span>'
        f'<span class="idx-value">{_esc(v)}</span></div>'
        for l, v in stat_rows
    )

    file_blocks = []
    for i, (path, file_doc, functions) in enumerate(files, start=1):
        func_blocks = []
        for fn in functions:
            params_html = ''
            if fn['params']:
                rows = '\n'.join(
                    f'          <tr><td class="mono">{_esc(p)}</td><td>{_render_inline(d)}</td></tr>'
                    for p, d in fn['params']
                )
                params_html = f"""        <table class="param-table">
          <thead><tr><th>Parameter</th><th>Description</th></tr></thead>
          <tbody>
{rows}
          </tbody>
        </table>"""

            return_html = f'<p class="return-line"><span class="tag-label">Returns</span> {_render_inline(fn["return_doc"])}</p>' if fn['return_doc'] else ''
            desc_html = _render_block(fn['description'])

            calls = fn['calls']
            called_by = fn['called_by']
            calls_html = ', '.join(f'<code>{_esc(c)}()</code>' for c in calls) if calls else '<span class="empty">none found in this file</span>'
            called_by_html = ', '.join(f'<code>{_esc(c)}()</code>' for c in called_by) if called_by else '<span class="empty">none found in this file</span>'

            func_blocks.append(f"""      <div class="func-block">
        <h3><code class="func-sig">{_esc(fn['qualified_name'])}()</code></h3>
        <p class="func-brief">{_render_inline(fn['brief'])}</p>
{desc_html}
{params_html}
{return_html}
        <div class="xref-box">
          <p class="xref-row"><span class="tag-label">Calls</span> {calls_html}</p>
          <p class="xref-row"><span class="tag-label">Called by</span> {called_by_html}</p>
        </div>
      </div>""")

        file_blocks.append(f"""    <div class="section">
      <div class="secnum">&sect;{i}</div>
      <div class="secbody">
        <h2><span class="mono">{_esc(path)}</span></h2>
        <p class="file-brief">{_render_inline(file_doc['brief'])}</p>
{_render_block(file_doc['description'])}
        <p class="section-meta">{len(functions)} function(s) documented</p>
{chr(10).join(func_blocks)}
      </div>
    </div>""")

    files_list = ', '.join(f'<code>{_esc(p)}</code>' for p, _, _ in files)

    html_doc = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Isearch2 Code Documentation Reference</title>
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
    font-size: 17px;
    font-weight: 700;
    color: #1a211c;
    margin: 0 0 4px 0;
  }}
  h3 {{
    font-size: 12.5px;
    margin: 0 0 4px 0;
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
    font-size: 11px;
    color: #2e5940;
    white-space: pre-wrap;
    word-wrap: break-word;
    break-inside: avoid;
  }}
  code.func-sig {{
    background: none;
    color: #1a211c;
    font-weight: 700;
    font-size: 1.1em;
    padding: 0;
  }}
  .note {{
    background: #dce6de;
    border-radius: 6px;
    padding: 16px 20px;
    margin: 20px 0;
    color: #4b564d;
  }}
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
    margin: 4px 0 14px 0;
  }}
  .file-brief {{
    font-weight: 600;
    color: #1a211c;
    margin: 2px 0 6px 0;
  }}
  .file-desc {{
    color: #3a423c;
    margin: 0 0 4px 0;
  }}
  .func-block {{
    background: #fbfcfb;
    border-left: 3px solid #d9ded9;
    padding: 10px 16px;
    margin: 0 0 16px 0;
    break-inside: avoid;
  }}
  .func-brief {{
    font-weight: 600;
    color: #1a211c;
    margin: 0 0 4px 0;
  }}
  .func-desc {{
    color: #3a423c;
    margin: 0 0 8px 0;
  }}
  table.param-table {{
    width: 100%;
    border-collapse: collapse;
    margin: 6px 0 8px 0;
    font-size: 11.5px;
  }}
  table.param-table th {{
    text-align: left;
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 700;
    color: #527560;
    padding: 3px 8px 3px 0;
    border-bottom: 1px solid #2e5940;
  }}
  table.param-table td {{
    padding: 3px 8px 3px 0;
    border-bottom: 1px solid #e3e8e3;
    vertical-align: top;
  }}
  td.mono {{ font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace; font-size: 11px; white-space: nowrap; }}
  .return-line {{ margin: 4px 0 8px 0; }}
  .tag-label {{
    display: inline-block;
    font-size: 9.5px;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 700;
    color: #527560;
    margin-right: 6px;
  }}
  .xref-box {{
    background: #eef2ec;
    border-radius: 3px;
    padding: 6px 10px;
    margin-top: 6px;
  }}
  .xref-row {{ margin: 2px 0; font-size: 12px; }}
  .empty {{ color: #7c8a7c; font-style: italic; }}
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
  <h1>Isearch2 Code Documentation Reference</h1>
  <p class="subtitle">File and function documentation for {files_list}, with a caller/callee cross-reference for every function.</p>

  <div class="note">
    <strong>Note.</strong> The Calls/Called&nbsp;by cross-reference below
    is a text-scan over each function's own body, scoped to the
    functions documented in this report &mdash; not a compiler-based
    whole-program call graph. It can't resolve virtual dispatch,
    function pointers, or macro-expanded calls, and a call into a
    function outside this file's own class won't appear. Treat it as a
    reading aid, not a verified analysis.
  </div>

  <div class="meta">
    <span class="meta-item"><span class="meta-label">Audience</span><span class="meta-dash">&mdash;</span><span class="meta-value">operators &amp; developers</span></span>
    <span class="meta-item"><span class="meta-label">Format</span><span class="meta-dash">&mdash;</span><span class="meta-value">code documentation reference</span></span>
    <span class="meta-item"><span class="meta-label">Generated</span><span class="meta-dash">&mdash;</span><span class="meta-value">{generated_date}</span></span>
  </div>
  <hr>

  <p class="index-heading">Summary</p>
  <div class="codebox">
{stat_rows_html}
  </div>

{chr(10).join(file_blocks)}

  <div class="footer">Isearch2 Code Documentation Reference &mdash; generated {generated_full}. Read-only, disposable output; not committed to the repository.</div>
</body>
</html>
"""
    return html_doc


def _find_chrome():
    for name in ('google-chrome', 'google-chrome-stable', 'chromium-browser', 'chromium'):
        path = shutil.which(name)
        if path:
            return path
    return None


def convert_to_pdf(html_file, output_pdf):
    chrome = _find_chrome()
    if chrome:
        temp_profile = tempfile.mkdtemp(prefix='isearch2-code-doc-chrome-')
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

    temp_profile = tempfile.mkdtemp(prefix='isearch2-code-doc-')
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


def _source_path_for(root_dir, doc_path):
    """doc_path from DOCUMENTATION_STATUS.md may be a .hxx row; the real
    documentation (and function bodies) lives in the paired .cxx."""
    if doc_path.endswith('.hxx'):
        cxx = doc_path[:-4] + '.cxx'
        if os.path.exists(os.path.join(root_dir, cxx)):
            return cxx
    return doc_path


def main():
    if len(sys.argv) < 2:
        print("Usage: generate_code_documentation_report.py <isearch2_root_dir> [file]")
        sys.exit(1)

    root_dir = sys.argv[1]
    target_file = sys.argv[2] if len(sys.argv) > 2 else None

    if target_file:
        doc_paths = [target_file]
    else:
        status_file = os.path.join(root_dir, 'docs', 'DOCUMENTATION_STATUS.md')
        if not os.path.exists(status_file):
            print(f"Error: {status_file} not found")
            sys.exit(1)
        doc_paths = []
        with open(status_file) as f:
            for line in f.readlines()[3:]:
                line = line.strip()
                if not line or line.startswith('|---'):
                    continue
                parts = [p.strip() for p in line.split('|')]
                if len(parts) >= 4 and parts[2] and parts[3] == 'done':
                    doc_paths.append(parts[2])

    if not doc_paths:
        print("Error: no documented files to report on")
        sys.exit(1)

    files = []
    for doc_path in doc_paths:
        source_path = _source_path_for(root_dir, doc_path)
        full_path = os.path.join(root_dir, source_path)
        if not os.path.exists(full_path):
            print(f"Warning: {full_path} not found, skipping")
            continue
        file_doc, functions = parse_source_file(full_path)
        if not functions:
            print(f"Warning: no documented functions found in {source_path}, skipping")
            continue
        build_xref(functions)
        files.append((source_path, file_doc, functions))

    if not files:
        print("Error: no functions could be parsed from the target file(s)")
        sys.exit(1)

    html_content = generate_html_report(files)

    with tempfile.NamedTemporaryFile(mode='w', suffix='.html', delete=False) as f:
        html_file = f.name
        f.write(html_content)

    reports_dir = os.path.join(root_dir, 'docs', 'reports')
    os.makedirs(reports_dir, exist_ok=True)

    timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    output_pdf = os.path.join(reports_dir, f'code-documentation-report-{timestamp}.pdf')

    conversion_method = convert_to_pdf(html_file, output_pdf)

    try:
        os.remove(html_file)
    except OSError:
        pass

    cleanup_temp_files(reports_dir)

    if conversion_method:
        total_functions = sum(len(fns) for _, _, fns in files)
        print(f"✓ Code documentation report generated successfully")
        print(f"  Path: {output_pdf}")
        print(f"  Conversion method: {conversion_method}")
        print(f"  Files: {len(files)}")
        print(f"  Functions documented: {total_functions}")
    else:
        print(f"✗ Failed to generate PDF (Chrome, LibreOffice, and wkhtmltopdf all unavailable)")
        if os.path.exists(output_pdf):
            os.remove(output_pdf)
        sys.exit(1)


if __name__ == '__main__':
    main()
