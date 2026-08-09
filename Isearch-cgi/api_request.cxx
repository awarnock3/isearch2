// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "api_request.hxx"

#include <climits>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>

#include "api_config.hxx"

ApiTerm::ApiTerm()
  : phrase(false)
{
}

ApiRequest::ApiRequest()
  : search_type(SEARCH_SIMPLE), op(OP_OR), element_set("B"), start(1),
    max_hits(API_DEFAULT_MAX_HITS), include_url(true),
    include_headline(true), include_record_key(true), score_scale(100)
{
}

static LONG g_request_counter = 0;

static bool IsMethod(const CHR *method, const CHR *target)
{
  if (method == nullptr || target == nullptr) {
    return false;
  }
  return StrCaseCmp(method, target) == 0;
}

static void SetError(STRING& error_detail, const CHR *message)
{
  error_detail = (message != nullptr) ? message : "Invalid request.";
}

static bool ParsePositiveInt(const CHR *raw, INT *value_out)
{
  if (raw == nullptr || raw[0] == '\0' || value_out == nullptr) {
    return false;
  }
  CHR *endptr = nullptr;
  const long parsed = strtol(raw, &endptr, 10);
  if (endptr == raw || *endptr != '\0' || parsed < 1) {
    return false;
  }
  // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_requesthxx): the same
  // defect as Isearch-cgi/api_config.cxx's own BUGFIX #1, but more
  // severely reachable here -- this parses start/max_hits/score_scale
  // directly from untrusted GET query parameters or POST JSON body,
  // not just server-side environment variables. Confirmed via a live
  // repro through the public ParseRequest() API: max_hits=4294967297
  // (2^32+1) truncated to max_hits=1 and passed ValidateRequest()'s
  // range checks completely unnoticed -- an attacker-supplied
  // out-of-range value silently aliased to a "valid-looking" one
  // instead of being rejected. Fixed by rejecting values that don't
  // fit in INT instead of truncating them.
  if (parsed > INT_MAX) {
    return false;
  }
  *value_out = (INT)parsed;
  return true;
}

static bool ParseBoolean(const CHR *raw, bool *value_out)
{
  if (raw == nullptr || value_out == nullptr) {
    return false;
  }

  STRING value = raw;
  value.Trim();
  if (value.CaseEquals("true") || value.CaseEquals("1") ||
      value.CaseEquals("yes") || value.CaseEquals("on")) {
    *value_out = true;
    return true;
  }
  if (value.CaseEquals("false") || value.CaseEquals("0") ||
      value.CaseEquals("no") || value.CaseEquals("off")) {
    *value_out = false;
    return true;
  }
  return false;
}

static const CHR *GetValue(CGIAPP *cgi, const CHR *primary, const CHR *alias)
{
  if (cgi == nullptr) {
    return nullptr;
  }

  PCHR value = nullptr;
  if (primary != nullptr && primary[0] != '\0') {
    value = cgi->GetValueByName(primary);
    if (value != nullptr && value[0] != '\0') {
      return value;
    }
  }
  if (alias != nullptr && alias[0] != '\0') {
    value = cgi->GetValueByName(alias);
    if (value != nullptr && value[0] != '\0') {
      return value;
    }
  }
  return nullptr;
}

static void GenerateRequestId(STRING *request_id)
{
  if (request_id == nullptr || request_id->GetLength() > 0) {
    return;
  }
  CHR idbuf[64];
  ++g_request_counter;
  snprintf(idbuf, sizeof(idbuf), "req-%ld-%ld",
           (LONG)time(nullptr), g_request_counter);
  *request_id = idbuf;
}

static bool ParseSearchType(const CHR *raw, SearchType *out)
{
  if (raw == nullptr || out == nullptr) {
    return true;
  }
  if (StrCaseCmp(raw, "simple") == 0) {
    *out = SEARCH_SIMPLE;
    return true;
  }
  if (StrCaseCmp(raw, "advanced") == 0) {
    *out = SEARCH_ADVANCED;
    return true;
  }
  if (StrCaseCmp(raw, "boolean") == 0) {
    *out = SEARCH_BOOLEAN;
    return true;
  }
  return false;
}

static bool ParseOperator(const CHR *raw, BoolOperator *out)
{
  if (raw == nullptr || out == nullptr) {
    return true;
  }
  if (StrCaseCmp(raw, "or") == 0) {
    *out = OP_OR;
    return true;
  }
  if (StrCaseCmp(raw, "and") == 0) {
    *out = OP_AND;
    return true;
  }
  if (StrCaseCmp(raw, "andnot") == 0) {
    *out = OP_ANDNOT;
    return true;
  }
  if (StrCaseCmp(raw, "near") == 0) {
    *out = OP_NEAR;
    return true;
  }
  return false;
}

