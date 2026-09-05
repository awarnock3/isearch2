#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CGI_DIR="$ROOT_DIR/Isearch-cgi"
DB_PATH="$ROOT_DIR/db"
API_WRAPPER="$CGI_DIR/isearch_api"
API_BINARY="$CGI_DIR/isrch_api"

fail() {
  echo "FAIL: $1" >&2
  exit 1
}

assert_status() {
  expected="$1"
  actual="$2"
  label="$3"
  [ "$actual" = "$expected" ] || fail "$label expected HTTP status $expected, got $actual"
}

parse_status() {
  printf '%s\n' "$1" | sed -n '1s/^Status: \([0-9][0-9][0-9]\).*/\1/p'
}

parse_body() {
  printf '%s\n' "$1" | awk 'seen { print } /^$/{ seen=1 }'
}

run_get() {
  path_info="$1"
  query="$2"
  output=$(ISEARCH_DB_PATH="$DB_PATH" REQUEST_METHOD=GET QUERY_STRING="$query" PATH_INFO="$path_info" "$API_WRAPPER" "$DB_PATH")
  RESPONSE_STATUS=$(parse_status "$output")
  RESPONSE_BODY=$(parse_body "$output")
}

run_post() {
  path_info="$1"
  body="$2"
  output=$(printf '%s' "$body" | ISEARCH_DB_PATH="$DB_PATH" REQUEST_METHOD=POST CONTENT_TYPE="application/json" PATH_INFO="$path_info" "$API_WRAPPER" "$DB_PATH")
  RESPONSE_STATUS=$(parse_status "$output")
  RESPONSE_BODY=$(parse_body "$output")
}

run_get_env() {
  path_info="$1"
  query="$2"
  max_hits="$3"
  allowlist="$4"
  output=$(ISEARCH_DB_PATH="$DB_PATH" ISEARCH_API_MAX_HITS="$max_hits" ISEARCH_API_ALLOWLIST="$allowlist" REQUEST_METHOD=GET QUERY_STRING="$query" PATH_INFO="$path_info" "$API_WRAPPER" "$DB_PATH")
  RESPONSE_STATUS=$(parse_status "$output")
  RESPONSE_BODY=$(parse_body "$output")
}

cd "$CGI_DIR"

[ -x "$API_BINARY" ] || make isrch_api
[ -x "$API_WRAPPER" ] || ./Configure "$DB_PATH"

# 1) GET /v1/api/health -> status == "ok"
run_get "/v1/api/health" ""
assert_status "200" "$RESPONSE_STATUS" "health endpoint"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert d.get("status") == "ok", d
'

# 2) GET /v1/api/capabilities -> valid JSON
run_get "/v1/api/capabilities" ""
assert_status "200" "$RESPONSE_STATUS" "capabilities endpoint"
printf '%s' "$RESPONSE_BODY" | python3 -c 'import json, sys; json.load(sys.stdin)'

# 2b) GET /v1/api/databases -> each entry has "name", XMLtest also has "doctype"
run_get "/v1/api/databases" ""
assert_status "200" "$RESPONSE_STATUS" "databases endpoint"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
dbs = d["databases"]
assert isinstance(dbs, list) and len(dbs) > 0, d
names = []
for entry in dbs:
    assert isinstance(entry, dict), entry
    assert "name" in entry and isinstance(entry["name"], str) and entry["name"], entry
    names.append(entry["name"])
    if "doctype" in entry:
        assert isinstance(entry["doctype"], str) and entry["doctype"], entry
assert "XMLtest" in names, names
'

