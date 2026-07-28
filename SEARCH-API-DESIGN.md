# Search API Design Plan (`search-api-design`)

## Objective

Define a **fully OpenAPI-compliant** HTTP Search API for Isearch, then define the modifications required to build an **MCP server** that uses this API as its backend.

This is design-only.

## OpenAPI Compliance Requirements (mandatory)

The API contract will be authored as **OpenAPI 3.1.0** (JSON Schema 2020-12 aligned) and treated as the source of truth.

1. `openapi: 3.1.0` with `info`, `servers`, `tags`, `paths`, `components`.
2. Every operation has unique `operationId`, tags, summary, and description.
3. Every request parameter is explicitly declared (`in`, `name`, `required`, `schema`, defaults, enum/range constraints).
4. `requestBody` schemas for JSON POST are explicit and reusable via `components/schemas`.
5. Every response code has explicit `content` schema and examples.
6. Errors are documented using `application/problem+json` (RFC 9457/7807 style).
7. Pagination, correlation headers, and caching headers are modeled in response headers.
8. Security requirements are explicit in spec (`security` block), even if v1 is anonymous read.
9. No undocumented parameters accepted in strict mode.

## OpenAPI Artifacts to Add

1. `api/openapi/search-api.v1.yaml` (canonical contract).
2. `api/openapi/examples/` (request/response examples).
3. `api/openapi/README.md` (how to read and version the spec).

Optional but recommended for enforcement:

4. `api/openapi/.spectral.yaml` (lint rules).

## API Base and Versioning

- Base path: `/api/v1`
- Media type:
  - success payloads: `application/json; charset=utf-8`
  - error payloads: `application/problem+json; charset=utf-8`
- Versioning: URI major version (`v1`).

## Endpoint Plan (OpenAPI paths)

| Method | Path | operationId | Purpose |
|---|---|---|---|
| GET | `/api/v1/search` | `searchRecordsGet` | Query-string search |
| POST | `/api/v1/search` | `searchRecordsPost` | JSON-body search |
| GET | `/api/v1/health` | `getHealth` | Liveness/readiness |
| GET | `/api/v1/capabilities` | `getCapabilities` | Query modes, element sets, limits |
| GET | `/api/v1/databases` | `listDatabases` | Optional configured database list |

## `/api/v1/search` Parameters

### Required

- `database` (string): DB root/stem (`DATABASE` equivalent).

### Query semantics

- `q` (string): interpreted query string.
- `search_type` (enum): `simple` | `advanced` | `boolean` (default `simple`).
- `operator` (enum): `and` | `or` | `andnot` | `near`.

### Structured term support (optional)

- `terms` (array objects in POST body): `{ term, field?, weight?, phrase? }`
- GET form:
  - repeated `term`
  - repeated `field`
  - repeated `weight`
  - repeated `phrase`
  - style: `form`, `explode: true`.

### Result controls (optional)

- `element_set` (string, default `B`)
- `start` (integer, default `1`, minimum `1`)
- `max_hits` (integer, default `50`, minimum `1`, maximum configurable, documented in capabilities)
- `include_url` (boolean, default `true`)
- `include_headline` (boolean, default `true`)
- `include_record_key` (boolean, default `true`)
- `score_scale` (integer, default `100`, minimum `1`)

### Compatibility aliases (optional mode)

Alias parsing for migration from current CGI clients:
`DATABASE`, `ISEARCH_TERM`, `SEARCH_TYPE`, `OPERATOR`, `TERM_n`, `FIELD_n`, `WEIGHT_n`, `PHRASE_n`, `ELEMENT_SET`, `START`, `MAXHITS`.

## Response Schemas (OpenAPI components)

- `SearchResponse`
  - `request_id`
  - `database`
  - `search_type`
  - `matching_record_count`
  - `total_retrieved`
  - `interpreted_query`
  - `total_database_records`
  - `query_time_seconds`
  - `start`
  - `max_hits`
  - `results` (array of `SearchHit`)
  - `links` (`next`, `prev`)
- `SearchHit`
  - `match_number`, `score`, `filename`, `headline`, `record_key`, `url`
- `CapabilitiesResponse`
- `HealthResponse`
- `DatabaseListResponse`
- `Problem` (RFC problem details for errors)

## HTTP Status Plan

- `200` success
- `400` invalid request/parameters
- `404` database not found/disabled
- `406` unacceptable `Accept` header
- `415` unsupported media type (POST)
- `422` query parse failure
- `429` rate-limited (if enabled)
- `500` internal execution failure

## Source Code Impact Plan

### Existing files to modify

1. `Isearch-cgi/Makefile`
   - Add `isrch_api` build target and API object modules.
2. `Isearch-cgi/Configure`
   - Generate `isearch_api` launcher script for CGI deployment.
