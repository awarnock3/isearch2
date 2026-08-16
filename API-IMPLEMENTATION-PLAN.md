# API Implementation Plan

This document breaks the work described in `SEARCH-API-DESIGN.md` into discrete,
independently implementable phases.  Each phase produces a testable increment and
has minimal coupling to phases that follow it.  Work through them in order; later
phases may optionally depend on earlier ones as noted.

---

## Phase 1 — Author the OpenAPI Contract

**Goal:** Produce the canonical API contract that all later code is validated against.

### Steps

1. Create directory structure:
   ```
   api/
     openapi/
       examples/
   ```
2. Write `api/openapi/search-api.v1.yaml` covering:
   - `openapi: 3.1.0`, `info`, `servers`, `tags`
   - All five paths: `/v1/api/search` (GET + POST), `/v1/api/health`,
     `/v1/api/capabilities`, `/v1/api/databases`
   - Every parameter from the design doc with `in`, `name`, `required`,
     `schema`, defaults, and enum/range constraints
   - `components/schemas` for `SearchResponse`, `SearchHit`,
     `CapabilitiesResponse`, `HealthResponse`, `DatabaseListResponse`, `Problem`
   - All HTTP status codes (200, 400, 404, 406, 415, 422, 429, 500) with
     `application/problem+json` error shapes
   - Pagination `links` fields (`next`, `prev`) and correlation/cache headers
   - `security` block (anonymous read for v1)
3. Add request/response examples to `api/openapi/examples/`:
   - `search-get-simple.json`, `search-post-boolean.json`,
     `search-response-200.json`, `search-response-400.json`
4. Write `api/openapi/README.md` explaining how to read the spec,
   versioning policy, and how to regenerate examples.

**Validation:** Run an OpenAPI linter (e.g. `npx @stoplight/spectral-cli lint`)
against the YAML and confirm zero errors.

**Files created:** `api/openapi/search-api.v1.yaml`,
`api/openapi/examples/*.json`, `api/openapi/README.md`

---

## Phase 2 — API Configuration Module

**Goal:** Centralise all runtime configuration (environment variables, compile-time
defaults) behind a single module so every other module reads config from one place.

### Steps

1. Create `Isearch-cgi/api_config.hxx` declaring:
   - `API_VERSION` string constant (`"1"`)
   - `API_DEFAULT_MAX_HITS` (50), `API_HARD_MAX_HITS` (configurable ceiling)
   - `API_DEFAULT_ELEMENT_SET` (`"B"`)
   - `struct ApiConfig` with fields: `db_path`, `max_hits_ceiling`, `allow_list`
   - `ApiConfig LoadApiConfig()` — reads from environment variables
     (`ISEARCH_DB_PATH`, `ISEARCH_API_MAX_HITS`, `ISEARCH_API_ALLOWLIST`)
2. Implement `Isearch-cgi/api_config.cxx` with `LoadApiConfig()`.
3. Add `api_config.o` compile rule to `Isearch-cgi/Makefile` (under its own
   stanza, not yet wired into any binary).

**Validation:** Compile `api_config.cxx` in isolation:
```bash
cd Isearch-cgi && make api_config.o
```

**Files created:** `Isearch-cgi/api_config.hxx`, `Isearch-cgi/api_config.cxx`  
**Files modified:** `Isearch-cgi/Makefile` (add `api_config.o` rule)

---

## Phase 3 — API Response Module

**Goal:** Provide reusable functions for writing well-formed JSON responses and
`application/problem+json` error documents to stdout.  No engine dependency.

### Steps

1. Create `Isearch-cgi/api_response.hxx` declaring:
   - `void WriteHttpHeader(int status, bool problem_json)` — emits status line +
     `Content-Type` + `X-Request-Id` + `Cache-Control` headers
   - `void BeginSearchResponse(const ApiSearchMeta& meta)` — opens the JSON envelope
     (`matching_record_count`, `total_retrieved`, `interpreted_query`, etc.)
   - `void WriteSearchHit(int index, const ApiHit& hit, bool first)` — emits one
     `SearchHit` object in the `results` array
   - `void EndSearchResponse(const ApiLinks& links)` — closes array + `links` object
   - `void WriteProblem(int status, const char* type, const char* title,
     const char* detail)` — emits RFC 9457 problem document
   - `void WriteJsonEscaped(const STRING& value)` — JSON-safe string encoder
     (refactored from `PrintJsonEscaped` in `isrch_srch.cxx`)
   - Plain struct definitions: `ApiSearchMeta`, `ApiHit`, `ApiLinks`
