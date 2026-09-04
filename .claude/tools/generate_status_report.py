#!/usr/bin/env python3
"""
Generate a comprehensive PDF status report for Isearch2 cleanup progress.

Styled after the project's "reference manual" template (see
TUTORIAL-reference-manual.pdf): serif headings, sage-green note/stat
boxes, and numbered sections in a left gutter — same visual language as
generate_commands_reference.py.
"""

import re
import os
import sys
import html
import shutil
import tempfile
import subprocess
from datetime import datetime
from pathlib import Path
from collections import defaultdict

def parse_processing_status(status_file):
    """Parse PROCESSING_STATUS.md and return file statuses."""
    files_by_status = defaultdict(list)

    with open(status_file, 'r') as f:
        lines = f.readlines()

    # Skip header (first 3 lines: title + blank + table header)
    for line in lines[3:]:
        line = line.strip()
        if not line or line.startswith('|---|'):
            continue

        # Parse markdown table row
        parts = [p.strip() for p in line.split('|')]
        if len(parts) < 5:
            continue

        try:
            order = parts[1]
            file_path = parts[2]
            status = parts[3]
            last_processed = parts[4]

            if file_path and status in ['done', 'blocked', 'pending', 'generated']:
                files_by_status[status].append({
                    'path': file_path,
                    'order': order,
                    'last_processed': last_processed if last_processed else '',
                })
        except (IndexError, ValueError):
            continue

    return dict(files_by_status)

def count_functions(file_path):
    """Count functions/subroutines in a source file."""
    if not os.path.exists(file_path):
        return None

    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception:
        return None

    # Remove comments to avoid counting function-like patterns in comments
    content = re.sub(r'//.*$', '', content, flags=re.MULTILINE)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)

    count = 0

    # C++ method/function declarations and definitions
    # Pattern: return_type name(...) [const] [noexcept] {;
    patterns = [
        r'^\s*(?:virtual|static|inline|constexpr)?\s*(?:const\s+)?[\w:]+[\*&]?\s+[\w:]+\s*\([^)]*\)\s*(?:const)?\s*(?:noexcept)?\s*(?:override)?\s*[{;]',
        r'^\s*[\w:]+[\*&]\s+[\w:]+\s*\([^)]*\)\s*(?:const)?\s*(?:noexcept)?\s*[{;]',
        r'^[\w:]+\s+[\w:]+\s*\([^)]*\)\s*[{;]',
    ]

    for line in content.split('\n'):
        # Skip preprocessor, namespace, class, struct definitions
        if re.match(r'\s*#\s*(?:define|include|ifdef|if|endif)', line):
            continue
        if re.match(r'\s*(?:namespace|class|struct|union|typedef|using)\s', line):
            continue

        # Check for function-like patterns
        for pattern in patterns:
            if re.match(pattern, line):
                count += 1
                break

    return count if count > 0 else 0

def _esc(text):
    return html.escape(str(text), quote=False)


