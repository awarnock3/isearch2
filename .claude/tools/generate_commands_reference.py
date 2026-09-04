#!/usr/bin/env python3
"""
Generate a PDF reference guide for all Isearch2 cleanup commands.

Styled after the project's "reference manual" template (see
TUTORIAL-reference-manual.pdf): serif headings, sage-green note/code
boxes, and numbered sections in a left gutter.
"""

import os
import re
import sys
import html
import shutil
import tempfile
import subprocess
from datetime import datetime


def extract_commands(claude_md_path):
    """Extract command descriptions from CLAUDE.md."""
    commands = []

    with open(claude_md_path, 'r') as f:
        content = f.read()

    # Find the COMMANDS section
    commands_start = content.find('## COMMANDS')
    if commands_start == -1:
        return []

    # Find the end of COMMANDS section (next major section or end of file)
    end_marker = content.find('\nWhen you\'re ready', commands_start)
    if end_marker == -1:
        end_marker = len(content)

    commands_section = content[commands_start:end_marker]

    # Split by command bullets (- **COMMANDNAME**)
    # Pattern: - **anything** — description
    pattern = r'- \*\*([^*]+)\*\*\s+—\s+(.+?)(?=\n- \*\*|$)'
    matches = re.findall(pattern, commands_section, re.DOTALL)

    for cmd_name, description in matches:
        # Clean up the description
        description = description.strip()
        # Get first 2-3 sentences as summary
        sentences = description.split('. ')[:2]
        summary = '. '.join(sentences)
        if not summary.endswith('.'):
            summary += '.'

        # Extract usage from command name
        # Handle special cases with arguments like "PROCESS `<filename>`"
        if '`<' in cmd_name:
            # Extract the base command and argument
            base_match = re.match(r'([A-Z\s]+)\s*`?<([^>]+)>`?', cmd_name)
            if base_match:
                base_cmd = base_match.group(1).strip().lower().replace(' ', '-')
                arg = base_match.group(2)
                usage = f"/{base_cmd} <{arg}>"
            else:
                usage = f"/{cmd_name.lower().replace(' ', '-')}"
        else:
            usage = f"/{cmd_name.lower().replace(' ', '-')}"

        commands.append({
            'name': cmd_name,
            'usage': usage,
            'description': description,
            'summary': summary
        })

    # Ascending alphabetic order by command name (as it appears in the
    # usage string, e.g. "/analyze"), not CLAUDE.md's document order.
    commands.sort(key=lambda cmd: cmd['usage'].lstrip('/').lower())

    return commands


def _inline_markup(text):
    """Escape HTML, then turn `code` spans and **bold** spans into tags."""
    escaped = html.escape(text, quote=False)
    escaped = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', escaped)
    escaped = re.sub(r'`([^`]+)`', r'<code>\1</code>', escaped)
    # Collapse the newlines/indentation left over from the source Markdown.
    # A trailing hyphen or slash means the source hard-wrapped mid-token
    # (e.g. a long path inside a code span) — rejoin those with no space;
    # everything else is an ordinary prose wrap and gets a single space.
    escaped = re.sub(r'[ \t]*\n[ \t]*', '\n', escaped)
    escaped = re.sub(r'([-/])\n', r'\1', escaped)
    escaped = escaped.replace('\n', ' ')
    escaped = re.sub(r' {2,}', ' ', escaped)
    return escaped.strip()