2. Implement `Isearch-cgi/api_response.cxx`.
3. Add `api_response.o` compile rule to `Isearch-cgi/Makefile`.

**Validation:** Write a short `test_response.cxx` that calls `WriteProblem` and
`WriteJsonEscaped` with known inputs and verify stdout manually; discard after
testing.

**Files created:** `Isearch-cgi/api_response.hxx`, `Isearch-cgi/api_response.cxx`  
**Files modified:** `Isearch-cgi/Makefile`

---

## Phase 4 — Request Parsing Module

**Goal:** Parse and validate all GET query-string and POST JSON-body inputs defined
in the OpenAPI spec, including backward-compatible CGI aliases.

### Steps

1. Create `Isearch-cgi/api_request.hxx` declaring:
   - `enum SearchType { SEARCH_SIMPLE, SEARCH_ADVANCED, SEARCH_BOOLEAN }`
   - `enum BoolOperator { OP_OR, OP_AND, OP_ANDNOT, OP_NEAR }`
   - `struct ApiTerm { STRING term; STRING field; STRING weight; bool phrase; }`
   - `struct ApiRequest` with all validated fields:
     `database`, `q`, `search_type`, `op`, `terms`, `element_set`,
     `record_syntax`, `start`, `max_hits`, `include_url`, `include_headline`,
     `include_record_key`, `score_scale`, `request_id`
   - `bool ParseRequest(CGIAPP* cgi, const char* method, const char* body,
     ApiRequest& out, STRING& error_detail)` — returns false and sets
     `error_detail` on validation failure
2. Implement `Isearch-cgi/api_request.cxx`:
   - Detect method (`REQUEST_METHOD` env var: `GET` or `POST`)
   - For `GET`: read parameters via `CGIAPP::GetValueByName`, apply new
     canonical names first then fall back to CGI aliases
     (`DATABASE`, `ISEARCH_TERM`, `SEARCH_TYPE`, `OPERATOR`, `TERM_n`,
     `FIELD_n`, `WEIGHT_n`, `PHRASE_n`, `ELEMENT_SET`, `RecordSyntax`,
     `START`, `MAXHITS`)
   - For `POST`: read `Content-Type` header; if `application/json`, parse the
     JSON body from `stdin` using a minimal hand-rolled parser (no external
     library required — the body is a flat object)
   - Validate all constraints (required fields, integer ranges, enum values)
     and populate an `error_detail` string on failure
   - Assign a `request_id` (use a simple counter or timestamp-based id)
3. Add `api_request.o` compile rule to `Isearch-cgi/Makefile`.

**Validation:** Compile `api_request.cxx` in isolation; manually run the resulting
object against representative CGI environment mocks.

**Files created:** `Isearch-cgi/api_request.hxx`, `Isearch-cgi/api_request.cxx`  
**Files modified:** `Isearch-cgi/Makefile`

---

## Phase 5 — Search Adapter Module

**Goal:** Translate a validated `ApiRequest` into a VIDB/SQUERY search and map
the result set into the response structs defined in Phase 3.  This is the core
bridge between the API layer and the Isearch engine.

### Steps

1. Create `Isearch-cgi/api_search.hxx` declaring:
   - `int ExecuteSearch(const ApiRequest& req, const ApiConfig& cfg,
     ApiSearchMeta& meta, std::vector<ApiHit>& hits, STRING& error_detail)`
     — returns HTTP status code (200 on success, 4xx/5xx on failure)
