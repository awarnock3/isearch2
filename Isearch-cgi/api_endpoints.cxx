#include "api_endpoints.hxx"

#include <dirent.h>
#include <string.h>

#include <vector>

#include "api_fetch.hxx"
#include "api_response.hxx"
#include "cgi-util.hxx"

static bool HasSuffix(const CHR *value, const CHR *suffix)
{
  if (value == NULL || suffix == NULL) {
    return false;
  }
  const size_t value_len = strlen(value);
  const size_t suffix_len = strlen(suffix);
  if (suffix_len > value_len) {
    return false;
  }
  return strcmp(value + (value_len - suffix_len), suffix) == 0;
}

void HandleHealth()
{
  WriteHttpHeader(200, false);
  cout << "{";
  cout << "\"status\":\"ok\",";
  cout << "\"version\":\"" << API_VERSION << "\"";
  cout << "}" << endl;
}

void HandleCapabilities(const ApiConfig& cfg)
{
  WriteHttpHeader(200, false);
  cout << "{";
  cout << "\"api_version\":\"" << API_VERSION << "\",";
  cout << "\"search_types\":[\"simple\",\"advanced\",\"boolean\"],";
  cout << "\"operators\":[\"or\",\"and\",\"andnot\",\"near\"],";
  cout << "\"element_sets\":[\"B\",\"F\",\"S\"],";
  cout << "\"record_syntaxes\":[\"HTML\",\"SUTRS\"],";
  cout << "\"defaults\":{";
  cout << "\"search_type\":\"simple\",";
  cout << "\"operator\":\"or\",";
  cout << "\"element_set\":\"B\",";
  cout << "\"record_syntax\":\"SUTRS\",";
  cout << "\"start\":1,";
  cout << "\"max_hits\":" << API_DEFAULT_MAX_HITS << ",";
  cout << "\"score_scale\":100";
  cout << "},";
  cout << "\"limits\":{";
  cout << "\"max_hits_ceiling\":" << cfg.max_hits_ceiling;
  cout << "}";
  cout << "}" << endl;
}

void HandleDatabases(const ApiConfig& cfg)
{
  if (cfg.db_path.GetLength() == 0) {
    WriteHttpHeader(501, true);
    WriteProblem(501,
                 "https://isearch.invalid/problems/databases-not-configured",
                 "Databases endpoint unavailable",
                 "ISEARCH_DB_PATH is not configured.");
    return;
  }

  CHR *db_path = cfg.db_path.NewCString();
  DIR *dir = opendir(db_path);
  if (dir == NULL) {
    delete [] db_path;
    WriteHttpHeader(500, true);
    WriteProblem(500,
                 "https://isearch.invalid/problems/databases-open-failed",
                 "Unable to list databases",
                 "Failed to open configured database path.");
    return;
  }

  std::vector<STRING> names;
  for (dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
    if (!HasSuffix(entry->d_name, ".mdt")) {
      continue;
    }
    STRING stem = entry->d_name;
    const STRINGINDEX len = stem.GetLength();
    if (len > 4) {
      stem.EraseAfter(len - 4);
      names.push_back(stem);
    }
  }
  closedir(dir);
  delete [] db_path;

  WriteHttpHeader(200, false);
  cout << "{";
  cout << "\"databases\":[";
  for (size_t i = 0; i < names.size(); i++) {
    if (i > 0) {
      cout << ",";
    }
    WriteJsonEscaped(names[i]);
  }
  cout << "]";
  cout << "}" << endl;
}

void HandleFetch(const ApiConfig& cfg, const CHR* method, const CHR* body)
{
  CGIAPP *cgi = NULL;
  if (method != NULL && StrCaseCmp(method, "GET") == 0) {
    cgi = new CGIAPP();
  }

  ApiFetchRequest req;
  STRING parse_error;
  if (!ParseFetchRequest(cgi, method, body, req, parse_error)) {
    if (cgi != NULL) delete cgi;
    WriteHttpHeader(400, true);
    WriteProblem(400, "https://isearch.invalid/problems/invalid-request",
                 "Invalid request parameters", parse_error);
    return;
  }

  ApiFetchResult result;
  STRING error_detail;
  const int status = ExecuteFetch(req, cfg, result, error_detail);
  if (cgi != NULL) delete cgi;

  if (status != 200) {
    const CHR *problem_type = (status == 404)
      ? "https://isearch.invalid/problems/record-not-found"
      : "https://isearch.invalid/problems/fetch-failed";
    WriteHttpHeader(status, true);
    WriteProblem(status, problem_type, "Fetch failed", error_detail);
    return;
  }

  WriteHttpHeader(200, false);
  cout << "{";
  cout << "\"record_key\":";
  WriteJsonEscaped(result.record_key);
  cout << ",\"database\":";
  WriteJsonEscaped(result.database);
  cout << ",\"filename\":";
  WriteJsonEscaped(result.filename);
  cout << ",\"content\":";
  WriteJsonEscaped(result.content);
  cout << "}" << endl;
}
