#include "api_config.hxx"

#include <cstdlib>

const CHR *API_VERSION = "1";
const INT API_DEFAULT_MAX_HITS = 50;
const INT API_HARD_MAX_HITS = 1000;

static INT ReadPositiveIntEnv(const CHR *name, const INT default_value);
static void ParseAllowList(const CHR *raw_value, STRLIST *allow_list);

ApiConfig::ApiConfig()
  : max_hits_ceiling(API_HARD_MAX_HITS)
{
  const CHR *db_path = getenv("ISEARCH_DB_PATH");
  if (db_path != NULL && db_path[0] != '\0') {
    this->db_path = db_path;
  }

  max_hits_ceiling =
    ReadPositiveIntEnv("ISEARCH_API_MAX_HITS", API_HARD_MAX_HITS);

  ParseAllowList(getenv("ISEARCH_API_ALLOWLIST"), &allow_list);
}

static INT ReadPositiveIntEnv(const CHR *name, const INT default_value)
{
  const CHR *raw_value = getenv(name);
  if (raw_value == NULL || raw_value[0] == '\0') {
    return default_value;
  }

  CHR *endptr = NULL;
  const long parsed = strtol(raw_value, &endptr, 10);
  if (endptr == raw_value || *endptr != '\0' || parsed < 1) {
    return default_value;
  }

  return static_cast<INT>(parsed);
}

static void ParseAllowList(const CHR *raw_value, STRLIST *allow_list)
{
  if (raw_value == NULL || raw_value[0] == '\0') {
    return;
  }

  STRING list_value = raw_value;
  list_value.Trim();
  if (list_value.GetLength() == 0) {
    return;
  }

  STRLIST parsed_entries;
  parsed_entries.Split(',', list_value);

  const SIZE_T total = parsed_entries.GetTotalEntries();
  for (SIZE_T i = 1; i <= total; i++) {
    STRING entry;
    parsed_entries.GetEntry(i, &entry);
    entry.Trim();
    if (entry.GetLength() > 0) {
      allow_list->AddEntry(entry);
    }
  }
}

ApiConfig LoadApiConfig()
{
  return ApiConfig();
}