def generate_html_report(files_by_status, root_dir):
    """Generate HTML report content in the reference-manual style."""
    done_files = files_by_status.get('done', [])
    blocked_files = files_by_status.get('blocked', [])
    pending_files = files_by_status.get('pending', [])
    generated_files = files_by_status.get('generated', [])

    # Count functions for each file
    all_files_with_counts = {}
    total_functions = 0
    done_functions = 0
    blocked_functions = 0
    pending_functions = 0

    for status, file_list in files_by_status.items():
        if status == 'generated':
            continue  # Skip generated files for function counting

        for file_info in file_list:
            file_path = os.path.join(root_dir, file_info['path'])
            func_count = count_functions(file_path) or 0
            all_files_with_counts[file_info['path']] = func_count
            total_functions += func_count

            if status == 'done':
                done_functions += func_count
            elif status == 'blocked':
                blocked_functions += func_count
            elif status == 'pending':
                pending_functions += func_count

    # Calculate statistics
    non_generated_count = len(done_files) + len(blocked_files) + len(pending_files)
    total_files = non_generated_count + len(generated_files)
    done_count = len(done_files)
    blocked_count = len(blocked_files)
    pending_count = len(pending_files)
    generated_count = len(generated_files)

    done_pct = (done_count / non_generated_count * 100) if non_generated_count > 0 else 0
    blocked_pct = (blocked_count / non_generated_count * 100) if non_generated_count > 0 else 0
    pending_pct = (pending_count / non_generated_count * 100) if non_generated_count > 0 else 0

    avg_functions = total_functions / non_generated_count if non_generated_count > 0 else 0
    avg_done_functions = done_functions / done_count if done_count > 0 else 0

    now = datetime.now()
    generated_date = now.strftime('%Y-%m-%d')
    generated_full = now.strftime('%Y-%m-%d %H:%M:%S')

    # Quick stats block (mirrors the Quick Index box in commands-reference)
    stat_rows = [
        ('Processed (Done)', f'{done_count} / {non_generated_count}  ({done_pct:.1f}%)'),
        ('Blocked', f'{blocked_count} / {non_generated_count}  ({blocked_pct:.1f}%)'),
        ('Pending', f'{pending_count} / {non_generated_count}  ({pending_pct:.1f}%)'),
        ('Total Functions', f'{total_functions:,}'),
        ('Avg Functions / File', f'{avg_functions:.1f}'),
        ('Avg Functions / Done File', f'{avg_done_functions:.1f}'),
        ('Generated Files', f'{generated_count}'),
    ]
    stat_rows_html = '\n'.join(
        f'      <div class="idx-row"><span class="idx-label">{_esc(label)}</span>'
        f'<span class="idx-value">{_esc(value)}</span></div>'
        for label, value in stat_rows
    )

    # Done table
    done_sorted = sorted(done_files, key=lambda x: x['path'])
    done_body = '\n'.join(
        f'        <tr><td class="mono">{_esc(fi["path"])}</td>'
        f'<td class="num">{all_files_with_counts.get(fi["path"], 0)}</td>'
        f'<td class="num">{_esc(fi.get("last_processed", ""))}</td></tr>'
        for fi in done_sorted
    )

    # Blocked table
    blocked_sorted = sorted(blocked_files, key=lambda x: x['path'])
    if blocked_sorted:
        blocked_body = '\n'.join(
            f'        <tr><td class="mono">{_esc(fi["path"])}</td>'
            f'<td class="num">{all_files_with_counts.get(fi["path"], 0)}</td>'
            f'<td class="num">{_esc(fi.get("last_processed", ""))}</td></tr>'
            for fi in blocked_sorted
        )
    else:
        blocked_body = '        <tr><td class="empty" colspan="3">No files currently blocked.</td></tr>'

    # Pending table (keep queue order, not alphabetical)
    pending_body = '\n'.join(
        f'        <tr><td class="num">{_esc(fi.get("order", "-"))}</td>'
        f'<td class="mono">{_esc(fi["path"])}</td>'
        f'<td class="num">{all_files_with_counts.get(fi["path"], 0)}</td></tr>'
        for fi in pending_files
    )

    # Generated table
    generated_sorted = sorted(generated_files, key=lambda x: x['path'])
    if generated_sorted:
        generated_body = '\n'.join(
            f'        <tr><td class="mono">{_esc(fi["path"])}</td></tr>'
            for fi in generated_sorted
        )
    else:
        generated_body = '        <tr><td class="empty">No generated files.</td></tr>'

    html_doc = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Isearch2 Cleanup Status Report</title>
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
  .subtitle {{
    font-size: 14px;
    color: #4b564d;
    margin: 0 0 20px 0;
    max-width: 34em;
  }}
  code {{
    background: #dce6de;
    color: #2e5940;
    font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace;
    padding: 1px 5px;
    border-radius: 3px;
    font-size: 0.9em;
    white-space: nowrap;
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
    margin: 0 0 10px 0;
  }}
  .section-note {{
    font-size: 12px;
    color: #4b564d;
    font-style: italic;
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
    padding: 5px 10px;
    border-bottom: 1px solid #e3e8e3;
    color: #3a423c;
  }}
  tr {{ break-inside: avoid; }}
  tbody tr:nth-child(even) td {{ background: #f5f8f5; }}
  td.mono {{ font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace; font-size: 11px; }}
  td.num {{ text-align: right; white-space: nowrap; color: #4b564d; font-variant-numeric: tabular-nums; }}
  td.empty {{ color: #7c8a7c; font-style: italic; }}
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
  <h1>Isearch2 Cleanup Status Report</h1>
  <p class="subtitle">A snapshot of processing progress across <code>src/</code>, <code>doctype/</code>, and <code>Isearch-cgi/</code> — done, blocked, and pending files, with per-file function counts.</p>

  <div class="note">
    <strong>Note.</strong> {done_count} of {non_generated_count} in-scope files are done
    ({done_pct:.1f}%); {blocked_count} blocked awaiting a human decision;
    {pending_count} pending. {generated_count} generated file(s) are
    machine-produced and excluded from these counts.
  </div>

  <div class="meta">
    <span class="meta-item"><span class="meta-label">Audience</span><span class="meta-dash">&mdash;</span><span class="meta-value">operators &amp; developers</span></span>
    <span class="meta-item"><span class="meta-label">Format</span><span class="meta-dash">&mdash;</span><span class="meta-value">status report</span></span>
    <span class="meta-item"><span class="meta-label">Files</span><span class="meta-dash">&mdash;</span><span class="meta-value">{total_files}</span></span>
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
      <h2>Processed Files (Done)</h2>
      <p class="section-meta">{done_count} files &middot; {done_functions:,} functions</p>
      <table>
        <thead><tr><th>File</th><th class="num">Functions</th><th class="num">Last Processed</th></tr></thead>
        <tbody>
{done_body if done_body else '        <tr><td class="empty" colspan="3">No done files.</td></tr>'}
        </tbody>
      </table>
    </div>
  </div>

  <div class="section">
    <div class="secnum">&sect;2</div>
    <div class="secbody">
      <h2>Blocked Files</h2>
      <p class="section-meta">{blocked_count} files &middot; {blocked_functions:,} functions</p>
      <p class="section-note">These files require human decisions (typically header signature changes) before processing can continue.</p>
      <table>
        <thead><tr><th>File</th><th class="num">Functions</th><th class="num">Last Processed</th></tr></thead>
        <tbody>
{blocked_body}
        </tbody>
      </table>
    </div>
  </div>

  <div class="section">
    <div class="secnum">&sect;3</div>
    <div class="secbody">
      <h2>Pending Files</h2>
      <p class="section-meta">{pending_count} files &middot; {pending_functions:,} functions</p>
      <p class="section-note">Queued for processing, in order.</p>
      <table>
        <thead><tr><th class="num">Order</th><th>File</th><th class="num">Functions</th></tr></thead>
        <tbody>
{pending_body if pending_body else '        <tr><td class="empty" colspan="3">No pending files.</td></tr>'}
        </tbody>
      </table>
    </div>
  </div>

  <div class="section">
    <div class="secnum">&sect;4</div>
    <div class="secbody">
      <h2>Generated Files</h2>
      <p class="section-meta">{generated_count} files</p>
      <p class="section-note">Machine-generated; never hand-edited or manually processed. Regenerated by their source file during processing.</p>
      <table>
        <thead><tr><th>File</th></tr></thead>
        <tbody>
{generated_body}
        </tbody>
      </table>
    </div>
  </div>

  <div class="footer">Isearch2 Cleanup Status Report &mdash; generated {generated_full} from docs/PROCESSING_STATUS.md. Read-only, disposable output; not committed to the repository.</div>
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
    to LibreOffice, then wkhtmltopdf, then weasyprint.
    """
    chrome = _find_chrome()
    if chrome:
        temp_profile = tempfile.mkdtemp(prefix='isearch2-status-report-chrome-')
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

    temp_profile = tempfile.mkdtemp(prefix='isearch2-status-report-')
    output_dir = os.path.dirname(output_pdf)

    try:
        # Try LibreOffice first
        # Note: LibreOffice creates PDF with same base name as input, so we need to rename
        result = subprocess.run(
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

        # LibreOffice creates PDF with same base name as HTML file, in the output_dir
        html_basename = os.path.basename(html_file)
        pdf_basename = html_basename.replace('.html', '.pdf')
        temp_pdf = os.path.join(output_dir, pdf_basename)

        if os.path.exists(temp_pdf) and os.path.getsize(temp_pdf) > 0:
            try:
                os.rename(temp_pdf, output_pdf)
            except OSError:
                # If rename fails, try copying
                shutil.copy2(temp_pdf, output_pdf)
                os.remove(temp_pdf)

            if os.path.exists(temp_profile):
                shutil.rmtree(temp_profile)
            return 'LibreOffice'
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError) as e:
        pass

    # Cleanup temp profile before fallback
    if os.path.exists(temp_profile):
        shutil.rmtree(temp_profile)

    # Fallback: try wkhtmltopdf if available
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

    # Final fallback: weasyprint (pure Python, if available)
    try:
        from weasyprint import HTML
        HTML(html_file).write_pdf(output_pdf)
        if os.path.exists(output_pdf) and os.path.getsize(output_pdf) > 0:
            return 'weasyprint'
    except ImportError:
        pass

    return None

def cleanup_temp_files(reports_dir, output_pdf_name):
    """Remove temporary PDF files from previous failed conversions."""
    try:
        for filename in os.listdir(reports_dir):
            # Remove temp PDFs that look like they were created by conversion tools
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
        print("Usage: generate_status_report.py <isearch2_root_dir>")
        sys.exit(1)

    root_dir = sys.argv[1]
    status_file = os.path.join(root_dir, 'docs', 'PROCESSING_STATUS.md')

    if not os.path.exists(status_file):
        print(f"Error: {status_file} not found")
        sys.exit(1)

    # Parse status
    files_by_status = parse_processing_status(status_file)

    # Generate HTML
    html_content = generate_html_report(files_by_status, root_dir)

    # Write to temp file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.html', delete=False) as f:
        html_file = f.name
        f.write(html_content)

    # Create output directory
    reports_dir = os.path.join(root_dir, 'docs', 'reports')
    os.makedirs(reports_dir, exist_ok=True)

    # Generate output filename with timestamp
    timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    output_pdf = os.path.join(reports_dir, f'status-report-{timestamp}.pdf')

    # Convert to PDF
    conversion_method = convert_to_pdf(html_file, output_pdf)

    # Cleanup: remove HTML temp file
    try:
        os.remove(html_file)
    except OSError:
        pass

    # Cleanup: remove other temporary PDFs from failed conversions
    cleanup_temp_files(reports_dir, f'status-report-{timestamp}.pdf')

    if conversion_method:
        print(f"✓ Report generated successfully")
        print(f"  Path: {output_pdf}")
        print(f"  Conversion method: {conversion_method}")

        # Print summary statistics
        done_count = len(files_by_status.get('done', []))
        blocked_count = len(files_by_status.get('blocked', []))
        pending_count = len(files_by_status.get('pending', []))
        total = done_count + blocked_count + pending_count

        if total > 0:
            print(f"\n  Summary:")
            print(f"    Done: {done_count}/{total} ({100*done_count/total:.1f}%)")
            print(f"    Blocked: {blocked_count}/{total} ({100*blocked_count/total:.1f}%)")
            print(f"    Pending: {pending_count}/{total} ({100*pending_count/total:.1f}%)")
    else:
        print(f"✗ Failed to generate PDF (LibreOffice and groff both unavailable)")
        if os.path.exists(output_pdf):
            os.remove(output_pdf)
        sys.exit(1)

if __name__ == '__main__':
    main()