static void AddGetTerms(CGIAPP *cgi, std::vector<ApiTerm> *terms_out)
{
  if (cgi == nullptr || terms_out == nullptr) {
    return;
  }

  const CHR *single_term = cgi->GetValueByName("term");
  if (single_term != nullptr && single_term[0] != '\0') {
    ApiTerm term;
    term.term = single_term;
    const CHR *single_field = cgi->GetValueByName("field");
    const CHR *single_weight = cgi->GetValueByName("weight");
    const CHR *single_phrase = cgi->GetValueByName("phrase");
    if (single_field != nullptr) term.field = single_field;
    if (single_weight != nullptr) term.weight = single_weight;
    if (single_phrase != nullptr) {
      bool phrase = false;
      if (ParseBoolean(single_phrase, &phrase)) {
        term.phrase = phrase;
      }
    }
    terms_out->push_back(term);
  }

  for (INT i = 1; i <= 64; i++) {
    CHR key[32];
    snprintf(key, sizeof(key), "TERM_%d", i);
    const CHR *term_value = cgi->GetValueByName(key);
    if (term_value == nullptr || term_value[0] == '\0') {
      continue;
    }

    ApiTerm term;
    term.term = term_value;

    snprintf(key, sizeof(key), "FIELD_%d", i);
    const CHR *field_value = cgi->GetValueByName(key);
    if (field_value != nullptr) {
      term.field = field_value;
    }

    snprintf(key, sizeof(key), "WEIGHT_%d", i);
    const CHR *weight_value = cgi->GetValueByName(key);
    if (weight_value != nullptr) {
      term.weight = weight_value;
    }

    snprintf(key, sizeof(key), "PHRASE_%d", i);
    const CHR *phrase_value = cgi->GetValueByName(key);
    if (phrase_value != nullptr) {
      bool phrase = false;
      if (ParseBoolean(phrase_value, &phrase) ||
          StrCaseCmp(phrase_value, "YES") == 0) {
        term.phrase = phrase;
      }
    }
    terms_out->push_back(term);
  }
}

static bool ValidateRequest(const ApiRequest& req, STRING& error_detail)
{
  if (req.database.GetLength() == 0) {
    SetError(error_detail, "Missing required parameter: database");
    return false;
  }
  if (req.q.GetLength() == 0 && req.terms.empty()) {
    SetError(error_detail, "Either q or at least one term is required.");
    return false;
  }
  if (req.start < 1) {
    SetError(error_detail, "start must be >= 1.");
    return false;
  }
  if (req.max_hits < 1) {
    SetError(error_detail, "max_hits must be >= 1.");
    return false;
  }
  if (req.max_hits > API_HARD_MAX_HITS) {
    SetError(error_detail, "max_hits exceeds configured hard limit.");
    return false;
  }
  if (req.score_scale < 1) {
    SetError(error_detail, "score_scale must be >= 1.");
    return false;
  }
  for (size_t i = 0; i < req.terms.size(); i++) {
    if (req.terms[i].term.GetLength() == 0) {
      SetError(error_detail, "Each term entry must include a non-empty term.");
      return false;
    }
  }
  return true;
}

class JsonReader {
public:
  explicit JsonReader(const CHR *text)
    : p(text != nullptr ? text : ""), pos(0), len(strlen(p))
  {
  }

  void SkipWs()
  {
    while (pos < len && isspace((unsigned char)p[pos])) {
      pos++;
    }
  }

  bool Consume(CHR c)
  {
    SkipWs();
    if (pos < len && p[pos] == c) {
      pos++;
      return true;
    }
    return false;
  }

  bool Peek(CHR c)
  {
    SkipWs();
    return pos < len && p[pos] == c;
  }

