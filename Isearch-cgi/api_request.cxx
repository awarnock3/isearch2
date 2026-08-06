#include "api_request.hxx"

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
  : search_type(SEARCH_SIMPLE), op(OP_OR), element_set("B"),
    record_syntax("HTML"), start(1), max_hits(API_DEFAULT_MAX_HITS), include_url(true),
    include_headline(true), include_record_key(true), score_scale(100)
{
}

static LONG g_request_counter = 0;

static bool IsMethod(const CHR *method, const CHR *target)
{
  if (method == NULL || target == NULL) {
    return false;
  }
  return StrCaseCmp(method, target) == 0;
}

static bool ParseRecordSyntax(const CHR *raw, STRING *out)
{
  if (raw == NULL || out == NULL) {
    return true;
  }
  if (StrCaseCmp(raw, "HTML") == 0) {
    *out = "HTML";
    return true;
  }
  if (StrCaseCmp(raw, "SUTRS") == 0) {
    *out = "SUTRS";
    return true;
  }
  return false;
}

static void SetError(STRING& error_detail, const CHR *message)
{
  error_detail = (message != NULL) ? message : "Invalid request.";
}

static bool ParsePositiveInt(const CHR *raw, INT *value_out)
{
  if (raw == NULL || raw[0] == '\0' || value_out == NULL) {
    return false;
  }
  CHR *endptr = NULL;
  const long parsed = strtol(raw, &endptr, 10);
  if (endptr == raw || *endptr != '\0' || parsed < 1) {
    return false;
  }
  *value_out = (INT)parsed;
  return true;
}

static bool ParseBoolean(const CHR *raw, bool *value_out)
{
  if (raw == NULL || value_out == NULL) {
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
  if (cgi == NULL) {
    return NULL;
  }

  PCHR value = NULL;
  if (primary != NULL && primary[0] != '\0') {
    value = cgi->GetValueByName(primary);
    if (value != NULL && value[0] != '\0') {
      return value;
    }
  }
  if (alias != NULL && alias[0] != '\0') {
    value = cgi->GetValueByName(alias);
    if (value != NULL && value[0] != '\0') {
      return value;
    }
  }
  return NULL;
}

static void GenerateRequestId(STRING *request_id)
{
  if (request_id == NULL || request_id->GetLength() > 0) {
    return;
  }
  CHR idbuf[64];
  ++g_request_counter;
  snprintf(idbuf, sizeof(idbuf), "req-%ld-%ld",
           (LONG)time(NULL), g_request_counter);
  *request_id = idbuf;
}

static bool ParseSearchType(const CHR *raw, SearchType *out)
{
  if (raw == NULL || out == NULL) {
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
  if (raw == NULL || out == NULL) {
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
  if (cgi == NULL || terms_out == NULL) {
    return;
  }

  const CHR *single_term = cgi->GetValueByName("term");
  if (single_term != NULL && single_term[0] != '\0') {
    ApiTerm term;
    term.term = single_term;
    const CHR *single_field = cgi->GetValueByName("field");
    const CHR *single_weight = cgi->GetValueByName("weight");
    const CHR *single_phrase = cgi->GetValueByName("phrase");
    if (single_field != NULL) term.field = single_field;
    if (single_weight != NULL) term.weight = single_weight;
    if (single_phrase != NULL) {
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
    if (term_value == NULL || term_value[0] == '\0') {
      continue;
    }

    ApiTerm term;
    term.term = term_value;

    snprintf(key, sizeof(key), "FIELD_%d", i);
    const CHR *field_value = cgi->GetValueByName(key);
    if (field_value != NULL) {
      term.field = field_value;
    }

    snprintf(key, sizeof(key), "WEIGHT_%d", i);
    const CHR *weight_value = cgi->GetValueByName(key);
    if (weight_value != NULL) {
      term.weight = weight_value;
    }

    snprintf(key, sizeof(key), "PHRASE_%d", i);
    const CHR *phrase_value = cgi->GetValueByName(key);
    if (phrase_value != NULL) {
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
    : p(text != NULL ? text : ""), pos(0), len(strlen(p))
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
    if (out == NULL) return false;
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
          case 'u':
            if (pos + 4 > len) return false;
            pos += 4;
            result.push_back('?');
            break;
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
    if (out == NULL) return false;
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
    if (out == NULL) return false;
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
      } else if (key.CaseEquals("record_syntax") || key.CaseEquals("RecordSyntax")) {
        STRING value;
        if (!jr.ParseString(&value) ||
            !ParseRecordSyntax(value, &value)) {
          SetError(error_detail, "record_syntax must be HTML or SUTRS.");
          return false;
        }
        out.record_syntax = value;
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
  if (cgi == NULL) {
    SetError(error_detail, "GET request requires CGI parameter parser.");
    return false;
  }

  const CHR *value = NULL;

  value = GetValue(cgi, "database", "DATABASE");
  if (value != NULL) out.database = value;

  value = GetValue(cgi, "q", "ISEARCH_TERM");
  if (value != NULL) out.q = value;

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
  if (value != NULL && value[0] != '\0') out.element_set = value;

  value = GetValue(cgi, "record_syntax", "RecordSyntax");
  if (value != NULL && value[0] != '\0') {
    if (!ParseRecordSyntax(value, &out.record_syntax)) {
      SetError(error_detail, "record_syntax must be HTML or SUTRS.");
      return false;
    }
  }

  value = GetValue(cgi, "start", "START");
  if (value != NULL && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.start)) {
      SetError(error_detail, "start must be a positive integer.");
      return false;
    }
  }

  value = GetValue(cgi, "max_hits", "MAXHITS");
  if (value != NULL && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.max_hits)) {
      SetError(error_detail, "max_hits must be a positive integer.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_url");
  if (value != NULL && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_url)) {
      SetError(error_detail, "include_url must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_headline");
  if (value != NULL && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_headline)) {
      SetError(error_detail, "include_headline must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("include_record_key");
  if (value != NULL && value[0] != '\0') {
    if (!ParseBoolean(value, &out.include_record_key)) {
      SetError(error_detail, "include_record_key must be boolean.");
      return false;
    }
  }

  value = cgi->GetValueByName("score_scale");
  if (value != NULL && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.score_scale)) {
      SetError(error_detail, "score_scale must be a positive integer.");
      return false;
    }
  }

  value = cgi->GetValueByName("request_id");
  if (value != NULL && value[0] != '\0') out.request_id = value;

  return ValidateRequest(out, error_detail);
}

static bool ParsePostRequest(const CHR *body, ApiRequest& out, STRING& error_detail)
{
  const CHR *content_type = getenv("CONTENT_TYPE");
  if ((content_type == NULL || content_type[0] == '\0')) {
    content_type = getenv("HTTP_CONTENT_TYPE");
  }
  if (!ContainsNoCase(content_type, "application/json")) {
    SetError(error_detail, "Unsupported Content-Type; expected application/json.");
    return false;
  }

  std::string stdin_body;
  if (body == NULL) {
    stdin_body = ReadStdinBody();
    body = stdin_body.c_str();
  }

  if (body == NULL || body[0] == '\0') {
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
  if (effective_method == NULL || effective_method[0] == '\0') {
    effective_method = getenv("REQUEST_METHOD");
  }
  if (effective_method == NULL || effective_method[0] == '\0') {
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
