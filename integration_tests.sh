#!/bin/bash
#
# integration_tests.sh
#
# Integration test suite for the Isearch2 toolset.
#
# Exercises the four CLI binaries (Iindex, Isearch, Iget, Iutil) against
# freshly-built indexes of the SGML, HTML, and Markdown documents under
# testdocs/, and exercises the HTTP JSON API (Isearch-cgi/isrch_api) with
# curl by running it as a CGI script under Python's built-in CGI HTTP
# server.
#
# Usage:
#   ./integration_tests.sh
#
# Requires: bin/Iindex, bin/Isearch, bin/Iget, bin/Iutil, and
# Isearch-cgi/isrch_api to already be built (run `./configure && make`,
# then `cd Isearch-cgi && make` first if they are missing).
# Requires python3 (for the disposable CGI HTTP server) and curl.

set -u

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BIN_DIR="$ROOT_DIR/bin"
CGI_DIR="$ROOT_DIR/Isearch-cgi"
TESTDOCS_DIR="$ROOT_DIR/testdocs"

IINDEX="$BIN_DIR/Iindex"
ISEARCH="$BIN_DIR/Isearch"
IGET="$BIN_DIR/Iget"
IUTIL="$BIN_DIR/Iutil"
ISRCH_API="$CGI_DIR/isrch_api"

WORK_DIR=$(mktemp -d /tmp/isearch_integration.XXXXXX)
CGI_ROOT="$WORK_DIR/cgi-root"
CGI_BIN="$CGI_ROOT/cgi-bin"
DB_DIR="$WORK_DIR/db"
API_PORT=8123
API_PID=""

PASS_COUNT=0
FAIL_COUNT=0
CURL_TIMEOUT=${CURL_TIMEOUT:-10}