  bool ParseString(STRING *out)
  {
    if (out == nullptr) return false;
    SkipWs();
    if (pos >= len || p[pos] != '"') return false;
    pos++;

    std::string result;
    while (pos < len) {
      CHR c = p[pos++];
      if (c == '"') {
        *out = result.c_str();
        return true;
      }
      if (c == '\\') {
        if (pos >= len) return false;
        CHR e = p[pos++];
        switch (e) {
          case '"': result.push_back('"'); break;
          case '\\': result.push_back('\\'); break;
          case '/': result.push_back('/'); break;
          case 'b': result.push_back('\b'); break;
          case 'f': result.push_back('\f'); break;
          case 'n': result.push_back('\n'); break;
          case 'r': result.push_back('\r'); break;
          case 't': result.push_back('\t'); break;
          // BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgiapi_requesthxx):
          // this used to skip the 4 hex digits and push a literal '?'
          // placeholder instead of actually decoding them -- silently
          // corrupting every \uXXXX escape (a completely ordinary way
          // to encode an accented character, or even just a quote, in
          // JSON) into a question mark, while still reporting the
          // parse as successful. Confirmed with a live repro:
          // {"q":"café"} parsed to q="caf?" instead of "café".
          // This codebase is single-byte (ISO-8859-1) throughout, so
          // there's no lossless representation above 0xFF -- decode
          // 0x00-0xFF to the matching Latin-1 byte, and fail the parse
          // (loud, not silent corruption) for anything higher instead
          // of guessing.
          case 'u': {
            if (pos + 4 > len) return false;
            unsigned int code = 0;
            for (int k = 0; k < 4; k++) {
              CHR h = p[pos + k];
              code <<= 4;
              if (h >= '0' && h <= '9') code |= (unsigned int)(h - '0');
              else if (h >= 'a' && h <= 'f') code |= (unsigned int)(h - 'a' + 10);
              else if (h >= 'A' && h <= 'F') code |= (unsigned int)(h - 'A' + 10);
              else return false;
            }
            pos += 4;
            if (code > 0xFF) return false;
            result.push_back((char)code);
            break;
          }
          default:
            return false;
        }
      } else {
        result.push_back(c);
      }
    }
    return false;
  }

  bool ParseBool(bool *out)
  {
    if (out == nullptr) return false;
    SkipWs();
    if (pos + 4 <= len && strncmp(p + pos, "true", 4) == 0) {
      pos += 4;
      *out = true;
      return true;
    }
    if (pos + 5 <= len && strncmp(p + pos, "false", 5) == 0) {
      pos += 5;
      *out = false;
      return true;
    }
    return false;
  }

  bool ParseNumberToken(STRING *out)
  {
    if (out == nullptr) return false;
    SkipWs();
    size_t start = pos;
    if (pos < len && (p[pos] == '-' || p[pos] == '+')) pos++;
    bool have_digits = false;
    while (pos < len && isdigit((unsigned char)p[pos])) {
      have_digits = true;
      pos++;
    }
    if (pos < len && p[pos] == '.') {
      pos++;
      while (pos < len && isdigit((unsigned char)p[pos])) {
        have_digits = true;
        pos++;
      }
    }
    if (!have_digits) return false;
    if (pos < len && (p[pos] == 'e' || p[pos] == 'E')) {
      pos++;
      if (pos < len && (p[pos] == '+' || p[pos] == '-')) pos++;
      bool exp_digits = false;
      while (pos < len && isdigit((unsigned char)p[pos])) {
        exp_digits = true;
        pos++;
      }
      if (!exp_digits) return false;
    }
    *out = STRING(p + start, pos - start);
    return true;
  }

  bool AtEnd()
  {
    SkipWs();
    return pos >= len;
  }

private:
  const CHR *p;
  size_t pos;
  size_t len;
};

static bool ParseJsonTerms(JsonReader *jr, std::vector<ApiTerm> *terms_out,
                           STRING& error_detail)
{
  if (!jr->Consume('[')) {
    SetError(error_detail, "terms must be a JSON array.");
    return false;
  }
  if (jr->Consume(']')) {
    return true;
  }

  while (true) {
    if (!jr->Consume('{')) {
      SetError(error_detail, "Each terms entry must be a JSON object.");
      return false;
    }

    ApiTerm term;
    if (!jr->Consume('}')) {
      while (true) {
        STRING key;
        if (!jr->ParseString(&key)) {
          SetError(error_detail, "Invalid terms object key.");
          return false;
        }
        if (!jr->Consume(':')) {
          SetError(error_detail, "Invalid terms object syntax.");
          return false;
        }

        if (key.CaseEquals("term")) {
          if (!jr->ParseString(&term.term)) {
            SetError(error_detail, "terms[].term must be a string.");
            return false;
          }
        } else if (key.CaseEquals("field")) {
          if (!jr->ParseString(&term.field)) {
            SetError(error_detail, "terms[].field must be a string.");
            return false;
          }
        } else if (key.CaseEquals("weight")) {
          if (jr->Peek('"')) {
            if (!jr->ParseString(&term.weight)) {
              SetError(error_detail, "terms[].weight must be string or number.");
              return false;
            }
          } else {
            STRING token;
            if (!jr->ParseNumberToken(&token)) {
              SetError(error_detail, "terms[].weight must be string or number.");
              return false;
            }
            term.weight = token;
          }
        } else if (key.CaseEquals("phrase")) {
          bool phrase = false;
          if (!jr->ParseBool(&phrase)) {
            SetError(error_detail, "terms[].phrase must be boolean.");
            return false;
          }
          term.phrase = phrase;
        } else {
          SetError(error_detail, "Unknown field in terms[] entry.");
          return false;
        }

        if (jr->Consume('}')) break;
        if (!jr->Consume(',')) {
          SetError(error_detail, "Invalid terms object delimiter.");
          return false;
        }
      }
    }

    if (term.term.GetLength() == 0) {
      SetError(error_detail, "Each terms entry must include term.");
      return false;
    }
    terms_out->push_back(term);

    if (jr->Consume(']')) break;
    if (!jr->Consume(',')) {
      SetError(error_detail, "Invalid terms array delimiter.");
      return false;
    }
  }
  return true;
}

