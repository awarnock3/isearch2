// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "api_config.hxx"

#include <climits>
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
  if (db_path != nullptr && db_path[0] != '\0') {
    this->db_path = db_path;
  }

  max_hits_ceiling =
    ReadPositiveIntEnv("ISEARCH_API_MAX_HITS", API_HARD_MAX_HITS);

  ParseAllowList(getenv("ISEARCH_API_ALLOWLIST"), &allow_list);
}

static INT ReadPositiveIntEnv(const CHR *name, const INT default_value)
{
  const CHR *raw_value = getenv(name);
  if (raw_value == nullptr || raw_value[0] == '\0') {
    return default_value;
  }

  CHR *endptr = nullptr;
  const long parsed = strtol(raw_value, &endptr, 10);
  if (endptr == raw_value || *endptr != '\0' || parsed < 1) {
    return default_value;
  }

  // BUGFIX #1 (docs/BUG_CATALOG.md#isearch-cgiapi_confighxx): parsed is
  // a `long` (64-bit on this platform) but was cast straight to `INT`
  // (32-bit) with no range check -- a value above INT_MAX silently
  // truncated to an unrelated, potentially nonsensical INT (confirmed:
  // ISEARCH_API_MAX_HITS=99999999999 produced max_hits_ceiling=
  // 1215752191, not a clamp to any sane maximum). Low real-world
  // severity (this env var is server-side configuration, not
  // remote-request input), but a real, easy-to-trigger-by-typo
  // truncation with a cheap, safe fix: clamp to INT_MAX instead of
  // wrapping.
  if (parsed > INT_MAX) {
    return INT_MAX;
  }

  return static_cast<INT>(parsed);
}

static void ParseAllowList(const CHR *raw_value, STRLIST *allow_list)
{
  if (raw_value == nullptr || raw_value[0] == '\0') {
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
    // BUGFIX #2 (docs/BUG_CATALOG.md#isearch-cgiapi_confighxx):
    // STRING::Trim() only strips *trailing* whitespace (src/string.cxx)
    // -- leading whitespace needs the separate TrimLeading() call, the
    // same two-call convention already used elsewhere in this tree
    // (e.g. src/thesaurus.cxx). Every entry after the first in an
    // ordinarily-formatted "alpha, beta, gamma" allowlist kept its
    // leading space, so it would never exact-match anything a caller
    // checks it against -- silently breaking the allowlist for every
    // entry but the first. Confirmed via a standalone Catch2 test
    // before this fix (entries came back as " beta"/" gamma", not
    // "beta"/"gamma").
    entry.Trim();
    entry.TrimLeading();
    if (entry.GetLength() > 0) {
      allow_list->AddEntry(entry);
    }
  }
}

ApiConfig LoadApiConfig()
{
  return ApiConfig();
}
