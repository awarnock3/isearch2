# Isearch HTTP API — User's Guide

This guide covers every endpoint of the Isearch v1 HTTP API.
No configuration or deployment knowledge is required; these are purely usage
instructions for anyone calling the API with `curl` or another HTTP client.

---

## Contents

1. [Base URL and Conventions](#1-base-url-and-conventions)
2. [Common Headers](#2-common-headers)
3. [Error Responses](#3-error-responses)
4. [Endpoint: Health Check — `GET /api/v1/health`](#4-endpoint-health-check)
5. [Endpoint: Capabilities — `GET /api/v1/capabilities`](#5-endpoint-capabilities)
6. [Endpoint: List Databases — `GET /api/v1/databases`](#6-endpoint-list-databases)
7. [Endpoint: Fetch Record — `GET /POST /api/v1/{database}/fetch`](#7-endpoint-fetch-record)
8. [Endpoint: Search (GET) — `GET /api/v1/search`](#8-endpoint-search-get)
9. [Endpoint: Search (POST) — `POST /api/v1/search`](#9-endpoint-search-post)
10. [Search Parameters Reference](#10-search-parameters-reference)
11. [Search Response Reference](#11-search-response-reference)
12. [Pagination](#12-pagination)
13. [HTTP Status Codes](#13-http-status-codes)

---

## 1. Base URL and Conventions

All endpoints are under the versioned base path:

```
/api/v1
```

Substitute your server's scheme, host, and (if applicable) port. Examples
throughout this guide use `http://localhost` as a placeholder:

```
http://localhost/api/v1/search
http://localhost/api/v1/health
```

**Media types**

| Direction           | Content-Type                              |
| ------------------- | ----------------------------------------- |
| Success responses   | `application/json; charset=utf-8`         |
| Error responses     | `application/problem+json; charset=utf-8` |
| POST request bodies | `application/json` (required)             |

The API is read-only and requires no authentication.

---

## 2. Common Headers

### Request headers

| Header                           | When required      | Notes                                                    |
| -------------------------------- | ------------------ | -------------------------------------------------------- |
| `Content-Type: application/json` | POST requests only | Required; returns `415` if missing or wrong              |
| `Accept: application/json`       | Optional           | Omit or set to `*/*` to accept the default JSON response |

### Response headers

| Header          | Description                                                                       |
| --------------- | --------------------------------------------------------------------------------- |
| `X-Request-Id`  | Correlation identifier assigned to the request. Quote this when reporting issues. |
| `Cache-Control` | Cache policy (typically `no-store`).                                              |

---

## 3. Error Responses

All errors are returned as [RFC 9457 Problem Details](https://www.rfc-editor.org/rfc/rfc9457)
with `Content-Type: application/problem+json`.

**Error response structure**

```json
{
  "type":     "https://isearch.invalid/problems/invalid-request",
  "title":    "Invalid request parameters",
  "status":   400,
  "detail":   "Missing required parameter: database",
  "instance": "/api/v1/search"
}
```

| Field      | Type         | Description                                         |
| ---------- | ------------ | --------------------------------------------------- |
| `type`     | string (URI) | Machine-readable error category                     |
| `title`    | string       | Short human-readable summary                        |
| `status`   | integer      | HTTP status code                                    |
| `detail`   | string       | (Optional) Specific explanation for this occurrence |
| `instance` | string (URI) | (Optional) The request path that produced the error |

---

## 4. Endpoint: Health Check

```
GET /api/v1/health
```

Returns liveness information. Use this to confirm the service is running before
issuing search requests.

### Request

No parameters.

```bash
curl -s http://localhost/api/v1/health
```

### Response `200 OK`

```json
{
  "status": "ok",
  "version": "2.0"
}
```

| Field     | Type   | Description                               |
| --------- | ------ | ----------------------------------------- |
| `status`  | string | Always `"ok"` when the service is healthy |
| `version` | string | API service version string                |

---

## 5. Endpoint: Capabilities

```
GET /api/v1/capabilities
```

Returns the query modes, operators, element sets, record syntaxes, and
configured limits supported by this installation. Consult this endpoint before
building queries so you know the available options and the `max_hits` ceiling.

### Request

No parameters.

```bash
curl -s http://localhost/api/v1/capabilities
```

### Response `200 OK`

```json
{
  "api_version": "1.0.0",
  "search_types": ["simple", "advanced", "boolean"],
  "operators": ["and", "or", "andnot", "near"],
  "defaults": {
    "search_type": "simple",
    "operator": "or",
    "element_set": "B",
    "record_syntax": "SUTRS",
    "start": 1,
    "max_hits": 50,
    "score_scale": 100
  },
  "limits": {
    "max_hits_ceiling": 200
  },
  "element_sets": ["B", "F", "S"],
  "record_syntaxes": ["HTML", "SUTRS"]
}
```

| Field                     | Type    | Description                                     |
| ------------------------- | ------- | ----------------------------------------------- |
| `api_version`             | string  | Semantic version of the API                     |
| `search_types`            | array   | Supported `search_type` values                  |
| `operators`               | array   | Supported `operator` values                     |
| `defaults`                | object  | Default values used when parameters are omitted |
| `limits.max_hits_ceiling` | integer | Maximum value accepted for `max_hits`           |
| `element_sets`            | array   | Valid `element_set` codes                       |
| `record_syntaxes`         | array   | Valid `record_syntax` values                    |

---

## 6. Endpoint: List Databases

```
GET /api/v1/databases
```

Returns the database names that are available for searching.
Use these names as the `database` parameter in search requests.

### Request

No parameters.

```bash
curl -s http://localhost/api/v1/databases
```

### Response `200 OK`

```json
{
  "databases": ["XMLtest", "MDtest", "FullCollection"]
}
```

| Field       | Type             | Description                                        |
| ----------- | ---------------- | -------------------------------------------------- |
| `databases` | array of strings | List of database root names available for querying |

---

## 7. Endpoint: Fetch Record

```
GET  /api/v1/{database}/fetch
POST /api/v1/{database}/fetch
```

Retrieves the full rendered content of a single indexed record using its
`record_key`, exactly as it appears in `SearchHit.record_key` in a search
response. Use this to display a complete document without needing to go back
to the source file system.

### Parameters

| Parameter       | Required | Default | Description                                                                                                  |
| --------------- | -------- | ------- | ------------------------------------------------------------------------------------------------------------ |
| `record_key`    | **Yes**  | —       | Record key from a `SearchHit.record_key` field.                                                              |
| `element_set`   | No       | `F`     | Element set controlling which fields are rendered. `F` returns the full record; `B` returns a brief summary. |
| `record_syntax` | No       | `SUTRS` | Presentation format for the JSON `content` field: `HTML` or `SUTRS`.                                        |
| `request_id`    | No       | —       | (POST only) Caller-supplied correlation ID, returned as-is in the response.                                  |

### GET example

```bash
curl -s "http://localhost/api/v1/XMLtest/fetch?record_key=XMLtest%2F1"
```

With optional controls:

```bash
curl -s "http://localhost/api/v1/XMLtest/fetch?\
record_key=XMLtest%2F1\
&element_set=F\
&record_syntax=SUTRS"
```

### POST example

```bash
curl -s -X POST http://localhost/api/v1/XMLtest/fetch \
  -H "Content-Type: application/json" \
  -d '{
    "record_key": "XMLtest/1",
    "element_set": "F",
    "record_syntax": "SUTRS"
  }'
```

### Response `200 OK`

```json
{
  "record_key": "XMLtest/1",
  "database": "XMLtest",
  "filename": "dust.xml",
  "content": "<h2>Dust storm updates for the region</h2>\n<p>Detailed record content follows...</p>"
}
```

| Field        | Type   | Description                                             |
| ------------ | ------ | ------------------------------------------------------- |
| `record_key` | string | The record key that was fetched.                        |
| `database`   | string | The database that was queried.                          |
| `filename`   | string | Source file name for the record.                        |
| `content`    | string | Full rendered content in the requested `record_syntax`. |

> **Tip:** The `record_key` in the URL or JSON body may contain `/` characters.
> In a GET request, URL-encode slashes as `%2F`.

---

## 8. Endpoint: Search (GET)

```
GET /api/v1/search
```

Executes a search using query-string parameters. This is the simplest form and
works well from a browser address bar, shell scripts, and quick `curl` tests.

### Minimal example

```bash
curl -s "http://localhost/api/v1/search?database=XMLtest&q=dust"
```

### Full example with all optional parameters

```bash
curl -s "http://localhost/api/v1/search?\
database=XMLtest\
&q=dust+storm\
&search_type=simple\
&operator=and\
&element_set=B\
&record_syntax=HTML\
&start=1\
&max_hits=10\
&include_url=true\
&include_headline=true\
&include_record_key=true\
&score_scale=100"
```

### Structured-term example (GET)

When you need per-term field targeting, weights, or phrase flags, supply
`term`, `field`, `weight`, and `phrase` as repeated parameters.
Each set of four parameters is aligned by position (index 0, 1, 2, …).

```bash
# Two terms: "dust" in any field (weight 3) AND "storm" as a phrase in TITLE (weight 2)
curl -s "http://localhost/api/v1/search?\
database=XMLtest\
&search_type=boolean\
&operator=and\
&term=dust&field=BODY&weight=3&phrase=false\
&term=storm&field=TITLE&weight=2&phrase=true"
```

---

## 9. Endpoint: Search (POST)

```
POST /api/v1/search
Content-Type: application/json
```

Accepts the same search parameters as the GET form but as a JSON request body.
Use POST when:

- The query string would be impractically long (many terms).
- You are passing structured `terms` arrays with field/weight/phrase per term.
- Your HTTP client or proxy imposes URL-length limits.

### Minimal example

```bash
curl -s -X POST http://localhost/api/v1/search \
  -H "Content-Type: application/json" \
  -d '{"database": "XMLtest", "q": "dust"}'
```

### Simple query with result controls

```bash
curl -s -X POST http://localhost/api/v1/search \
  -H "Content-Type: application/json" \
  -d '{
    "database": "XMLtest",
    "q": "dust",
    "search_type": "simple",
    "operator": "or",
    "start": 1,
    "max_hits": 5,
    "include_url": true,
    "include_headline": true,
    "include_record_key": true,
    "score_scale": 100
  }'
```

### Boolean query with structured terms array

```bash
curl -s -X POST http://localhost/api/v1/search \
  -H "Content-Type: application/json" \
  -d '{
    "database": "XMLtest",
    "search_type": "boolean",
    "operator": "and",
    "terms": [
      { "term": "dust",  "field": "BODY",  "weight": "3", "phrase": false },
      { "term": "storm", "field": "TITLE", "weight": "2", "phrase": true  }
    ],
    "element_set": "B",
    "start": 1,
    "max_hits": 10,
    "include_url": true,
    "include_headline": true,
    "include_record_key": true,
    "score_scale": 100
  }'
```

---

## 10. Search Parameters Reference

All parameters apply to both GET (query-string) and POST (JSON body) unless
noted otherwise.

### Required

| Parameter  | Type   | Description                                                                       |
| ---------- | ------ | --------------------------------------------------------------------------------- |
| `database` | string | Database root/stem to search. Use `GET /api/v1/databases` to see available names. |

### Query semantics

| Parameter     | Type   | Default  | Description                                                                   |
| ------------- | ------ | -------- | ----------------------------------------------------------------------------- |
| `q`           | string | —        | Free-text query string, interpreted according to `search_type`.               |
| `search_type` | enum   | `simple` | Query parser mode: `simple`, `advanced`, or `boolean`.                        |
| `operator`    | enum   | `or`     | Boolean combination for multi-term queries: `and`, `or`, `andnot`, or `near`. |

#### `search_type` values

| Value      | Behavior                                                                       |
| ---------- | ------------------------------------------------------------------------------ |
| `simple`   | Default free-text ranking search. Terms are tokenized and ranked by relevance. |
| `advanced` | Enables advanced query syntax (field qualifiers, wildcards).                   |
| `boolean`  | Strict boolean evaluation using the `operator` and/or `terms` array.           |

#### `operator` values

| Value    | Behavior                                                          |
| -------- | ----------------------------------------------------------------- |
| `or`     | Match records containing any of the terms (broadest).             |
| `and`    | Match records containing all of the terms.                        |
| `andnot` | Match records containing the first term but not subsequent terms. |
| `near`   | Match records where terms appear in proximity to each other.      |

### Structured terms (for targeted field searches)

Use these when you need per-term field, weight, or phrase-match control.
In a GET request, supply each as a repeated parameter; in a POST body, use the
`terms` array.

**GET form** (repeated, aligned by position)

| Parameter | Type               | Description                                                                                |
| --------- | ------------------ | ------------------------------------------------------------------------------------------ |
| `term`    | string (repeated)  | A single search term.                                                                      |
| `field`   | string (repeated)  | Field to search for the aligned `term` (e.g., `BODY`, `TITLE`). Omit to search all fields. |
| `weight`  | string (repeated)  | Numeric weight/boost for the aligned `term`.                                               |
| `phrase`  | boolean (repeated) | `true` to treat the aligned `term` as a phrase; `false` for a word. Default `false`.       |

**POST form** (`terms` array — each element is a `SearchTerm` object)

| Field    | Type    | Required | Description                               |
| -------- | ------- | -------- | ----------------------------------------- |
| `term`   | string  | **Yes**  | The search term text.                     |
| `field`  | string  | No       | Field to search (e.g., `BODY`, `TITLE`).  |
| `weight` | string  | No       | Numeric weight/boost.                     |
| `phrase` | boolean | No       | `true` for phrase match. Default `false`. |

> **Note:** `q` and `terms` can be combined; the engine merges them.

### Result controls

| Parameter            | Type    | Default | Min | Max            | Description                                                                                                                              |
| -------------------- | ------- | ------- | --- | -------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `start`              | integer | `1`     | `1` | —              | First result to return (1-based). Used for pagination.                                                                                   |
| `max_hits`           | integer | `50`    | `1` | server ceiling | Maximum number of results to return. The server-enforced ceiling is reported by `GET /api/v1/capabilities` as `limits.max_hits_ceiling`. |
| `element_set`        | string  | `B`     | —   | —              | Record presentation element set code. Available values listed in capabilities.                                                           |
| `record_syntax`      | enum    | `SUTRS` | —   | —              | Format for the JSON `content` field: `HTML` or `SUTRS`.                                                                                 |
| `include_url`        | boolean | `true`  | —   | —              | Include `url` in each result hit.                                                                                                        |
| `include_headline`   | boolean | `true`  | —   | —              | Include `headline` in each result hit.                                                                                                   |
| `include_record_key` | boolean | `true`  | —   | —              | Include `record_key` in each result hit.                                                                                                 |
| `score_scale`        | integer | `100`   | `1` | —              | Scale factor for relevance scores. The top-ranked result scores `score_scale`; all others are scaled proportionally.                     |

### Optional correlation

| Parameter    | Type   | Description                                                                                         |
| ------------ | ------ | --------------------------------------------------------------------------------------------------- |
| `request_id` | string | (POST body only) Caller-supplied correlation ID. Returned as-is in the response `request_id` field. |

---

## 11. Search Response Reference

A successful `200 OK` search response has the following structure:

```json
{
  "request_id": "req-20260801-0001",
  "database": "XMLtest",
  "search_type": "simple",
  "matching_record_count": 3,
  "total_retrieved": 3,
  "interpreted_query": "dust",
  "total_database_records": 5,
  "query_time_seconds": 0.012,
  "start": 1,
  "max_hits": 5,
  "results": [
    {
      "match_number": 1,
      "score": 100,
      "filename": "dust.xml",
      "headline": "Dust storm updates for the region",
      "record_key": "XMLtest/1",
      "url": "https://example.com/dust.xml"
    },
    {
      "match_number": 2,
      "score": 84,
      "filename": "goes_9_conus.xml",
      "headline": "Weather satellite imagery and dust transport",
      "record_key": "XMLtest/2",
      "url": "https://example.com/goes_9_conus.xml"
    }
  ],
  "links": {
    "next": null,
    "prev": null
  }
}
```

### Top-level fields

| Field                    | Type    | Description                                                   |
| ------------------------ | ------- | ------------------------------------------------------------- |
| `request_id`             | string  | Correlation ID (server-generated unless you supplied one).    |
| `database`               | string  | Database that was searched.                                   |
| `search_type`            | string  | Effective `search_type` used.                                 |
| `matching_record_count`  | integer | Total records in the database that matched the query.         |
| `total_retrieved`        | integer | Number of records returned in this response page.             |
| `interpreted_query`      | string  | The query as understood by the engine (useful for debugging). |
| `total_database_records` | integer | Total records in the database (matched or not).               |
| `query_time_seconds`     | number  | Time the engine spent executing the query.                    |
| `start`                  | integer | First result position returned (1-based).                     |
| `max_hits`               | integer | Effective `max_hits` used.                                    |
| `results`                | array   | Array of `SearchHit` objects (see below).                     |
| `links`                  | object  | Pagination links (see [Pagination](#11-pagination)).          |

### SearchHit fields

| Field          | Type         | Description                                                                                    |
| -------------- | ------------ | ---------------------------------------------------------------------------------------------- |
| `match_number` | integer      | Rank position (1 = best match).                                                                |
| `score`        | integer      | Relevance score, scaled to `score_scale`.                                                      |
| `filename`     | string       | Source file name for this record.                                                              |
| `headline`     | string       | Brief descriptive headline extracted from the record. Present unless `include_headline=false`. |
| `record_key`   | string       | Stable internal key for the record. Present unless `include_record_key=false`.                 |
| `url`          | string (URI) | URL to the source document. Present unless `include_url=false`.                                |

---

## 12. Pagination

Large result sets can be paged using `start` and `max_hits`.

- `start` is **1-based**: the first result is `start=1`.
- `max_hits` controls the page size.
- The response `links.next` and `links.prev` fields contain ready-made URLs you
  can follow directly, or `null` when there is no further page in that direction.

### Example: page through results 10 at a time

**Page 1**

```bash
curl -s "http://localhost/api/v1/search?database=XMLtest&q=dust&start=1&max_hits=10"
```

**Page 2** (if `matching_record_count > 10`)

```bash
curl -s "http://localhost/api/v1/search?database=XMLtest&q=dust&start=11&max_hits=10"
```

Or follow the `links.next` URL directly:

```bash
NEXT=$(curl -s "http://localhost/api/v1/search?database=XMLtest&q=dust&start=1&max_hits=10" \
       | python3 -c "import sys,json; print(json.load(sys.stdin)['links']['next'] or '')")
[ -n "$NEXT" ] && curl -s "http://localhost${NEXT}"
```

---

## 13. HTTP Status Codes

| Status                       | Meaning                | Likely cause                                                                               |
| ---------------------------- | ---------------------- | ------------------------------------------------------------------------------------------ |
| `200 OK`                     | Success                | Request was valid and search completed.                                                    |
| `400 Bad Request`            | Invalid parameters     | Missing required `database`; parameter value out of range; or an **unknown parameter name** (e.g. `max_hist` instead of `max_hits`). |
| `404 Not Found`              | Unknown resource       | The `database` name does not exist or is not accessible; or an unrecognized endpoint path. |
| `406 Not Acceptable`         | Accept header mismatch | Your `Accept` header excludes `application/json`.                                          |
| `415 Unsupported Media Type` | Wrong content type     | POST body sent without `Content-Type: application/json`.                                   |
| `422 Unprocessable Entity`   | Query parse failure    | The query string or terms could not be interpreted by the engine.                          |
| `429 Too Many Requests`      | Rate limited           | Server-configured rate limit reached; slow down and retry.                                 |
| `500 Internal Server Error`  | Engine failure         | An unexpected error occurred inside the search engine.                                     |

All error responses include a `Problem` document (see [Error Responses](#3-error-responses)).