static bool ParsePostJson(const CHR *body, ApiRequest& out, STRING& error_detail)
{
  JsonReader jr(body);
  if (!jr.Consume('{')) {
    SetError(error_detail, "JSON body must be an object.");
    return false;
  }

  if (!jr.Consume('}')) {
    while (true) {
      STRING key;
      if (!jr.ParseString(&key)) {
        SetError(error_detail, "Invalid JSON object key.");
        return false;
      }
      if (!jr.Consume(':')) {
        SetError(error_detail, "Invalid JSON object syntax.");
        return false;
      }

      if (key.CaseEquals("database")) {
        if (!jr.ParseString(&out.database)) {
          SetError(error_detail, "database must be a string.");
          return false;
        }
      } else if (key.CaseEquals("q")) {
        if (!jr.ParseString(&out.q)) {
          SetError(error_detail, "q must be a string.");
          return false;
        }
      } else if (key.CaseEquals("search_type")) {
        STRING value;
        if (!jr.ParseString(&value) || !ParseSearchType(value, &out.search_type)) {
          SetError(error_detail, "search_type must be simple, advanced, or boolean.");
          return false;
        }
      } else if (key.CaseEquals("operator")) {
        STRING value;
        if (!jr.ParseString(&value) || !ParseOperator(value, &out.op)) {
          SetError(error_detail, "operator must be and, or, andnot, or near.");
          return false;
        }
      } else if (key.CaseEquals("terms")) {
        if (!ParseJsonTerms(&jr, &out.terms, error_detail)) {
          return false;
        }
      } else if (key.CaseEquals("element_set")) {
        if (!jr.ParseString(&out.element_set)) {
          SetError(error_detail, "element_set must be a string.");
          return false;
        }
      } else if (key.CaseEquals("start")) {
        STRING token;
        if (!jr.ParseNumberToken(&token) || !ParsePositiveInt(token, &out.start)) {
          SetError(error_detail, "start must be a positive integer.");
          return false;
        }
      } else if (key.CaseEquals("max_hits")) {
        STRING token;
        if (!jr.ParseNumberToken(&token) || !ParsePositiveInt(token, &out.max_hits)) {
          SetError(error_detail, "max_hits must be a positive integer.");
          return false;
        }
      } else if (key.CaseEquals("include_url")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "include_url must be boolean.");
          return false;
        }
        out.include_url = value;
      } else if (key.CaseEquals("include_headline")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "include_headline must be boolean.");
          return false;
        }
        out.include_headline = value;
      } else if (key.CaseEquals("include_record_key")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "include_record_key must be boolean.");
          return false;
        }
        out.include_record_key = value;
      } else if (key.CaseEquals("score_scale")) {
        STRING token;
        if (!jr.ParseNumberToken(&token) ||
            !ParsePositiveInt(token, &out.score_scale)) {
          SetError(error_detail, "score_scale must be a positive integer.");
          return false;
        }
      } else if (key.CaseEquals("request_id")) {
        if (!jr.ParseString(&out.request_id)) {
          SetError(error_detail, "request_id must be a string.");
          return false;
        }
      } else {
        SetError(error_detail, "Unsupported JSON field in request body.");
        return false;
      }

      if (jr.Consume('}')) break;
      if (!jr.Consume(',')) {
        SetError(error_detail, "Invalid JSON delimiter.");
        return false;
      }
    }
  }

  if (!jr.AtEnd()) {
    SetError(error_detail, "Unexpected trailing content in JSON body.");
    return false;
  }
  return true;
}

