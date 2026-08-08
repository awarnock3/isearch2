# Isearch API Examples

## Health
- curl -sS http://localhost/v1/api/health

## Identity
- curl -sS http://localhost/v1/api/identity

## Capabilities
- curl -sS http://localhost/v1/api/capabilities

## Databases
- curl -sS http://localhost/v1/api/databases

## Search
- curl -sS 'http://localhost/v1/api/search?database=Markdown&q=Features'
- curl -sS 'http://localhost/v1/api/search?database=Markdown&q=Features&ElementSet=S'
- curl -sS 'http://localhost/v1/api/search?database=Isite&q=test&element_set=B&max_hits=5'
- curl -sS 'http://localhost/v1/api/search?database=Isite&q=test&element_set=B&start=6&max_hits=5'
- curl -sS 'http://localhost/v1/api/search?database=Isite&q=test&element_set=S&record_syntax=SUTRS&start=6&max_hits=5'
- curl -sS 'http://localhost/v1/api/search?database=Isite&q=test&element_set=F&start=6&max_hits=1'

## Fetch
- curl -sS 'http://localhost/v1/api/fetch?database=Markdown&record_key=29903'