def generate_html_reference(commands, claude_md_path):
    """Generate HTML reference guide in the reference-manual style."""
    now = datetime.now()
    generated_date = now.strftime('%Y-%m-%d')
    generated_full = now.strftime('%Y-%m-%d %H:%M:%S')

    # Quick-index rows (usage + one-line summary)
    index_rows = []
    for cmd in commands:
        summary = cmd['summary']
        if len(summary) > 92:
            summary = summary[:92].rsplit(' ', 1)[0] + '…'
        index_rows.append(f"""      <div class="idx-row">
        <span class="idx-usage">{html.escape(cmd['usage'])}</span>
        <span class="idx-desc">{html.escape(summary)}</span>
      </div>""")

    # Detailed per-command sections
    sections = []
    for i, cmd in enumerate(commands, start=1):
        name_html = _inline_markup(cmd['name'])
        usage_html = html.escape(cmd['usage'])
        description_html = _inline_markup(cmd['description'])
        sections.append(f"""    <div class="section">
      <div class="secnum">§{i}</div>
      <div class="secbody">
        <h2>{name_html} <span class="usage-badge"><code>{usage_html}</code></span></h2>
        <p>{description_html}</p>
      </div>
    </div>""")

    html_doc = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Operator Commands Reference for Isearch2 Cleanup</title>
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
    margin: 0 0 8px 0;
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
    gap: 18px;
    padding: 3px 0;
    font-family: 'DejaVu Sans Mono', 'Liberation Mono', Consolas, Menlo, monospace;
    font-size: 11.5px;
    break-inside: avoid;
  }}
  .idx-usage {{
    flex: 0 0 170px;
    color: #2e5940;
    font-weight: 700;
  }}
  .idx-desc {{
    color: #4b564d;
    font-family: -apple-system, BlinkMacSystemFont, 'Noto Sans', 'Liberation Sans', Arial, sans-serif;
  }}
  .section {{
    display: flex;
    gap: 22px;
    margin: 0 0 28px 0;
    break-inside: avoid;
  }}
  .secnum {{
    flex: 0 0 30px;
    color: #527560;
    font-size: 12px;
    padding-top: 3px;
  }}
  .secbody {{ flex: 1; }}
  .secbody p {{ margin: 0; color: #3a423c; }}
  .usage-badge {{ font-weight: 400; margin-left: 4px; }}
  .index-heading {{
    font-family: Georgia, 'Noto Serif', 'Liberation Serif', serif;
    font-size: 13px;
    font-weight: 700;
    color: #527560;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    margin: 0 0 10px 0;
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
  <h1>Operator Commands Reference for Isearch2 Cleanup</h1>
  <p class="subtitle">Invocation syntax and behavior for every slash command in the Isearch2 cleanup workflow, from <code>/analyze</code> through <code>/commands-reference</code>.</p>

  <div class="note">
    <strong>Note.</strong> This guide is generated directly from the COMMANDS
    section of <code>CLAUDE.md</code>, so it always reflects the current
    workflow. Commands are listed in ascending alphabetic order below,
    for quick lookup rather than workflow sequence.
  </div>

  <div class="meta">
    <span class="meta-item"><span class="meta-label">Audience</span><span class="meta-dash">&mdash;</span><span class="meta-value">operators &amp; developers</span></span>
    <span class="meta-item"><span class="meta-label">Format</span><span class="meta-dash">&mdash;</span><span class="meta-value">command reference</span></span>
    <span class="meta-item"><span class="meta-label">Commands</span><span class="meta-dash">&mdash;</span><span class="meta-value">{len(commands)}</span></span>
    <span class="meta-item"><span class="meta-label">Generated</span><span class="meta-dash">&mdash;</span><span class="meta-value">{generated_date}</span></span>
  </div>
  <hr>

  <p class="index-heading">Quick Index</p>
  <div class="codebox">
{chr(10).join(index_rows)}
  </div>

  {chr(10).join(sections)}

  <div class="footer">Operator Commands Reference for Isearch2 Cleanup &mdash; generated {generated_full} from CLAUDE.md. Read-only, disposable output; not committed to the repository.</div>
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
    support, so the sage boxes and gutter layout render faithfully). Falls
    back to LibreOffice, then wkhtmltopdf, for environments without Chrome.
    """
    chrome = _find_chrome()
    if chrome:
        temp_profile = tempfile.mkdtemp(prefix='isearch2-commands-ref-chrome-')
        try:
            subprocess.run(
                [
                    chrome, '--headless=new', '--disable-gpu', '--no-sandbox',
                    f'--user-data-dir={temp_profile}',
                    f'--print-to-pdf={output_pdf}',
                    '--no-pdf-header-footer',
                    '--virtual-time-budget=5000',
                    f'file://{os.path.abspath(html_file)}',
                ],
                capture_output=True,
                timeout=60,
                text=True,
            )
            if os.path.exists(output_pdf) and os.path.getsize(output_pdf) > 0:
                return 'Chrome'
        except (subprocess.TimeoutExpired, OSError):
            pass
        finally:
            shutil.rmtree(temp_profile, ignore_errors=True)

    # Fallback: LibreOffice
    temp_profile = tempfile.mkdtemp(prefix='isearch2-commands-ref-')
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

    # Final fallback: wkhtmltopdf
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
        print("Usage: generate_commands_reference.py <isearch2_root_dir>")
        sys.exit(1)

    root_dir = sys.argv[1]
    claude_md = os.path.join(root_dir, 'CLAUDE.md')

    if not os.path.exists(claude_md):
        print(f"Error: {claude_md} not found")
        sys.exit(1)

    # Extract commands
    commands = extract_commands(claude_md)

    if not commands:
        print("Error: No commands found in CLAUDE.md")
        sys.exit(1)

    # Generate HTML
    html_content = generate_html_reference(commands, claude_md)

    # Write to temp file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.html', delete=False) as f:
        html_file = f.name
        f.write(html_content)

    # Create output directory
    reports_dir = os.path.join(root_dir, 'docs', 'reports')
    os.makedirs(reports_dir, exist_ok=True)

    # Generate output filename with timestamp
    timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    output_pdf = os.path.join(reports_dir, f'commands-reference-{timestamp}.pdf')

    # Convert to PDF
    conversion_method = convert_to_pdf(html_file, output_pdf)

    # Cleanup temp files
    try:
        os.remove(html_file)
    except OSError:
        pass

    cleanup_temp_files(reports_dir)

    if conversion_method:
        print(f"✓ Commands reference generated successfully")
        print(f"  Path: {output_pdf}")
        print(f"  Conversion method: {conversion_method}")
        print(f"  Total commands: {len(commands)}")
    else:
        print(f"✗ Failed to generate PDF (Chrome, LibreOffice, and wkhtmltopdf all unavailable)")
        if os.path.exists(output_pdf):
            os.remove(output_pdf)
        sys.exit(1)


if __name__ == '__main__':
    main()
