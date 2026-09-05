# Isearch OpenAPI CGI Deployment Guide

This guide covers deployment of the OpenAPI CGI endpoint runner (`isrch_api`)
behind Apache and Nginx.

The API endpoints are:

- `/api/v1/health`
- `/api/v1/capabilities`
- `/api/v1/databases`
- `/api/v1/search` (GET and POST JSON)

## Databases response

`/api/v1/databases` returns each configured database as an object, not a
bare string:

```json
{
  "databases": [
    { "name": "XMLtest", "doctype": "xml" },
    { "name": "patents", "doctype": "usmarc" }
  ]
}
```

`doctype` reflects the database's configured global Isearch document type
(`IDB`/`VIDB::GetGlobalDocType`) and is omitted for a database entry that
could not be opened to determine it; `name` is always present.

## Search request options

Both GET `/api/v1/search` and POST `/api/v1/search` support path-style
database routing and request fields aligned with `Isearch` options.

Core fields:

- `database` (required when not supplied by path)
- `q` or `terms[]`
- `search_type` (`simple|advanced|boolean`)
- `operator` (`and|or|andnot|near`)
- `element_set`
- `record_syntax` (`TEXT|SUTRS|USMARC|HTML|SGML|XML|GRS-1` and supported OIDs)
- `start`, `max_hits`
- `include_url`, `include_headline`, `include_record_key`
- `score_scale`

Additional option fields:

- `rpn`, `infix`, `and_mode` (mutually exclusive controls)
- `synonyms`
- `doc_type_option` (GET repeatable) / `doc_type_options` (POST array)
- `highlight_prefix`, `highlight_suffix`
- `byte_range` (include structured byte offsets in response hits)
- `start_doc`, `end_doc`
- `rect` (north,south,west,east)

Byte range response fields:

- When `byte_range=true`, each hit may include:
  - `record_start`
  - `record_end`

These are structured JSON fields, not text appended to `headline`.

## 1. Build and prepare wrappers

From the repository root:

```bash
./configure
make
cd Isearch-cgi
./Configure /absolute/path/to/databases
```

`./Configure` generates wrapper scripts:

- `isearch`
- `ifetch`
- `ihtml`
- `isearch_api` (wrapper for `isrch_api`)

Use absolute database paths. The wrapper executes `isrch_api` with the database
path argument.

## 2. Runtime environment variables

`isrch_api` reads:

- `ISEARCH_DB_PATH` (recommended): base directory for database roots
- `ISEARCH_API_MAX_HITS`: ceiling for `max_hits`
- `ISEARCH_API_ALLOWLIST`: comma-separated allowed database names

Example:

```text
ISEARCH_DB_PATH=/srv/isearch/db
ISEARCH_API_MAX_HITS=200
ISEARCH_API_ALLOWLIST=XMLtest,MDtest
```

## 3. Apache CGI deployment

### 3.1 Prerequisites

Enable CGI modules (`cgi` or `cgid`) and ensure script execution is allowed.

### 3.2 Install scripts and binaries

Example layout:

- Wrapper in CGI directory: `/usr/lib/cgi-bin/isearch_api`
- Built API binary: `/opt/isearch/Isearch-cgi/isrch_api`
- Databases: `/srv/isearch/db`

Copy wrapper and keep it executable:

```bash
install -m 0755 Isearch-cgi/isearch_api /usr/lib/cgi-bin/isearch_api
```

If the runtime user cannot execute files under your source tree, copy
`isrch_api` and linked files to an executable location readable by Apache.

### 3.3 VirtualHost configuration

Use `ScriptAlias` so `/api/v1/...` maps to the wrapper and preserves
`PATH_INFO`:

```apacheconf
ScriptAlias /api/v1 /usr/lib/cgi-bin/isearch_api

<Location /api/v1>
  Options +ExecCGI
  AcceptPathInfo On
  SetEnv ISEARCH_DB_PATH /srv/isearch/db
  SetEnv ISEARCH_API_MAX_HITS 200
  SetEnv ISEARCH_API_ALLOWLIST XMLtest,MDtest
</Location>
```