3. `Isearch-cgi/README` and `Isearch-cgi/README.md`
   - Add API install/config examples.
4. `README` and `README.md`
   - Add top-level API usage/deploy overview.
5. `Isearch-cgi/isrch_srch.cxx` (minimal)
   - Extract reusable search/result mapping logic if needed.
6. `Isearch-cgi/config.hxx` and `Isearch-cgi/config.cxx` (optional)
   - Add API defaults/version/config constants.

### New modules to add (Search API)

1. `Isearch-cgi/isrch_api.cxx` (API entrypoint/router).
2. `Isearch-cgi/api_request.hxx` / `Isearch-cgi/api_request.cxx` (GET/POST parsing + validation + alias normalization).
3. `Isearch-cgi/api_search.hxx` / `Isearch-cgi/api_search.cxx` (adapter to `VIDB`/`SQUERY`/`IRSET`).
4. `Isearch-cgi/api_response.hxx` / `Isearch-cgi/api_response.cxx` (JSON and problem+json envelopes).
5. `Isearch-cgi/api_config.hxx` / `Isearch-cgi/api_config.cxx` (runtime env/config parsing).

## MCP Server Design (on top of Search API)

The MCP layer is a separate front end that calls `/api/v1/*` rather than linking to engine internals.

### MCP tools to expose

1. `search_records`
   - Input: database/query/search_type/operator/terms/element_set/start/max_hits.
   - Backend call: `GET` or `POST /api/v1/search`.
2. `get_capabilities`
   - Backend call: `GET /api/v1/capabilities`.
3. `health_check`
   - Backend call: `GET /api/v1/health`.
4. `list_databases` (optional)
   - Backend call: `GET /api/v1/databases`.

### New modules to add (MCP server)

1. `mcp-search-server/README.md` (operator docs).
2. `mcp-search-server/server.py` or `server.ts` (MCP stdio server; choose one implementation language at implementation time).
3. `mcp-search-server/client.py` or `client.ts` (typed API client for OpenAPI endpoints).
4. `mcp-search-server/schemas/` (tool input/output schemas aligned to OpenAPI).
5. `mcp-search-server/config.example` (API base URL, auth headers/timeouts).

### Build/deploy changes for MCP

1. Root `Makefile`
   - Add targets:
     - `make mcp-build`
     - `make mcp-run`
     - `make mcp-install` (optional local install)
2. Root `README.md`
   - Add MCP setup and client integration section.
3. Optional lock/dependency file for chosen MCP implementation stack.

## OpenAPI ↔ MCP contract alignment rules

1. MCP tool schemas are generated or manually mirrored from OpenAPI components.
2. MCP server must reject inputs violating OpenAPI constraints before forwarding.
3. MCP responses pass through API response envelopes without lossy field renaming.
4. Error mapping:
   - API `Problem` object -> MCP tool error payload preserving `type`, `title`, `status`, `detail`.

## Deployment Plan

### Apache (CGI front end)

- Build `isrch_api` and deploy wrapper script into `cgi-bin`.
- Route:
  - `ScriptAlias /api/v1/search /usr/lib/cgi-bin/isearch_api`
  - optional `ScriptAlias /api/v1 /usr/lib/cgi-bin/isearch_api` with `PATH_INFO` routing.
- Set environment:
  - `ISEARCH_DB_PATH`
  - `ISEARCH_API_MAX_HITS`
  - `ISEARCH_API_ALLOWLIST` (optional)

### Nginx

1. **CGI compatibility mode**: `nginx -> fcgiwrap -> isearch_api`.
2. **Preferred production mode**: `nginx -> native isearch_api daemon`.

### Native

- Run API as local daemon bound to localhost.
- Reverse proxy TLS termination via Apache/Nginx.
- Deploy MCP server as separate process:
  - stdio mode for local MCP clients.
  - optional HTTP/SSE bridge only if required by runtime.

## Compatibility Strategy

1. Keep existing CGI tools (`isearch`, `ifetch`, `ihtml`) unchanged.
2. Support compatibility aliases during transition, with deprecation notes in OpenAPI.
3. Keep JSON field names stable across CLI `-json`, CGI JSON, API JSON, and MCP tool outputs.

## Phased Implementation Plan (after design approval)

1. Author `search-api.v1.yaml` and freeze schema names/operationIds.
2. Implement API modules and `isrch_api` endpoint.
3. Implement OpenAPI-compliant error/status/header behavior.
4. Add MCP server with tool schemas mapped to OpenAPI.
5. Add deployment scripts/docs for Apache, Nginx, and native daemon + MCP sidecar.
6. Validate compatibility using existing `db/XMLtest` and JSON search behavior.

## Out of Scope

- Indexing API and authentication implementation (`index-api-auth-design`).
- Collection monitoring/auto-indexing (`collection-monitoring-design`).
- Core ranking/index algorithm changes in `src/`.
