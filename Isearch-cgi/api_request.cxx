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
    rpn(false), infix(false), and_mode(false), synonyms(false),
    record_syntax("HTML"), byte_range(false), start(1), max_hits(API_DEFAULT_MAX_HITS),
    start_doc(1), end_doc(1), has_start_doc(false), has_end_doc(false), has_rect(false),
    rect_north(0.0), rect_south(0.0), rect_west(0.0), rect_east(0.0), include_url(true),
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
  if (StrCaseCmp(raw, "TEXT") == 0 || StrCaseCmp(raw, "SUTRS") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.101") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.109") == 0) {
    *out = "SUTRS";
    return true;
  }
  if (StrCaseCmp(raw, "USMARC") == 0 || StrCaseCmp(raw, "1.2.840.10003.5.10") == 0) {
    *out = "USMARC";
    return true;
  }
  if (StrCaseCmp(raw, "HTML") == 0) {
    *out = "HTML";
    return true;
  }
  if (StrCaseCmp(raw, "1.2.840.10003.5.109.3") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.108") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.1000.34.1") == 0) {
    *out = "HTML";
    return true;
  }
  if (StrCaseCmp(raw, "SGML") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.109.9") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.1000.34.2") == 0) {
    *out = "SGML";
    return true;
  }
  if (StrCaseCmp(raw, "XML") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.109.10") == 0) {
    *out = "XML";
    return true;
  }
  if (StrCaseCmp(raw, "GRS-1") == 0 ||
      StrCaseCmp(raw, "1.2.840.10003.5.105") == 0) {
    *out = "GRS-1";
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

static bool ParseDouble(const CHR *raw, DOUBLE *value_out)
{
  if (raw == NULL || raw[0] == '\0' || value_out == NULL) {
    return false;
  }
  CHR *endptr = NULL;
  const double parsed = strtod(raw, &endptr);
  if (endptr == raw || *endptr != '\0') {
    return false;
  }
  *value_out = (DOUBLE)parsed;
  return true;
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
  if (req.rpn && req.infix) {
    SetError(error_detail, "rpn and infix cannot both be true.");
    return false;
  }
  if (req.rpn && req.and_mode) {
    SetError(error_detail, "rpn and and_mode cannot both be true.");
    return false;
  }
  if (req.infix && req.and_mode) {
    SetError(error_detail, "infix and and_mode cannot both be true.");
    return false;
  }
  if (req.has_start_doc && req.start_doc < 1) {
    SetError(error_detail, "start_doc must be >= 1.");
    return false;
  }
  if (req.has_end_doc && req.end_doc < 1) {
    SetError(error_detail, "end_doc must be >= 1.");
    return false;
  }
  if (req.has_start_doc && req.has_end_doc && req.end_doc < req.start_doc) {
    SetError(error_detail, "end_doc must be >= start_doc.");
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
      } else if (key.CaseEquals("rpn")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "rpn must be boolean.");
          return false;
        }
        out.rpn = value;
      } else if (key.CaseEquals("infix")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "infix must be boolean.");
          return false;
        }
        out.infix = value;
      } else if (key.CaseEquals("and_mode")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "and_mode must be boolean.");
          return false;
        }
        out.and_mode = value;
      } else if (key.CaseEquals("synonyms")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "synonyms must be boolean.");
          return false;
        }
        out.synonyms = value;
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
          SetError(error_detail, "record_syntax must be SUTRS or HTML.");
          return false;
        }
        out.record_syntax = value;
      } else if (key.CaseEquals("doc_type_options")) {
        if (!jr.Consume('[')) {
          SetError(error_detail, "doc_type_options must be an array.");
          return false;
        }
        if (!jr.Consume(']')) {
          while (true) {
            STRING opt;
            if (!jr.ParseString(&opt)) {
              SetError(error_detail, "doc_type_options entries must be strings.");
              return false;
            }
            out.doc_type_options.push_back(opt);
            if (jr.Consume(']')) break;
            if (!jr.Consume(',')) {
              SetError(error_detail, "Invalid doc_type_options delimiter.");
              return false;
            }
          }
        }
      } else if (key.CaseEquals("highlight_prefix")) {
        if (!jr.ParseString(&out.highlight_prefix)) {
          SetError(error_detail, "highlight_prefix must be a string.");
          return false;
        }
      } else if (key.CaseEquals("highlight_suffix")) {
        if (!jr.ParseString(&out.highlight_suffix)) {
          SetError(error_detail, "highlight_suffix must be a string.");
          return false;
        }
      } else if (key.CaseEquals("byte_range")) {
        bool value = false;
        if (!jr.ParseBool(&value)) {
          SetError(error_detail, "byte_range must be boolean.");
          return false;
        }
        out.byte_range = value;
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
      } else if (key.CaseEquals("start_doc")) {
        STRING token;
        if (!jr.ParseNumberToken(&token) || !ParsePositiveInt(token, &out.start_doc)) {
          SetError(error_detail, "start_doc must be a positive integer.");
          return false;
        }
        out.has_start_doc = true;
      } else if (key.CaseEquals("end_doc")) {
        STRING token;
        if (!jr.ParseNumberToken(&token) || !ParsePositiveInt(token, &out.end_doc)) {
          SetError(error_detail, "end_doc must be a positive integer.");
          return false;
        }
        out.has_end_doc = true;
      } else if (key.CaseEquals("rect")) {
        STRING token;
        if (!jr.Consume('[')) {
          SetError(error_detail, "rect must be an array of four numbers.");
          return false;
        }
        DOUBLE values[4];
        for (INT i = 0; i < 4; i++) {
          if (!jr.ParseNumberToken(&token) || !ParseDouble(token, &values[i])) {
            SetError(error_detail, "rect must contain four numeric values.");
            return false;
          }
          if (i < 3 && !jr.Consume(',')) {
            SetError(error_detail, "rect must contain four numeric values.");
            return false;
          }
        }
        if (!jr.Consume(']')) {
          SetError(error_detail, "rect must contain exactly four values.");
          return false;
        }
        out.has_rect = true;
        out.rect_north = values[0];
        out.rect_south = values[1];
        out.rect_west = values[2];
        out.rect_east = values[3];
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

static bool PathProvidesDatabase(STRING *database_out)
{
  if (database_out == NULL) {
    return false;
  }

  const CHR *path_info = getenv("PATH_INFO");
  if (path_info == NULL || path_info[0] == '\0') {
    return false;
  }

  STRING endpoint = ExtractPathParam(path_info, 1);
  STRING database = ExtractPathParam(path_info, 0);

  if (!(endpoint.CaseEquals("search") || endpoint.CaseEquals("fetch"))) {
    // Support prefixed forms like /v1/api/{database}/search and /api/v1/{database}/search.
    for (INT i = 2; i <= 5; i++) {
      endpoint = ExtractPathParam(path_info, i);
      if (endpoint.CaseEquals("search") || endpoint.CaseEquals("fetch")) {
        database = ExtractPathParam(path_info, i - 1);
        break;
      }
    }
  }

  if (!(endpoint.CaseEquals("search") || endpoint.CaseEquals("fetch")) ||
      database.GetLength() == 0) {
    return false;
  }

  *database_out = database;
  return true;
}

static bool ParseGetRequest(CGIAPP *cgi, ApiRequest& out, STRING& error_detail)
{
  if (cgi == NULL) {
    SetError(error_detail, "GET request requires CGI parameter parser.");
    return false;
  }

  // Reject unknown parameter names so typos (e.g. max_hist) are caught.
  static const CHR *const known_params[] = {
    "database", "DATABASE",
    "q", "ISEARCH_TERM",
    "search_type", "SEARCH_TYPE",
    "operator", "OPERATOR",
    "term", "field", "weight", "phrase",
    "element_set", "ELEMENT_SET",
    "record_syntax", "RecordSyntax",
    "start", "START",
    "max_hits", "MAXHITS",
    "include_url",
    "include_headline",
    "include_record_key",
    "score_scale",
    "request_id",
    NULL
  };

  for (INT4 i = 0; ; i++) {
    const CHR *name = cgi->GetName(i);
    if (name == NULL) break;
    if (name[0] == '\0') continue;

    // Skip indexed term/field/weight/phrase (term_0, field_1, …)
    if (strncasecmp(name, "term",   4) == 0 ||
        strncasecmp(name, "field",  5) == 0 ||
        strncasecmp(name, "weight", 6) == 0 ||
        strncasecmp(name, "phrase", 6) == 0) {
      continue;
    }

    bool found = false;
    for (int k = 0; known_params[k] != NULL; k++) {
      if (StrCaseCmp(name, known_params[k]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      STRING msg = "Unknown parameter: ";
      msg.Cat(name);
      CHR *msg_cstr = msg.NewCString();
      SetError(error_detail, msg_cstr);
      delete [] msg_cstr;
      return false;
    }
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
  value = GetValue(cgi, "rpn", "RPN");
  if (value != NULL && value[0] != '\0' && !ParseBoolean(value, &out.rpn)) {
    SetError(error_detail, "rpn must be boolean.");
    return false;
  }
  value = GetValue(cgi, "infix", "INFIX");
  if (value != NULL && value[0] != '\0' && !ParseBoolean(value, &out.infix)) {
    SetError(error_detail, "infix must be boolean.");
    return false;
  }
  value = GetValue(cgi, "and_mode", "AND");
  if (value != NULL && value[0] != '\0' && !ParseBoolean(value, &out.and_mode)) {
    SetError(error_detail, "and_mode must be boolean.");
    return false;
  }
  value = GetValue(cgi, "synonyms", "SYN");
  if (value != NULL && value[0] != '\0' && !ParseBoolean(value, &out.synonyms)) {
    SetError(error_detail, "synonyms must be boolean.");
    return false;
  }

  AddGetTerms(cgi, &out.terms);

  value = GetValue(cgi, "element_set", "ELEMENT_SET");
  if (value != NULL && value[0] != '\0') out.element_set = value;

  value = GetValue(cgi, "record_syntax", "RecordSyntax");
  if (value != NULL && value[0] != '\0') {
    if (!ParseRecordSyntax(value, &out.record_syntax)) {
      SetError(error_detail, "record_syntax must be SUTRS or HTML.");
      return false;
    }
  }
  for (INT i = 1; i <= 32; i++) {
    CHR key[32];
    snprintf(key, sizeof(key), "doc_type_option_%d", i);
    value = cgi->GetValueByName(key);
    if (value != NULL && value[0] != '\0') {
      out.doc_type_options.push_back(STRING(value));
    }
    snprintf(key, sizeof(key), "OPTION_%d", i);
    value = cgi->GetValueByName(key);
    if (value != NULL && value[0] != '\0') {
      out.doc_type_options.push_back(STRING(value));
    }
  }
  value = GetValue(cgi, "highlight_prefix", "PREFIX");
  if (value != NULL && value[0] != '\0') out.highlight_prefix = value;
  value = GetValue(cgi, "highlight_suffix", "SUFFIX");
  if (value != NULL && value[0] != '\0') out.highlight_suffix = value;
  value = GetValue(cgi, "byte_range", "BYTERANGE");
  if (value != NULL && value[0] != '\0' && !ParseBoolean(value, &out.byte_range)) {
    SetError(error_detail, "byte_range must be boolean.");
    return false;
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
  value = GetValue(cgi, "start_doc", "STARTDOC");
  if (value != NULL && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.start_doc)) {
      SetError(error_detail, "start_doc must be a positive integer.");
      return false;
    }
    out.has_start_doc = true;
  }
  value = GetValue(cgi, "end_doc", "ENDDOC");
  if (value != NULL && value[0] != '\0') {
    if (!ParsePositiveInt(value, &out.end_doc)) {
      SetError(error_detail, "end_doc must be a positive integer.");
      return false;
    }
    out.has_end_doc = true;
  }
  value = cgi->GetValueByName("rect");
  if (value != NULL && value[0] != '\0') {
    CHR *buf = strdup(value);
    if (buf == NULL) {
      SetError(error_detail, "Failed to parse rect.");
      return false;
    }
    CHR *save = NULL;
    CHR *tok = strtok_r(buf, ",", &save);
    DOUBLE vals[4];
    INT idx = 0;
    while (tok != NULL && idx < 4) {
      STRING t = tok;
      t.Trim();
      if (!ParseDouble(t, &vals[idx])) {
        free(buf);
        SetError(error_detail, "rect must contain four numeric values.");
        return false;
      }
      idx++;
      tok = strtok_r(NULL, ",", &save);
    }
    if (idx != 4 || tok != NULL) {
      free(buf);
      SetError(error_detail, "rect must contain exactly four values.");
      return false;
    }
    free(buf);
    out.has_rect = true;
    out.rect_north = vals[0];
    out.rect_south = vals[1];
    out.rect_west = vals[2];
    out.rect_east = vals[3];
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

  if (out.database.GetLength() == 0) {
    PathProvidesDatabase(&out.database);
  }

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

  if (out.database.GetLength() == 0) {
    PathProvidesDatabase(&out.database);
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
