// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

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
  if (name != nullptr && StrCaseCmp(name, "database") == 0) {
    const CHR *db_from_path = getenv("ISEARCH_API_DB_FROM_PATH");
    if (db_from_path != nullptr && db_from_path[0] != '\0') {
      return db_from_path;
    }
  }
  if (cgi == nullptr || name == nullptr) {
    return "";
  }
  const CHR *raw = cgi->GetValueByName(name);
  return (raw != nullptr) ? raw : "";
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
  if (body == nullptr || field == nullptr) {
    return "";
  }
  STRING needle = "\"";
  needle.Cat(field);
  needle.Cat("\"");
  CHR *needle_cstr = needle.NewCString();
  const CHR *start = strstr(body, needle_cstr);
  delete [] needle_cstr;
  if (start == nullptr) {
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
        case 'b':  result.Cat("\b"); break;
        case 'f':  result.Cat("\f"); break;
        case 'n':  result.Cat("\n"); break;
        case 'r':  result.Cat("\r"); break;
        case 't':  result.Cat("\t"); break;
        // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_fetchhxx-isearch-cgiapi_fetchcxx):
        // this had no 'u' case at all, so it fell into `default:`, which
        // appends the byte *after* the backslash literally and then keeps
        // walking the string one character at a time -- a `10`
        // escape (meant to decode to "10") came out as the literal text
        // "u0031u0030" instead, six garbage characters in place of two
        // real ones. Confirmed with a live repro through the real
        // isrch_api binary before this fix: a POST /fetch body with
        // record_key given as `"10"` (a \uXXXX-escaped "10")
        // failed to match the real record ("10") at all, returning 404
        // "Record not found" instead of the expected 200. Fixed the same
        // way as the identical gap in api_request.cxx's JsonReader
        // (BUGFIX #2 there): decode 0x00-0xFF to the matching Latin-1
        // byte (this codebase is single-byte/ISO-8859-1 throughout, per
        // every CGI file's own Content-Type declaration). Unlike
        // JsonReader, this function has no way to fail the parse and
        // report an error back to its caller (it just returns a STRING),
        // so a codepoint above 0xFF -- for which there's no lossless
        // single-byte representation -- is silently dropped (nothing
        // appended) rather than guessed at with a placeholder; still
        // strictly better than emitting garbage that looks like real
        // (wrong) data.
        case 'u': {
          if (*(start + 1) && *(start + 2) && *(start + 3) && *(start + 4)) {
            unsigned int code = 0;
            bool valid = true;
            for (int k = 1; k <= 4; k++) {
              const CHR h = *(start + k);
              code <<= 4;
              if (h >= '0' && h <= '9') code |= (unsigned int)(h - '0');
              else if (h >= 'a' && h <= 'f') code |= (unsigned int)(h - 'a' + 10);
              else if (h >= 'A' && h <= 'F') code |= (unsigned int)(h - 'A' + 10);
              else { valid = false; break; }
            }
            if (valid) {
              start += 4;
              if (code <= 0xFF) {
                result.Cat((UCHR)code);
              }
              break;
            }
          }
          result.Cat(*start);
          break;
        }
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

  const bool is_post = (method != nullptr && StrCaseCmp(method, "POST") == 0);

  if (!is_post) {
    // Reject unknown GET parameters so typos are caught.
    static const CHR *const known_params[] = {
      "database", "record_key", "element_set", "record_syntax", "request_id",
      nullptr
    };
    if (cgi != nullptr) {
      for (INT4 i = 0; ; i++) {
        const CHR *name = cgi->GetName(i);
        if (name == nullptr) break;
        if (name[0] == '\0') continue;
        bool found = false;
        for (int k = 0; known_params[k] != nullptr; k++) {
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
    if (body == nullptr) {
      stdin_body = ReadStdinBody();
      body = stdin_body.c_str();
    }
    if (body == nullptr || body[0] == '\0') {
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
  if (pdb == nullptr) {
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