2. Implement `Isearch-cgi/api_search.cxx`:
   - Construct `DBPathName` and `DBRootName` from `req.database` and
     `cfg.db_path`
   - Open `VIDB`; return 404 if `GetTotalRecords() <= 0`
   - Build `SQUERY` from `req` (mirrors the `switch(SEARCH_TYPE)` block in
     `isrch_srch.cxx::Search`, reusing `INFIX2RPN` for boolean queries)
   - Execute `VIDB::AndSearch` or `VIDB::Search` depending on operator
   - Sort by score; apply `start`/`max_hits` window
   - Populate `ApiSearchMeta` (counts, timing, db size)
   - Populate `vector<ApiHit>` (score, filename, headline, record_key, url)
     using `pdb->Present(…, ESName, req.record_syntax, …)` for headline
   - Compute `ApiLinks` `next`/`prev` URLs using the request's own query
     string as a template
3. Add `api_search.o` compile rule to `Isearch-cgi/Makefile`.

**Validation:** Link a minimal `test_search` binary against `api_search.o` and
`libIsearch.a`; run it against `db/XMLtest` with a known query and verify result
counts match direct `Isearch` CLI output.

**Files created:** `Isearch-cgi/api_search.hxx`, `Isearch-cgi/api_search.cxx`  
**Files modified:** `Isearch-cgi/Makefile`

---

## Phase 6 — Health, Capabilities, and Databases Handlers

**Goal:** Implement the three simpler supporting endpoints, each as a self-contained
function, before writing the router.

### Steps

1. Create `Isearch-cgi/api_endpoints.hxx` declaring:
   - `void HandleHealth()` — writes `{"status":"ok","version":"1"}` with 200
   - `void HandleCapabilities(const ApiConfig& cfg)` — writes search modes,
     element sets, record syntaxes, default/max hit limits, supported operators
   - `void HandleDatabases(const ApiConfig& cfg)` — if `ISEARCH_DB_PATH` is set,
     enumerate `*.mdt` files in that directory and return their stems; otherwise
     return 501 Not Implemented
2. Implement `Isearch-cgi/api_endpoints.cxx` using the `WriteHttpHeader` and
   `WriteJsonEscaped` functions from `api_response`.
3. Add `api_endpoints.o` compile rule to `Isearch-cgi/Makefile`.

**Validation:** Compile in isolation; pipe each handler's stdout through a JSON
parser (`python3 -m json.tool`) to confirm valid JSON output.

**Files created:** `Isearch-cgi/api_endpoints.hxx`, `Isearch-cgi/api_endpoints.cxx`  
**Files modified:** `Isearch-cgi/Makefile`

---

## Phase 7 — API Entrypoint and Router

**Goal:** Write `isrch_api.cxx` — the CGI `main()` that dispatches to the correct
handler based on `PATH_INFO` and HTTP method.

### Steps

1. Create `Isearch-cgi/isrch_api.cxx`:
   - `int main(int argc, char** argv)`:
     - Call `LoadApiConfig()` (Phase 2)
     - Instantiate `CGIAPP` to obtain query-string parameters
     - Read `PATH_INFO` from the environment (e.g. `/search`, `/health`,
       `/capabilities`, `/databases`)
     - Strip the `/v1/api` prefix if present (for installations that do not
       use a URL rewrite rule)
     - Dispatch:
       - `/search` → `ParseRequest` (Phase 4) then `ExecuteSearch` (Phase 5)
         and write response using Phase 3 helpers; stream results progressively
       - `/health` → `HandleHealth()` (Phase 6)
       - `/capabilities` → `HandleCapabilities(cfg)` (Phase 6)
       - `/databases` → `HandleDatabases(cfg)` (Phase 6)
       - anything else → `WriteProblem(404, …)` (Phase 3)
     - Accept header negotiation: if `Accept` contains
       `application/problem+json` but not `application/json`, return 406
     - On POST with unsupported `Content-Type`, return 415
2. Declare the `isrch_api` build target in `Isearch-cgi/Makefile` linking all
   Phase 2–6 objects plus `cgi-util.o`, `config.o`, and `libIsearch.a`.

**Validation:** Build `isrch_api`; invoke it via `curl` with a mock CGI
environment:
```bash
REQUEST_METHOD=GET QUERY_STRING='database=XMLtest&q=dust' \
PATH_INFO='/search' \
./isrch_api /path/to/db | python3 -m json.tool
```

