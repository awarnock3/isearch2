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
  std::vector<ApiTerm> terms;
  STRING element_set;
  STRING record_syntax;
  INT start;
  INT max_hits;
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
