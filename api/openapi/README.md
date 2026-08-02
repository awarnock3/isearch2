# Isearch OpenAPI Specification (v1)

This directory contains the canonical API contract for the Isearch HTTP API.

## Files

- `search-api.v1.yaml`: OpenAPI 3.1.0 source of truth.
- `examples/`: request/response payload examples referenced by the spec.

## How to read the spec

1. Start with `info`, `servers`, and `tags`.
2. Review `paths` for endpoint behavior and status codes.
3. Use `components/schemas` for canonical request/response shapes.
4. Use `components/responses` for shared error documents (`application/problem+json`).

## Versioning policy

- URI major versioning is used (`/api/v1`).
- Backward-incompatible changes require a new major path version.
- Backward-compatible changes may add optional fields/operations within v1.

## Regenerating examples

Examples are plain JSON files in `examples/`.

To regenerate them:

1. Capture representative requests and responses from a running API build.
2. Normalize fields to match schema constraints in `search-api.v1.yaml`.
3. Replace only files under `examples/` and keep names stable.
4. Keep error examples as `application/problem+json` payloads.