curl_api() {
  curl --connect-timeout 5 --max-time "$CURL_TIMEOUT" "$@"
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

pass() {
  PASS_COUNT=$((PASS_COUNT + 1))
  echo "  PASS: $1"
}

fail() {
  FAIL_COUNT=$((FAIL_COUNT + 1))
  echo "  FAIL: $1"
}

section() {
  echo
  echo "=== $1 ==="
}

# assert_contains OUTPUT NEEDLE LABEL
# Passes if NEEDLE is found somewhere in OUTPUT.
assert_contains() {
  local output="$1" needle="$2" label="$3"
  if printf '%s' "$output" | grep -qF -- "$needle"; then
    pass "$label"
  else
    fail "$label (expected to find: $needle)"
  fi
}

# assert_exit_zero STATUS LABEL
assert_exit_zero() {
  local status="$1" label="$2"
  if [ "$status" -eq 0 ]; then
    pass "$label"
  else
    fail "$label (exit status $status)"
  fi
}

cleanup() {
  if [ -n "$API_PID" ] && kill -0 "$API_PID" 2>/dev/null; then
    kill "$API_PID" 2>/dev/null
    wait "$API_PID" 2>/dev/null
  fi
  rm -rf "$WORK_DIR"
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------

section "Pre-flight checks"

for exe in "$IINDEX" "$ISEARCH" "$IGET" "$IUTIL" "$ISRCH_API"; do
  if [ -x "$exe" ]; then
    pass "$exe is present and executable"
  else
    fail "$exe is missing or not executable"
  fi
done

if ! command -v curl >/dev/null 2>&1; then
  echo "curl is required for API tests but was not found; aborting." >&2
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is required to host the CGI API for curl tests; aborting." >&2
  exit 1
fi

mkdir -p "$DB_DIR" "$CGI_BIN"

# ---------------------------------------------------------------------------
# Iindex: build one database per doctype from testdocs/
# ---------------------------------------------------------------------------

section "Iindex: indexing testdocs/ by doctype"

# HTML doctype
HTML_DB="$DB_DIR/HTMLtest"
out=$("$IINDEX" -d "$HTML_DB" -t HTML "$TESTDOCS_DIR"/html/*.html 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -d -t HTML (build HTML index)"
assert_contains "$out" "Database files saved to disk." "Iindex HTML: reports success"

# Markdown doctype
MD_DB="$DB_DIR/MDtest"
out=$("$IINDEX" -d "$MD_DB" -t MARKDOWN "$TESTDOCS_DIR"/Markdown/*.md 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -d -t MARKDOWN (build Markdown index)"
assert_contains "$out" "Database files saved to disk." "Iindex Markdown: reports success"

# SGML doctype (SGMLTAG parses generic SGML/XML-ish tagged text)
SGML_DB="$DB_DIR/SGMLtest"
out=$("$IINDEX" -d "$SGML_DB" -t SGMLTAG "$TESTDOCS_DIR"/sgml/*.sgml 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -d -t SGMLTAG (build SGML index)"
assert_contains "$out" "Database files saved to disk." "Iindex SGML: reports success"

# -f: index from a file list instead of argv
FILELIST="$WORK_DIR/html_filelist.txt"
printf '%s\n' "$TESTDOCS_DIR"/html/*.html > "$FILELIST"
FROMFILE_DB="$DB_DIR/HTMLfromfile"
out=$("$IINDEX" -d "$FROMFILE_DB" -t HTML -f "$FILELIST" 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -f (index from file list)"
assert_contains "$out" "Database files saved to disk." "Iindex -f: reports success"

# -m: megabytes-per-load-batch tuning (should not change results, just exercised)
MTUNE_DB="$DB_DIR/HTMLmtune"
out=$("$IINDEX" -d "$MTUNE_DB" -t HTML -m 4 "$TESTDOCS_DIR"/html/*.html 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -m (load batch size in MB)"

# -r: recursive descent, pointed at the whole testdocs tree (mixed doctypes
# will be parsed as HTML here; this only checks that recursion + indexing
# does not crash, not the semantic correctness of mixed-content parsing).
RECURSE_DB="$DB_DIR/Recursetest"
out=$("$IINDEX" -d "$RECURSE_DB" -t HTML -r "$TESTDOCS_DIR/html" 2>&1)
status=$?
assert_exit_zero "$status" "Iindex -r (recursive descent)"

# ---------------------------------------------------------------------------
# Isearch: run searches, formats, boolean modes, and pagination flags
# ---------------------------------------------------------------------------

section "Isearch: querying the HTML index"

out=$("$ISEARCH" -d "$HTML_DB" -t -q tutorial 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Isearch: simple term search finds expected doc"

out=$("$ISEARCH" -d "$HTML_DB" -p B -f TEXT -t -q tutorial 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Isearch -p B -f TEXT (brief element set, TEXT syntax)"

out=$("$ISEARCH" -d "$HTML_DB" -f HTML -t -q tutorial 2>&1)
assert_exit_zero "$?" "Isearch -f HTML (HTML record syntax)"

out=$("$ISEARCH" -d "$HTML_DB" -json -q tutorial 2>&1)
assert_contains "$out" '"database"' "Isearch -json (JSON output)"

out=$("$ISEARCH" -d "$HTML_DB" -rpn -t -q tutorial quickstart or 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Isearch -rpn (reverse-Polish boolean query)"

out=$("$ISEARCH" -d "$HTML_DB" -infix -t -q 'tutorial or guide' 2>&1)
assert_contains "$out" "QuickStart.html" "Isearch -infix (infix boolean query)"

out=$("$ISEARCH" -d "$HTML_DB" -and -t -q isearch tutorial 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Isearch -and (AND combination of terms)"

out=$("$ISEARCH" -d "$HTML_DB" -syn "$WORK_DIR/nonexistent_syn.txt" -t -q tutorial 2>&1)
# Synonym file is optional/absent here; just confirm the flag is accepted
# without the process crashing outright.
assert_contains "$out" "IsearchTutorial" "Isearch -syn (synonym file flag accepted)"

out=$("$ISEARCH" -d "$HTML_DB" -prefix '>>>' -suffix '<<<' -t -q tutorial 2>&1)
assert_exit_zero "$?" "Isearch -prefix/-suffix (term highlighting markers)"

out=$("$ISEARCH" -d "$HTML_DB" -byterange -t -q tutorial 2>&1)
assert_contains "$out" "[ 0 -" "Isearch -byterange (byte offsets reported)"

out=$("$ISEARCH" -d "$HTML_DB" -startdoc 1 -enddoc 1 -t -q tutorial 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Isearch -startdoc/-enddoc (result window)"

# Isearch -V is supported and should print a banner and exit 0.
# We assert the actual runtime contract here rather than pinning a stale
# historical quirk that has since been corrected in the CLI.
out=$("$ISEARCH" -d "$HTML_DB" -V 2>&1)
status=$?
assert_exit_zero "$status" "Isearch -V (prints version banner and exits 0)"
assert_contains "$out" "Isearch v2.00" "Isearch -V output includes the version banner"

section "Isearch: querying the Markdown and SGML indexes"

out=$("$ISEARCH" -d "$MD_DB" -t -q tutorial 2>&1)
assert_contains "$out" "Tutorial.md" "Isearch: Markdown doctype search finds expected doc"

out=$("$ISEARCH" -d "$SGML_DB" -t -q isearch 2>&1)
assert_contains "$out" "Isearch.sgml" "Isearch: SGML doctype search finds expected doc"

# ---------------------------------------------------------------------------
# Iget: fetch full records and specific element sets
# ---------------------------------------------------------------------------

section "Iget: fetching records"

out=$("$ISEARCH" -d "$HTML_DB" -t -q tutorial 2>&1)
DOC_KEY=$(printf '%s\n' "$out" | awk -F'|' '{gsub(/ /,"",$3); print $3; exit}')

if [ -n "$DOC_KEY" ]; then
  out=$("$IGET" -d "$HTML_DB" -id "$DOC_KEY" -p F -f TEXT 2>&1)
  assert_contains "$out" "ISEARCH TUTORIAL" "Iget -id -p F -f TEXT (full-record fetch)"

  out=$("$IGET" -d "$HTML_DB" -id "$DOC_KEY" -p B -f TEXT 2>&1)
  assert_exit_zero "$?" "Iget -id -p B (brief element set fetch)"
else
  fail "Iget: could not determine a document key from Isearch output to fetch"
fi

# ---------------------------------------------------------------------------
# Iutil: database introspection and maintenance
# ---------------------------------------------------------------------------

section "Iutil: database introspection"

out=$("$IUTIL" -d "$HTML_DB" -vi 2>&1)
assert_contains "$out" "Total number of documents: 4" "Iutil -vi (summary info)"

out=$("$IUTIL" -d "$HTML_DB" -vf 2>&1)
assert_contains "$out" "TITLE" "Iutil -vf (field list)"

out=$("$IUTIL" -d "$HTML_DB" -v 2>&1)
assert_contains "$out" "IsearchTutorial.html" "Iutil -v (document list)"

out=$("$IUTIL" -d "$HTML_DB" -state 2>&1)
assert_contains "$out" "ready" "Iutil -state (database state)"

out=$("$IUTIL" -d "$HTML_DB" -urn 2>&1)
assert_exit_zero "$?" "Iutil -urn (generate URN table)"

if [ -n "$DOC_KEY" ]; then
  out=$(echo "$DOC_KEY" | "$IUTIL" -d "$HTML_DB" -del 2>&1)
  assert_exit_zero "$?" "Iutil -del (mark document deleted)"

  out=$(echo "$DOC_KEY" | "$IUTIL" -d "$HTML_DB" -undel 2>&1)
  assert_exit_zero "$?" "Iutil -undel (unmark document deleted)"
fi

out=$("$IUTIL" -d "$HTML_DB" -c 2>&1)
assert_exit_zero "$?" "Iutil -c (cleanup database)"

out=$("$IUTIL" -d "$HTML_DB" -optimize 2>&1)
assert_exit_zero "$?" "Iutil -optimize (optimize indexes)"

out=$("$IUTIL" -d "$HTML_DB" -V 2>&1)
assert_contains "$out" "Iutil v2.00" "Iutil -V (version string)"

# Non-destructive -gt / -gt0 round trip against a disposable copy of the DB,
# so we don't perturb the doctype used by the rest of the suite.
GT_DB="$DB_DIR/HTMLgttest"
"$IINDEX" -d "$GT_DB" -t HTML "$TESTDOCS_DIR"/html/QuickStart.html >/dev/null 2>&1
out=$("$IUTIL" -d "$GT_DB" -gt HTML 2>&1)
assert_exit_zero "$?" "Iutil -gt (set global document type)"
out=$("$IUTIL" -d "$GT_DB" -gt0 2>&1)
assert_exit_zero "$?" "Iutil -gt0 (clear global document type)"

# ---------------------------------------------------------------------------
# HTTP API: start isrch_api under Python's CGI HTTP server and drive it
# with curl.
# ---------------------------------------------------------------------------

section "HTTP API: starting CGI test server"

cp "$ISRCH_API" "$CGI_BIN/isrch_api"
cat > "$CGI_BIN/isearch_api" <<EOF
#!/bin/sh
export ISEARCH_DB_PATH="$DB_DIR"
export ISEARCH_API_MAX_HITS=200
export ISEARCH_API_ALLOWLIST=HTMLtest,MDtest,SGMLtest
exec "$CGI_BIN/isrch_api" "$DB_DIR"
EOF
chmod +x "$CGI_BIN/isrch_api" "$CGI_BIN/isearch_api"

( cd "$CGI_ROOT" && exec python3 -m http.server --cgi "$API_PORT" ) \
  > "$WORK_DIR/api_server.log" 2>&1 &
API_PID=$!

API_BASE="http://127.0.0.1:$API_PORT/cgi-bin/isearch_api"

# Wait for the server to come up.
ready=0
for _ in 1 2 3 4 5 6 7 8 9 10; do
  if curl -sS -o /dev/null "$API_BASE/api/v1/health" 2>/dev/null; then
    ready=1
    break
  fi
  sleep 0.5
done

if [ "$ready" -eq 1 ]; then
  pass "CGI API server started on port $API_PORT"
else
  fail "CGI API server did not start in time"
fi

section "HTTP API: curl-driven endpoint tests"

out=$(curl_api -sS "$API_BASE/api/v1/health")
assert_contains "$out" '"status":"ok"' "GET /api/v1/health"

out=$(curl_api -sS "$API_BASE/api/v1/capabilities")
assert_contains "$out" '"api_version"' "GET /api/v1/capabilities"

out=$(curl_api -sS "$API_BASE/api/v1/databases")
assert_contains "$out" "HTMLtest" "GET /api/v1/databases"

out=$(curl_api -sS "$API_BASE/api/v1/search?database=HTMLtest&q=tutorial")
assert_contains "$out" "IsearchTutorial.html" "GET /api/v1/search (simple query)"

out=$(curl_api -sS "$API_BASE/api/v1/search?database=HTMLtest&q=tutorial&record_syntax=HTML")
assert_contains "$out" '"database":"HTMLtest"' "GET /api/v1/search (record_syntax=HTML)"

out=$(curl_api -sS "$API_BASE/api/v1/search?database=HTMLtest&q=tutorial&byte_range=true&max_hits=1")
assert_contains "$out" "record_start" "GET /api/v1/search (byte_range=true)"

out=$(curl_api -sS "$API_BASE/api/v1/search?database=HTMLtest&q=tutorial&start=1&max_hits=1")
assert_contains "$out" '"max_hits":1' "GET /api/v1/search (start/max_hits pagination)"

# Some CGI implementations accept GET requests but never return a response to
# POST bodies in a timely fashion; bound the request here so the suite does not
# block forever and the script still reports the endpoint status.
POST_JSON="{\"database\":\"HTMLtest\",\"q\":\"tutorial\"}"
out=$(curl_api -sS --max-time "$CURL_TIMEOUT" -X POST "$API_BASE/api/v1/search" \
  -H 'Content-Type: application/json' \
  -d "$POST_JSON" 2>&1)
if printf '%s' "$out" | grep -q 'IsearchTutorial.html'; then
  pass "POST /api/v1/search (JSON body)"
elif printf '%s' "$out" | grep -Eq 'timed out|Operation timed out|curl: \(28\)'; then
  fail "POST /api/v1/search (JSON body) timed out after ${CURL_TIMEOUT}s"
else
  fail "POST /api/v1/search (JSON body) did not return the expected record: $out"
fi

POST_MULTI="{\"database\":\"HTMLtest\",\"terms\":[\"tutorial\",\"guide\"],\"operator\":\"or\"}"
out=$(curl_api -sS --max-time "$CURL_TIMEOUT" -X POST "$API_BASE/api/v1/search" \
  -H 'Content-Type: application/json' \
  -d "$POST_MULTI" 2>&1)
if printf '%s' "$out" | grep -q 'IsearchTutorial.html'; then
  pass "POST /api/v1/search (terms[] + operator=or)"
elif printf '%s' "$out" | grep -Eq 'timed out|Operation timed out|curl: \(28\)'; then
  fail "POST /api/v1/search (terms[] + operator=or) timed out after ${CURL_TIMEOUT}s"
else
  fail "POST /api/v1/search (terms[] + operator=or) did not return an expected result: $out"
fi

# Malformed content-type should be rejected.
status=$(curl_api -sS -o /dev/null -w '%{http_code}' -X POST "$API_BASE/api/v1/search" \
  -H 'Content-Type: text/plain' \
  -d '{"database":"HTMLtest","q":"tutorial"}')
if [ "$status" = "415" ]; then
  pass "POST /api/v1/search rejects non-JSON Content-Type (415)"
else
  fail "POST /api/v1/search should return 415 for bad Content-Type, got $status"
fi

# Unknown database should 404.
status=$(curl_api -sS -o /dev/null -w '%{http_code}' "$API_BASE/api/v1/search?database=NoSuchDB&q=x")
if [ "$status" = "404" ]; then
  pass "GET /api/v1/search returns 404 for unknown database"
else
  fail "GET /api/v1/search should return 404 for unknown database, got $status"
fi

out=$(curl_api -sS "$API_BASE/api/v1/HTMLtest/fetch?id=10")
assert_contains "$out" '"status"' "GET /api/v1/{database}/fetch returns a JSON response"

out=$(curl_api -sS -X POST "$API_BASE/api/v1/HTMLtest/fetch" \
  -H 'Content-Type: application/json' \
  -d '{"id":"10"}' 2>&1)
if printf '%s' "$out" | grep -Eq 'timed out|Operation timed out|curl: \(28\)'; then
  fail "POST /api/v1/{database}/fetch timed out after ${CURL_TIMEOUT}s"
else
  pass "POST /api/v1/{database}/fetch (bounded request does not hang)"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

section "Summary"
echo "Passed: $PASS_COUNT"
echo "Failed: $FAIL_COUNT"

# ---------------------------------------------------------------------------
# NOT covered by this script (documented for future extension):
#
# Iindex:
#   -meta (X)     # Custom XML metadata file for indexing (default meta.xml
#                  # not exercised against a real GILS/metadata scenario).
#   -gils         # GILS metadata generation during indexing.
#   -merge        # Merging of subindexes (requires multi-batch/-m-triggered
#                  # subindex state to exercise meaningfully).
#   -o (X)        # Document-type-specific options (doctype dependent; no
#                  # single value is universally valid across HTML/Markdown/
#                  # SGMLTAG).
#
# Isearch:
#   -RECT{North South West East}  # Geospatial rectangle search; none of the
#                  # testdocs/ documents carry geographic metadata.
#   -o (X)        # Document-type-specific search options (same caveat as
#                  # Iindex -o above).
#   -f with USMARC/SUTRS/XML/GRS-1 and the raw OID syntax strings (only
#                  # TEXT, HTML, and the default were exercised above).
#   -p with a custom comma-separated field list or doctype-specific element
#                  # sets other than B/F (e.g. A/S/C/R) were not exercised.
#
# Iget:
#   -f with formats other than TEXT (USMARC/SUTRS/HTML/SGML/XML/GRS-1).
#
# Iutil:
#   -newpaths     # Interactive prompt for new file pathnames; not scriptable
#                  # without a pseudo-tty/expect-style harness.
#   -erase        # Destructive; would require rebuilding indexes afterward
#                  # for the remainder of the suite to run, so intentionally
#                  # left untested here beyond code inspection.
#   -replace (X)  # Requires two independent, compatible databases staged
#                  # ahead of time; out of scope for this smoke-style suite.
#   -gilsdocs / -gilsindex / -meta (X)  # GILS metadata export; none of the
#                  # testdocs/ content is modeled with GILS elements.
#   -m (X) on -optimize  # Load-batch tuning was only exercised for Iindex,
#                  # not for the -optimize codepath.
#   -o (X)        # Document-type-specific options (same caveat as above).
#
# HTTP API:
#   Apache/nginx/fcgiwrap deployment paths themselves (README-API.md) are
#   not exercised; this script drives isrch_api directly as a CGI script
#   under Python's http.server for a fast, dependency-light smoke test.
#   ISEARCH_API_ALLOWLIST rejection behavior (querying a database excluded
#   from the allowlist) is not separately tested.
# ---------------------------------------------------------------------------

if [ "$FAIL_COUNT" -eq 0 ]; then
  exit 0
else
  exit 1
fi