**Files created:** `Isearch-cgi/isrch_api.cxx`  
**Files modified:** `Isearch-cgi/Makefile` (add `isrch_api` target to `all:`)

---

## Phase 8 — Deploy Wrapper Script

**Goal:** Extend the `Isearch-cgi/Configure` script to generate an `isearch_api`
launcher script alongside the existing `isearch`, `ifetch`, and `ihtml` scripts.

### Steps

1. Append to `Isearch-cgi/Configure`:
   ```sh
   rm -f isearch_api
   echo "#!/bin/sh" > isearch_api
   echo "exec $(pwd)/isrch_api $1" >> isearch_api
   chmod 755 isearch_api
   ```
2. Update `Isearch-cgi/README` and `Isearch-cgi/README.md` to document:
   - Running `./Configure <db-path>` to generate the `isearch_api` wrapper
   - Environment variables (`ISEARCH_DB_PATH`, `ISEARCH_API_MAX_HITS`,
     `ISEARCH_API_ALLOWLIST`)
   - CGI deployment example: copy `isearch_api` to your `cgi-bin` directory
   - Apache `ScriptAlias` and `SetEnv` directives for path routing

**Validation:** Run `./Configure /path/to/db` and confirm `isearch_api` is
generated and executable; invoke it from the shell with a test query.

**Files modified:** `Isearch-cgi/Configure`, `Isearch-cgi/README`,
`Isearch-cgi/README.md`

---

## Phase 9 — Smoke Tests Against Real Data

**Goal:** Validate the complete API pipeline end-to-end against the existing
`db/XMLtest` and `db/MDtest` databases before writing the MCP layer.

### Steps

1. Write `Isearch-cgi/test_api.sh` — a shell script that exercises every
   endpoint with known inputs and asserts expected JSON field values:
   - `GET /v1/api/health` → `status == "ok"`
   - `GET /v1/api/capabilities` → parses without error
   - `GET /v1/api/search?database=XMLtest&q=dust` → `matching_record_count > 0`
   - `POST /v1/api/search` with JSON body `{"database":"XMLtest","q":"dust"}`
     → same count as GET result
   - `GET /v1/api/search?database=XMLtest&q=dust&start=2&max_hits=1`
     → `results` has exactly 1 element, `start == 2`
   - `GET /v1/api/search` (no `database`) → HTTP-like 400 error object with
     `"type"` and `"title"` fields
   - `GET /v1/api/search?database=no_such_db&q=dust` → 404 error object
2. Confirm JSON output is field-compatible with the existing `isrch_srch.cxx`
   JSON output when `OUTPUT=JSON` (same field names).

**Validation:** `bash Isearch-cgi/test_api.sh` exits 0.

**Files created:** `Isearch-cgi/test_api.sh`

---

## Phase 10 — MCP Server

**Goal:** Build a standalone MCP stdio server in `mcp-search-server/` that calls
the deployed API.  This phase is entirely independent of the C++ code.

### Steps

1. Choose implementation language: **Python** (no compilation, easy deployment;
   use `fastmcp` or bare `mcp` SDK).
2. Create `mcp-search-server/`:
   - `README.md` — operator docs: installation, env vars, client config
   - `server.py` — MCP stdio server exposing four tools:
     - `search_records` — calls `GET /v1/api/search`, maps params 1-to-1 with
       the OpenAPI schema; returns `SearchResponse` envelope
     - `get_capabilities` — calls `GET /v1/api/capabilities`
     - `health_check` — calls `GET /v1/api/health`
     - `list_databases` — calls `GET /v1/api/databases`
   - `client.py` — typed HTTP client (`urllib` or `httpx`) wrapping the four
     endpoints; handles `application/problem+json` errors by raising a typed
     exception
   - `schemas/` — JSON Schema files for each tool's `inputSchema`, mirrored
     from `api/openapi/search-api.v1.yaml` components
   - `config.example` — annotated example showing `ISEARCH_API_BASE_URL`,
     optional auth headers, and timeouts
   - `requirements.txt` — pinned dependencies