# 3) GET /v1/api/XMLtest/search?q=dust -> matching_record_count > 0
run_get "/v1/api/XMLtest/search" "q=dust"
assert_status "200" "$RESPONSE_STATUS" "GET /search dust"
GET_DUST_COUNT=$(printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
print(int(d["matching_record_count"]))
')
[ "$GET_DUST_COUNT" -gt 0 ] || fail "GET /search dust expected matching_record_count > 0"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert d.get("request_id"), d
assert "links" in d and set(d["links"]).issuperset({"next", "prev"}), d
'

# 4) POST /v1/api/XMLtest/search {"q":"dust","request_id":"smoke-post"} -> same count as GET
run_post "/v1/api/XMLtest/search" '{"q":"dust","request_id":"smoke-post","include_url":false,"score_scale":10}'
assert_status "200" "$RESPONSE_STATUS" "POST /search dust"
POST_DUST_COUNT=$(printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
print(int(d["matching_record_count"]))
')
[ "$POST_DUST_COUNT" -eq "$GET_DUST_COUNT" ] || fail "POST/GET dust count mismatch ($POST_DUST_COUNT != $GET_DUST_COUNT)"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert d.get("request_id") == "smoke-post", d
assert int(d["max_hits"]) == 50, d
'

# 5) GET /v1/api/XMLtest/search?q=xml&start=2&max_hits=1 -> results length 1, start 2
run_get "/v1/api/XMLtest/search" "q=xml&start=2&max_hits=1"
assert_status "200" "$RESPONSE_STATUS" "GET /search pagination"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert int(d["start"]) == 2, d
assert len(d["results"]) == 1, d
assert d["links"]["prev"], d
'

# 6) GET /v1/api/search (missing database path segment) -> 404 endpoint not found
run_get "/v1/api/search" "q=dust"
assert_status "404" "$RESPONSE_STATUS" "GET /search missing database path segment"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert isinstance(d.get("type"), str) and d["type"], d
assert isinstance(d.get("title"), str) and d["title"], d
'

# 7) GET /v1/api/no_such_db/search?q=dust -> 404 problem object
run_get "/v1/api/no_such_db/search" "q=dust"
assert_status "404" "$RESPONSE_STATUS" "GET /search missing database root"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert isinstance(d.get("type"), str) and d["type"], d
assert isinstance(d.get("title"), str) and d["title"], d
'

# 8) GET /v1/api/XMLtest/search with invalid start -> 400 problem
run_get "/v1/api/XMLtest/search" "q=dust&start=0"
assert_status "400" "$RESPONSE_STATUS" "GET /search invalid start"

# 9) GET/POST parity with option flags + byte range fields
run_get "/v1/api/XMLtest/search" "q=xml&search_type=advanced&operator=or&rpn=false&infix=true&and_mode=false&synonyms=false&element_set=B&record_syntax=HTML&byte_range=true&start=1&max_hits=1&score_scale=100&include_url=false&include_record_key=false"
assert_status "200" "$RESPONSE_STATUS" "GET /search options parity"
GET_PARITY_COUNT=$(printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert len(d["results"]) == 1, d
h = d["results"][0]
assert "record_start" in h and "record_end" in h, h
assert "headline" in h, h
print(int(d["matching_record_count"]))
')

run_post "/v1/api/XMLtest/search" '{"q":"xml","search_type":"advanced","operator":"or","rpn":false,"infix":true,"and_mode":false,"synonyms":false,"element_set":"B","record_syntax":"HTML","byte_range":true,"start":1,"max_hits":1,"score_scale":100,"include_url":false,"include_record_key":false}'
assert_status "200" "$RESPONSE_STATUS" "POST /search options parity"
POST_PARITY_COUNT=$(printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert len(d["results"]) == 1, d
h = d["results"][0]
assert "record_start" in h and "record_end" in h, h
assert "headline" in h, h
print(int(d["matching_record_count"]))
')
[ "$POST_PARITY_COUNT" -eq "$GET_PARITY_COUNT" ] || fail "GET/POST parity count mismatch ($GET_PARITY_COUNT != $POST_PARITY_COUNT)"

# 10) Ceiling enforcement from ISEARCH_API_MAX_HITS
run_get_env "/v1/api/XMLtest/search" "q=dust&max_hits=3" "2" ""
assert_status "400" "$RESPONSE_STATUS" "GET /search max_hits ceiling"

# 11) Allowlist enforcement from ISEARCH_API_ALLOWLIST
run_get_env "/v1/api/XMLtest/search" "q=dust" "50" "OtherDb"
assert_status "404" "$RESPONSE_STATUS" "GET /search allowlist denied"

# 12) start_doc/end_doc range honored with pagination metadata
run_get "/v1/api/XMLtest/search" "q=xml&start_doc=2&end_doc=2&max_hits=5"
assert_status "200" "$RESPONSE_STATUS" "GET /search start_doc end_doc"
printf '%s' "$RESPONSE_BODY" | python3 -c '
import json, sys
d = json.load(sys.stdin)
assert len(d["results"]) <= 1, d
if d["results"]:
    assert int(d["results"][0]["match_number"]) == 2, d
'

echo "PASS: API smoke tests completed successfully."
