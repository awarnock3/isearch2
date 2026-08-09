// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include <stdlib.h>
#include <string.h>

#include "api_config.hxx"
#include "api_endpoints.hxx"
#include "api_request.hxx"
#include "api_response.hxx"
#include "api_search.hxx"
#include "cgi-util.hxx"

static bool ContainsNoCase(const CHR *value, const CHR *needle)
{
  if (value == nullptr || needle == nullptr || needle[0] == '\0') {
    return false;
  }
  const size_t value_len = strlen(value);
  const size_t needle_len = strlen(needle);
  if (needle_len > value_len) {
    return false;
  }
  for (size_t i = 0; i + needle_len <= value_len; i++) {
    size_t j = 0;
    for (; j < needle_len; j++) {
      CHR a = value[i + j];
      CHR b = needle[j];
      if (a >= 'A' && a <= 'Z') a = (CHR)(a - 'A' + 'a');
      if (b >= 'A' && b <= 'Z') b = (CHR)(b - 'A' + 'a');
      if (a != b) break;
    }
    if (j == needle_len) {
      return true;
    }
  }
  return false;
}

static bool WantsProblemOnly(const CHR *accept_header)
{
  if (accept_header == nullptr || accept_header[0] == '\0') {
    return false;
  }
  const bool wants_problem = ContainsNoCase(accept_header, "application/problem+json");
  const bool wants_json = ContainsNoCase(accept_header, "application/json");
  return wants_problem && !wants_json;
}

static STRING BuildSearchLink(const ApiRequest& req, INT start)
{
  // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiisrch_apicxx): STRING has no
  // Cat(INT) overload, only Cat(UCHR) and Cat(const STRING&) (the latter
  // reachable via STRING's own INT-converting constructor). Passing an INT
  // directly resolves to Cat(UCHR) -- a standard integral conversion beats
  // the user-defined conversion needed to reach Cat(const STRING&) -- so
  // it appended the raw byte value of the number (truncated mod 256)
  // instead of its decimal digits. Confirmed live against the real
  // isrch_api binary: start=65/max_hits=66 produced a raw 0x01 control
  // byte (from a clamped prev_start of 1) and a literal "B" (byte value 66)
  // in the emitted "prev" link instead of "1" and "66" -- silently
  // corrupting every pagination link this file ever generates. Fixed by
  // wrapping each value in STRING(...) explicitly so Cat(const STRING&)
  // is the only viable overload.
  STRING link = "/api/v1/search?database=";
  link.Cat(req.database);
  if (req.q.GetLength() > 0) {
    link.Cat("&q=");
    link.Cat(req.q);
  }
  link.Cat("&start=");
  link.Cat(STRING(start));
  link.Cat("&max_hits=");
  link.Cat(STRING(req.max_hits));
  return link;
}

static STRING NormalizePath(const CHR *raw_path)
{
  STRING path = raw_path != nullptr ? raw_path : "";
  if (path.GetLength() == 0) {
    return "/";
  }
  if (path.Search("/api/v1") == 1) {
    path.EraseBefore(8);
    if (path.GetLength() == 0) {
      return "/";
    }
  }
  return path;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  const CHR *method = getenv("REQUEST_METHOD");
  if (method == nullptr || method[0] == '\0') {
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request", "REQUEST_METHOD is required.");
    return 0;
  }

  const CHR *accept_header = getenv("HTTP_ACCEPT");
  if (accept_header == nullptr || accept_header[0] == '\0') {
    accept_header = getenv("ACCEPT");
  }
  if (WantsProblemOnly(accept_header)) {
    WriteHttpHeader(406, true);
    WriteProblem(406, "https://isearch.invalid/problems/not-acceptable",
                 "Not acceptable",
                 "Accept header must allow application/json.");
    return 0;
  }

  ApiConfig cfg = LoadApiConfig();
  const STRING path = NormalizePath(getenv("PATH_INFO"));

  if (path == "/health") {
    HandleHealth();
    return 0;
  }
  if (path == "/capabilities") {
    HandleCapabilities(cfg);
    return 0;
  }
  if (path == "/databases") {
    HandleDatabases(cfg);
    return 0;
  }
  if (!(path == "/search")) {
    WriteHttpHeader(404, true);
    WriteProblem(404, "https://isearch.invalid/problems/not-found",
                 "Not found", "Unknown API endpoint.");
    return 0;
  }

  if (!(StrCaseCmp(method, "GET") == 0 || StrCaseCmp(method, "POST") == 0)) {
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request", "Only GET and POST are supported.");
    return 0;
  }

  if (StrCaseCmp(method, "POST") == 0) {
    const CHR *content_type = getenv("CONTENT_TYPE");
    if (content_type == nullptr || content_type[0] == '\0') {
      content_type = getenv("HTTP_CONTENT_TYPE");
    }
    if (!ContainsNoCase(content_type, "application/json")) {
      WriteHttpHeader(415, true);
      WriteProblem(415, "https://isearch.invalid/problems/unsupported-media-type",
                   "Unsupported media type",
                   "Content-Type must be application/json.");
      return 0;
    }
  }

  CGIAPP *cgi = nullptr;
  if (StrCaseCmp(method, "GET") == 0) {
    cgi = new CGIAPP();
  }

  ApiRequest req;
  STRING parse_error;
  if (!ParseRequest(cgi, method, nullptr, req, parse_error)) {
    if (cgi != nullptr) delete cgi;
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request parameters", parse_error);
    return 0;
  }

  ApiSearchMeta meta;
  std::vector<ApiHit> hits;
  STRING error_detail;
  const int status = ExecuteSearch(req, cfg, meta, hits, error_detail);
  if (cgi != nullptr) delete cgi;

  if (status != 200) {
    WriteHttpHeader(status, true);
    WriteProblem(status, "https://isearch.invalid/problems/search-failed",
                 "Search failed", error_detail);
    return 0;
  }

  WriteHttpHeader(200, false);
  BeginSearchResponse(meta);
  for (size_t i = 0; i < hits.size(); i++) {
    WriteSearchHit((INT)(req.start + (INT)i), hits[i], i == 0);
  }

  ApiLinks links;
  if (req.start > 1) {
    INT prev_start = req.start - req.max_hits;
    if (prev_start < 1) prev_start = 1;
    links.prev = BuildSearchLink(req, prev_start);
  }
  INT next_start = req.start + req.max_hits;
  if (next_start <= meta.matching_record_count) {
    links.next = BuildSearchLink(req, next_start);
  }
  EndSearchResponse(links);
  return 0;
}
