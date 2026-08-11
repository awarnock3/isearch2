# Isearch API Examples

## Health
- curl -sS http://localhost/v1/api/health

## Identity
- curl -sS http://localhost/v1/api/identity

## Capabilities
- curl -sS http://localhost/v1/api/capabilities

## Databases
- curl -sS http://localhost/v1/api/databases

## Search (GET)
- curl -sS 'http://localhost/v1/api/Markdown/search?q=Features'
- curl -sS 'http://localhost/v1/api/Markdown/search?q=Features&ElementSet=S'
- curl -sS 'http://localhost/v1/api/Isite/search?q=test&element_set=B&max_hits=5'
- curl -sS 'http://localhost/v1/api/Isite/search?q=test&element_set=B&start=6&max_hits=5'
- curl -sS 'http://localhost/v1/api/Isite/search?q=test&element_set=S&record_syntax=SUTRS&start=6&max_hits=5'
- curl -sS 'http://localhost/v1/api/Isite/search?q=test&element_set=F&start=6&max_hits=1'

## Search (POST)
- curl -sS -X POST -H "Content-Type: application/json" -d '{"q" : "test"}'  http://localhost/v1/api/Isite/search | jq

## Fetch (GET)
- curl -sS 'http://localhost/v1/api/Markdown/fetch?record_key=29903'

## Fetch (POST)
