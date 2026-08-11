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
  if (value == NULL || needle == NULL || needle[0] == '\0') {
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
  if (accept_header == NULL || accept_header[0] == '\0') {
    return false;
  }
  const bool wants_problem = ContainsNoCase(accept_header, "application/problem+json");
  const bool wants_json = ContainsNoCase(accept_header, "application/json");
  return wants_problem && !wants_json;
}

static STRING BuildSearchLink(const ApiRequest& req, INT start)
{
  STRING link = "/v1/api/";
  link.Cat(req.database);
  link.Cat("/search");

  bool has_param = false;
#define APPEND_PARAM(K, V) \
  do { \
    if (has_param) link.Cat("&"); else { link.Cat("?"); has_param = true; } \
    link.Cat((K)); link.Cat("="); link.Cat((V)); \
  } while (0)

  if (req.q.GetLength() > 0) APPEND_PARAM("q", req.q);
  if (req.search_type == SEARCH_ADVANCED) APPEND_PARAM("search_type", "advanced");
  if (req.search_type == SEARCH_BOOLEAN) APPEND_PARAM("search_type", "boolean");
  if (req.op == OP_AND) APPEND_PARAM("operator", "and");
  if (req.op == OP_ANDNOT) APPEND_PARAM("operator", "andnot");
  if (req.op == OP_NEAR) APPEND_PARAM("operator", "near");
  if (req.rpn) APPEND_PARAM("rpn", "true");
  if (req.infix) APPEND_PARAM("infix", "true");
  if (req.and_mode) APPEND_PARAM("and_mode", "true");
  if (req.synonyms) APPEND_PARAM("synonyms", "true");

  for (size_t i = 0; i < req.terms.size(); i++) {
    const ApiTerm& t = req.terms[i];
    if (t.term.GetLength() > 0) APPEND_PARAM("term", t.term);
    if (t.field.GetLength() > 0) APPEND_PARAM("field", t.field);
    if (t.weight.GetLength() > 0) APPEND_PARAM("weight", t.weight);
    if (t.phrase) APPEND_PARAM("phrase", "true");
  }

  for (size_t i = 0; i < req.doc_type_options.size(); i++) {
    if (req.doc_type_options[i].GetLength() > 0) {
      APPEND_PARAM("doc_type_option", req.doc_type_options[i]);
    }
  }

  if (req.element_set.GetLength() > 0) APPEND_PARAM("element_set", req.element_set);
  if (req.record_syntax.GetLength() > 0) APPEND_PARAM("record_syntax", req.record_syntax);
  if (req.highlight_prefix.GetLength() > 0) APPEND_PARAM("highlight_prefix", req.highlight_prefix);
  if (req.highlight_suffix.GetLength() > 0) APPEND_PARAM("highlight_suffix", req.highlight_suffix);
  if (req.byte_range) APPEND_PARAM("byte_range", "true");
  if (req.has_start_doc) APPEND_PARAM("start_doc", req.start_doc);
  if (req.has_end_doc) APPEND_PARAM("end_doc", req.end_doc);
  if (req.has_rect) {
    STRING rect;
    CHR rectbuf[256];
    snprintf(rectbuf, sizeof(rectbuf), "%g,%g,%g,%g",
             (double)req.rect_north, (double)req.rect_south,
             (double)req.rect_west, (double)req.rect_east);
    rect = rectbuf;
    APPEND_PARAM("rect", rect);
  }
  if (!req.include_url) APPEND_PARAM("include_url", "false");
  if (!req.include_headline) APPEND_PARAM("include_headline", "false");
  if (!req.include_record_key) APPEND_PARAM("include_record_key", "false");
  if (req.score_scale != 100) APPEND_PARAM("score_scale", req.score_scale);
  if (req.request_id.GetLength() > 0) APPEND_PARAM("request_id", req.request_id);

  APPEND_PARAM("start", start);
  APPEND_PARAM("max_hits", req.max_hits);
  #undef APPEND_PARAM
  return link;
}

static STRING NormalizePath(const CHR *raw_path)
{
  STRING path = raw_path != NULL ? raw_path : "";
  if (path.GetLength() == 0) {
    return "/";
  }
  if (path.Search("/v1/api") == 1) {
    path.EraseBefore(8);
    if (path.GetLength() == 0) {
      return "/";
    }
  }
  if (path.Search("/api/v1") == 1) {
    path.EraseBefore(8);
    if (path.GetLength() == 0) {
      return "/";
    }
  }
  return path;
}

static bool InAllowListNoCase(const STRLIST& allow_list, const STRING& value)
{
  const INT total = allow_list.GetTotalEntries();
  for (INT i = 1; i <= total; i++) {
    STRING entry;
    allow_list.GetEntry(i, &entry);
    if (entry.CaseEquals(value)) {
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  const CHR *method = getenv("REQUEST_METHOD");
  if (method == NULL || method[0] == '\0') {
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request", "REQUEST_METHOD is required.");
    return 0;
  }

  const CHR *accept_header = getenv("HTTP_ACCEPT");
  if (accept_header == NULL || accept_header[0] == '\0') {
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
  CHR *path_cstr = path.NewCString();
  STRING endpoint = ExtractPathParam(path_cstr, 1);
  delete [] path_cstr;
  const bool is_search_path = (path == "/search") || endpoint.CaseEquals("search");
  if (!is_search_path) {
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
    if (content_type == NULL || content_type[0] == '\0') {
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

  CGIAPP *cgi = NULL;
  if (StrCaseCmp(method, "GET") == 0) {
    cgi = new CGIAPP();
  }

  ApiRequest req;
  STRING parse_error;
  if (!ParseRequest(cgi, method, NULL, req, parse_error)) {
    if (cgi != NULL) delete cgi;
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request parameters", parse_error);
    return 0;
  }

  if (cfg.allow_list.GetTotalEntries() > 0 &&
      !InAllowListNoCase(cfg.allow_list, req.database)) {
    if (cgi != NULL) delete cgi;
    WriteHttpHeader(404, true);
    WriteProblem(404, "https://isearch.invalid/problems/not-found",
                 "Not found", "Requested database is not available.");
    return 0;
  }

  if (req.max_hits > cfg.max_hits_ceiling) {
    if (cgi != NULL) delete cgi;
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request parameters",
                 "max_hits exceeds configured API limit.");
    return 0;
  }

  ApiSearchMeta meta;
  std::vector<ApiHit> hits;
  STRING error_detail;
  const int status = ExecuteSearch(req, cfg, meta, hits, error_detail);
  if (cgi != NULL) delete cgi;

  if (status != 200) {
    WriteHttpHeader(status, true);
    WriteProblem(status, "https://isearch.invalid/problems/search-failed",
                 "Search failed", error_detail);
    return 0;
  }

  WriteHttpHeader(200, false);
  BeginSearchResponse(meta);
  INT response_start = req.start;
  if (req.has_start_doc) response_start = req.start_doc;
  for (size_t i = 0; i < hits.size(); i++) {
    WriteSearchHit((INT)(response_start + (INT)i), hits[i], i == 0);
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
