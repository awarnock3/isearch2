# TODO

Based on the active implementation plan.

## Completed

- [x] Add Markdown doctype support (`doctype/markdown.*`, registered in `doctype/dtconf.inf`)

## Next ready item

- [ ] Establish modernization baseline (`modernization-baseline`)
  - Capture current compiler behavior/warnings.
  - Confirm generated vs hand-edited file boundaries.

## Pending (sequenced)

- [ ] Upgrade to modern ISO C++ (`iso-cpp-upgrade`) — phased C++17 baseline, then targeted C++20 evaluation
- [ ] Run comprehensive codebase health + security analysis (`health-security-analysis`)
- [ ] Regenerate and expand README (`readme-regeneration`)
- [ ] Design collection monitoring for auto-indexing (`collection-monitoring-design`) — hybrid fs-events + reconciliation polling
- [ ] Design JSON search result contract (`json-results-design`)
- [ ] Design Search API (`search-api-design`) — sidecar service MVP: basic search + document fetch
- [ ] Design Indexing API with authentication (`index-api-auth-design`) — JWT/OIDC MVP: submit/update + job status
- [ ] Integration hardening and rollout (`integration-rollout-hardening`)
