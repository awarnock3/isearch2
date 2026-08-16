// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef API_RESPONSE_HXX
#define API_RESPONSE_HXX

#include "gdt.h"
#include "string.hxx"

struct ApiSearchMeta {
  STRING request_id;
  STRING database;
  STRING search_type;
  INT matching_record_count;
  INT total_retrieved;
  STRING interpreted_query;
  INT total_database_records;
  DOUBLE query_time_seconds;
  INT start;
  INT max_hits;

  ApiSearchMeta();
};

struct ApiHit {
  DOUBLE score;
  STRING filename;
  STRING headline;
  STRING record_key;
  STRING url;
  bool has_byte_range;
  LONG record_start;
  LONG record_end;

  ApiHit();
};

struct ApiLinks {
  STRING next;
  STRING prev;

  ApiLinks();
};

void WriteHttpHeader(INT status, bool problem_json);
void BeginSearchResponse(const ApiSearchMeta& meta);
void WriteSearchHit(INT index, const ApiHit& hit, bool first);
void EndSearchResponse(const ApiLinks& links);
void WriteProblem(INT status, const CHR* type, const CHR* title,
                  const CHR* detail);
void WriteJsonEscaped(const STRING& value);

#endif
