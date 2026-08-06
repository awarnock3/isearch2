#include "api_fetch.hxx"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <string>

#include "api_response.hxx"
#include "cgi-util.hxx"
#include "defs.hxx"
#include "vidb.hxx"
#include "result.hxx"

ApiFetchRequest::ApiFetchRequest()
  : element_set("F"), record_syntax("SUTRS") {}

ApiFetchResult::ApiFetchResult() {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static STRING GetParam(CGIAPP* cgi, const CHR* name)
{
  if (cgi == NULL || name == NULL) {
    return "";
  }
  const CHR *raw = cgi->GetValueByName(name);
  return (raw != NULL) ? raw : "";
}

static std::string ReadStdinBody()
{
  std::string body;
  int c;
  while ((c = fgetc(stdin)) != EOF) {
    body.push_back((char)c);
  }
  return body;
}

static STRING GetJsonField(const CHR* body, const CHR* field)
{
  if (body == NULL || field == NULL) {
    return "";
  }
  STRING needle = "\"";
  needle.Cat(field);
  needle.Cat("\"");
  CHR *needle_cstr = needle.NewCString();
  const CHR *start = strstr(body, needle_cstr);
  delete [] needle_cstr;
  if (start == NULL) {
    return "";
  }
  start += needle.GetLength();
  while (*start == ' ' || *start == '\t' || *start == ':') ++start;
  if (*start != '"') {
    return "";
  }
  ++start;
  STRING result;
  while (*start && *start != '"') {
    if (*start == '\\' && *(start + 1)) {
      ++start;
      switch (*start) {
        case '"':  result.Cat("\""); break;
        case '\\': result.Cat("\\"); break;
        case '/':  result.Cat("/"); break;
        case 'n':  result.Cat("\n"); break;
        case 'r':  result.Cat("\r"); break;
        case 't':  result.Cat("\t"); break;
        default:   result.Cat(*start); break;
      }
    } else {
      result.Cat(*start);
    }
    ++start;
  }
  return result;
}

// ---------------------------------------------------------------------------
// Request parsing
// ---------------------------------------------------------------------------

bool ParseFetchRequest(CGIAPP* cgi, const CHR* method, const CHR* body,
                       ApiFetchRequest& out, STRING& error_detail)
{
  out = ApiFetchRequest();

  const bool is_post = (method != NULL && StrCaseCmp(method, "POST") == 0);

  if (!is_post) {
    // Reject unknown GET parameters so typos are caught.
    static const CHR *const known_params[] = {
      "database", "record_key", "element_set", "record_syntax", "request_id",
      NULL
    };
    if (cgi != NULL) {
      for (INT4 i = 0; ; i++) {
        const CHR *name = cgi->GetName(i);
        if (name == NULL) break;
        if (name[0] == '\0') continue;
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
          error_detail = msg_cstr;
          delete [] msg_cstr;
          return false;
        }
      }
    }
  }

  if (is_post) {
    std::string stdin_body;
    if (body == NULL) {
      stdin_body = ReadStdinBody();
      body = stdin_body.c_str();
    }
    if (body == NULL || body[0] == '\0') {
      error_detail = "Request body is empty.";
      return false;
    }
    out.database      = GetJsonField(body, "database");
    out.record_key    = GetJsonField(body, "record_key");
    out.element_set   = GetJsonField(body, "element_set");
    out.record_syntax = GetJsonField(body, "record_syntax");
    out.request_id    = GetJsonField(body, "request_id");
  } else {
    out.database      = GetParam(cgi, "database");
    out.record_key    = GetParam(cgi, "record_key");
    out.element_set   = GetParam(cgi, "element_set");
    out.record_syntax = GetParam(cgi, "record_syntax");
    out.request_id    = GetParam(cgi, "request_id");
  }

  if (out.database.GetLength() == 0) {
    error_detail = "Missing required parameter: database";
    return false;
  }
  if (out.record_key.GetLength() == 0) {
    error_detail = "Missing required parameter: record_key";
    return false;
  }

  if (out.element_set.GetLength() == 0) {
    out.element_set = "F";
  }
  if (out.record_syntax.GetLength() == 0) {
    out.record_syntax = "SUTRS";
  }

  if (!out.record_syntax.CaseEquals("HTML") &&
      !out.record_syntax.CaseEquals("SUTRS")) {
    error_detail = "record_syntax must be SUTRS or HTML.";
    return false;
  }

  return true;
}

// ---------------------------------------------------------------------------
// Fetch execution
// ---------------------------------------------------------------------------

int ExecuteFetch(const ApiFetchRequest& req, const ApiConfig& cfg,
                 ApiFetchResult& result, STRING& error_detail)
{
  result = ApiFetchResult();

  STRING db_path = cfg.db_path;
  if (db_path.GetLength() == 0) {
    db_path = ".";
  }

  VIDB *pdb = new VIDB(db_path, req.database);
  if (pdb == NULL) {
    error_detail = "Failed to open database.";
    return 500;
  }

  if (pdb->GetTotalRecords() <= 0) {
    error_detail = "Database does not exist or is corrupted.";
    delete pdb;
    return 404;
  }

  RESULT rs_record;
  pdb->KeyLookup(req.record_key, &rs_record);

  STRING filename;
  rs_record.GetFileName(&filename);
  if (filename.GetLength() == 0) {
    error_detail = "Record not found for the supplied record_key.";
    delete pdb;
    return 404;
  }

  STRING record_syntax_str = req.record_syntax;
  if (record_syntax_str.CaseEquals("SUTRS")) {
    record_syntax_str = SutrsRecordSyntax;
  } else {
    record_syntax_str = HtmlRecordSyntax;
  }

  STRING content;
  pdb->Present(rs_record, req.element_set, record_syntax_str, &content);

  result.record_key = req.record_key;
  result.database   = req.database;
  result.filename   = filename;
  result.content    = content;

  delete pdb;
  return 200;
}