static std::string ReadStdinBody()
{
  std::string body;
  int c = 0;
  while ((c = getchar()) != EOF) {
    body.push_back((char)c);
  }
  return body;
}

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
      CHR a = (CHR)tolower((unsigned char)value[i + j]);
      CHR b = (CHR)tolower((unsigned char)needle[j]);
      if (a != b) break;
    }
    if (j == needle_len) return true;
  }
  return false;
}

static bool ParseGetRequest(CGIAPP *cgi, ApiRequest& out, STRING& error_detail)
{
  if (cgi == nullptr) {
    SetError(error_detail, "GET request requires CGI parameter parser.");
    return false;
  }

  const CHR *value = nullptr;

  value = GetValue(cgi, "database", "DATABASE");
  if (value != nullptr) out.database = value;

  value = GetValue(cgi, "q", "ISEARCH_TERM");
  if (value != nullptr) out.q = value;

  value = GetValue(cgi, "search_type", "SEARCH_TYPE");
  if (!ParseSearchType(value, &out.search_type)) {
    SetError(error_detail, "search_type must be simple, advanced, or boolean.");
    return false;
  }

  value = GetValue(cgi, "operator", "OPERATOR");
  if (!ParseOperator(value, &out.op)) {
    SetError(error_detail, "operator must be and, or, andnot, or near.");
    return false;
  }

  AddGetTerms(cgi, &out.terms);

  value = GetValue(cgi, "element_set", "ELEMENT_SET");
  if (value != nullptr && value[0] != '\0') out.element_set = value;

  value = GetValue(cgi, "start", "START");
  if (value != nullptr && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.start)) {
      SetError(error_detail, "start must be a positive integer.");
      return false;
    }
  }

  value = GetValue(cgi, "max_hits", "MAXHITS");
  if (value != nullptr && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.max_hits)) {
      SetError(error_detail, "max_hits must be a positive integer.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_url");
  if (value != nullptr && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_url)) {
      SetError(error_detail, "include_url must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_headline");
  if (value != nullptr && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_headline)) {
      SetError(error_detail, "include_headline must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_record_key");
  if (value != nullptr && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_record_key)) {
      SetError(error_detail, "include_record_key must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("score_scale");
  if (value != nullptr && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.score_scale)) {
      SetError(error_detail, "score_scale must be a positive integer.");
      return false;
    }
  }

  value = cgi->GetValueByName("request_id");
  if (value != nullptr && value[0] != '\0') out.request_id = value;

  return ValidateRequest(out, error_detail);
}

static bool ParsePostRequest(const CHR *body, ApiRequest& out, STRING& error_detail)
{
  const CHR *content_type = getenv("CONTENT_TYPE");
  if ((content_type == nullptr || content_type[0] == '\0')) {
    content_type = getenv("HTTP_CONTENT_TYPE");
  }
  if (!ContainsNoCase(content_type, "application/json")) {
    SetError(error_detail, "Unsupported Content-Type; expected application/json.");
    return false;
  }

  std::string stdin_body;
  if (body == nullptr) {
    stdin_body = ReadStdinBody();
    body = stdin_body.c_str();
  }

  if (body == nullptr || body[0] == '\0') {
    SetError(error_detail, "POST body is required.");
    return false;
  }

  if (!ParsePostJson(body, out, error_detail)) {
    return false;
  }
  return ValidateRequest(out, error_detail);
}

bool ParseRequest(CGIAPP* cgi, const CHR* method, const CHR* body,
                  ApiRequest& out, STRING& error_detail)
{
  error_detail = "";
  out = ApiRequest();

  const CHR *effective_method = method;
  if (effective_method == nullptr || effective_method[0] == '\0') {
    effective_method = getenv("REQUEST_METHOD");
  }
  if (effective_method == nullptr || effective_method[0] == '\0') {
    SetError(error_detail, "REQUEST_METHOD is required.");
    return false;
  }

  bool ok = false;
  if (IsMethod(effective_method, "GET")) {
    ok = ParseGetRequest(cgi, out, error_detail);
  } else if (IsMethod(effective_method, "POST")) {
    ok = ParsePostRequest(body, out, error_detail);
  } else {
    SetError(error_detail, "Unsupported method. Expected GET or POST.");
    return false;
  }

  if (!ok) {
    return false;
  }

  GenerateRequestId(&out.request_id);
  return true;
}