3. Root `Makefile` additions:
   - `mcp-build`: install Python deps via `pip install -r requirements.txt`
   - `mcp-run`: launch `mcp-search-server/server.py` in stdio mode
4. Update root `README.md` with a new **MCP Integration** section covering
   installation, Claude Desktop / VS Code config snippet, and a quick example.

**Validation:** Start the API (Phase 7–8) locally; run the MCP server and
exercise it via `npx @modelcontextprotocol/inspector` or equivalent:
- `search_records` with `database=XMLtest`, `q=dust` returns results
- `health_check` returns `{"status":"ok"}`

**Files created:** `mcp-search-server/` (all files above)  
**Files modified:** `Makefile`, `README.md`

---

## Phase 11 — OpenAPI Lint Rules and CI

**Goal:** Enforce the contract quality rules from the design doc automatically.

### Steps

1. Create `api/openapi/.spectral.yaml` with rules enforcing:
   - Every operation has `operationId`, `summary`, `description`, and `tags`
   - Every parameter has `description` and explicit `schema`
   - Error responses use `application/problem+json`
   - No `additionalProperties: true` on response schemas
2. Add a lint target to the root `Makefile`:
   ```makefile
   openapi-lint:
       npx @stoplight/spectral-cli lint api/openapi/search-api.v1.yaml \
         --ruleset api/openapi/.spectral.yaml
   ```
3. If the project uses CI (`.gitlab-ci.yml` already exists), add a lint job
   that runs `make openapi-lint` on every push to the `api` branch.

**Validation:** `make openapi-lint` exits 0 with zero violations.

**Files created:** `api/openapi/.spectral.yaml`  
**Files modified:** `Makefile`, `.gitlab-ci.yml`

---

## Phase 12 — Deployment Documentation

**Goal:** Provide ready-to-use deployment recipes for the three target
environments in the design doc.

### Steps

1. Create `doc/api-deployment.md` covering:
   - **Apache CGI**: `ScriptAlias`, `SetEnv ISEARCH_DB_PATH`, `Options ExecCGI`
   - **Nginx + fcgiwrap**: `location /v1/api`, `fastcgi_pass` to `fcgiwrap`
     socket, `fastcgi_param PATH_INFO`, env var passthrough
   - **Native daemon + reverse proxy**: run `isrch_api` as a persistent process
     (systemd unit file example), bind to `127.0.0.1:8765`, proxy via
     `ProxyPass /v1/api http://127.0.0.1:8765`
   - **MCP sidecar**: systemd unit and Claude Desktop JSON config snippet
2. Update root `README.md` to reference `doc/api-deployment.md`.

**Validation:** Review only — confirm each example is syntactically valid for
its respective server.

**Files created:** `doc/api-deployment.md`  
**Files modified:** `README.md`

---

## Dependency Summary

```
Phase 1  (OpenAPI spec)         ── no deps
Phase 2  (api_config)           ── no deps
Phase 3  (api_response)         ── Phase 2 (ApiConfig type)
Phase 4  (api_request)          ── Phase 2
Phase 5  (api_search)           ── Phases 2, 3, 4 + engine
Phase 6  (endpoint handlers)    ── Phases 2, 3
Phase 7  (entrypoint/router)    ── Phases 2–6
Phase 8  (wrapper script)       ── Phase 7
Phase 9  (smoke tests)          ── Phase 8
Phase 10 (MCP server)           ── Phase 8 (API deployed); Phase 1 (schema)
Phase 11 (OpenAPI lint)         ── Phase 1
Phase 12 (deployment docs)      ── Phase 7
```

Phases 1, 2, 11, and 12 are fully independent and can be worked on in any order.
Phases 3–6 depend only on Phase 2 and can be developed in parallel once Phase 2 is
done.  Phase 7 is the integration point.

---

## Out of Scope for This Plan

The following items are deferred to later milestones per `TODO.md`:

- Indexing API with authentication (`index-api-auth-design`)
- Collection monitoring / auto-indexing (`collection-monitoring-design`)
- Integration hardening and rollout (`integration-rollout-hardening`)
- Core ranking or index algorithm changes in `src/`
