// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef API_REQUEST_HXX
#define API_REQUEST_HXX

#include <vector>

#include "cgi-util.hxx"
#include "string.hxx"

enum SearchType {
  SEARCH_SIMPLE = 0,
  SEARCH_ADVANCED = 1,
  SEARCH_BOOLEAN = 2
};

enum BoolOperator {
  OP_OR = 0,
  OP_AND = 1,
  OP_ANDNOT = 2,
  OP_NEAR = 3
};

struct ApiTerm {
  STRING term;
  STRING field;
  STRING weight;
  bool phrase;

  ApiTerm();
};

struct ApiRequest {
  STRING database;
  STRING q;
  SearchType search_type;
  BoolOperator op;
  bool rpn;
  bool infix;
  bool and_mode;
  bool synonyms;
  std::vector<ApiTerm> terms;
  std::vector<STRING> doc_type_options;
  STRING element_set;
  STRING record_syntax;
  STRING highlight_prefix;
  STRING highlight_suffix;
  bool byte_range;
  INT start;
  INT max_hits;
  INT start_doc;
  INT end_doc;
  bool has_start_doc;
  bool has_end_doc;
  bool has_rect;
  DOUBLE rect_north;
  DOUBLE rect_south;
  DOUBLE rect_west;
  DOUBLE rect_east;
  bool include_url;
  bool include_headline;
  bool include_record_key;
  INT score_scale;
  STRING request_id;

  ApiRequest();
};

bool ParseRequest(CGIAPP* cgi, const CHR* method, const CHR* body,
                  ApiRequest& out, STRING& error_detail);

#endif