Notes:

- `AcceptPathInfo On` is required so `/api/v1/search` passes `/search` to CGI.
- The API binary accepts both direct `/search` and `/api/v1/search` style paths.
- If using SELinux/AppArmor, allow the server process to execute the wrapper
  and read database files.

### 3.4 Apache smoke checks

```bash
curl -sS 'http://127.0.0.1/api/v1/health'
curl -sS 'http://127.0.0.1/api/v1/capabilities'
curl -sS 'http://127.0.0.1/api/v1/search?database=XMLtest&q=dust'
curl -sS -X POST 'http://127.0.0.1/api/v1/search' \
  -H 'Content-Type: application/json' \
  -d '{"database":"XMLtest","q":"dust"}'
curl -sS 'http://127.0.0.1/api/v1/search?database=XMLtest&q=xml&byte_range=true&max_hits=1'
```

## 4. Nginx + fcgiwrap deployment

Nginx does not run CGI directly; use `fcgiwrap`.

### 4.1 Prerequisites

Install and run:

- `fcgiwrap`
- a FastCGI transport (commonly Unix socket at `/run/fcgiwrap.socket`)

Ensure the `fcgiwrap` service user can execute `isearch_api` and read database
files.

### 4.2 Install wrapper

Example:

```bash
install -m 0755 Isearch-cgi/isearch_api /usr/lib/cgi-bin/isearch_api
```

### 4.3 Nginx server block snippet

```nginx
location ~ ^/api/v1(/.*)?$ {
    include fastcgi_params;

    # Route all /api/v1 requests to the isearch_api wrapper.
    fastcgi_param SCRIPT_FILENAME /usr/lib/cgi-bin/isearch_api;
    fastcgi_param SCRIPT_NAME /api/v1;
    fastcgi_param PATH_INFO $1;
    fastcgi_param REQUEST_METHOD $request_method;
    fastcgi_param QUERY_STRING $query_string;
    fastcgi_param CONTENT_TYPE $content_type;
    fastcgi_param CONTENT_LENGTH $content_length;

    # API runtime config.
    fastcgi_param ISEARCH_DB_PATH /srv/isearch/db;
    fastcgi_param ISEARCH_API_MAX_HITS 200;
    fastcgi_param ISEARCH_API_ALLOWLIST XMLtest,MDtest;

    fastcgi_pass unix:/run/fcgiwrap.socket;
}
```

Notes:

- Regex capture `(/.*)?` maps to `PATH_INFO` (`/search`, `/health`, etc.).
- `SCRIPT_FILENAME` must point to the wrapper script, not to a directory.
- If your distro uses a different socket path, update `fastcgi_pass`.

### 4.4 Nginx smoke checks

```bash
curl -sS 'http://127.0.0.1/api/v1/health'
curl -sS 'http://127.0.0.1/api/v1/search?database=XMLtest&q=dust'
```

## 5. Troubleshooting

- **`404 Unknown API endpoint`**: `PATH_INFO` not being forwarded correctly.
  Check `AcceptPathInfo` (Apache) or `fastcgi_param PATH_INFO` (Nginx).
- **`404 Database does not exist or is corrupted`**: wrong database root/name.
  Verify `ISEARCH_DB_PATH` and query `database=` value.
- **`415 Unsupported media type`**: POST must send
  `Content-Type: application/json`.
- **`406 Not acceptable`**: client `Accept` header excludes `application/json`.

## 6. Operational checklist

1. Build binaries (`make`) and generate wrappers (`./Configure <db-path>`).
2. Deploy `isearch_api` to CGI path with execute permissions.
3. Set `ISEARCH_DB_PATH` (and optional max/allowlist).
4. Configure routing so `/api/v1/*` passes `PATH_INFO`.
5. Run curl smoke checks for health and search endpoints.
