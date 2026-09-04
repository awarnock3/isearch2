// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef API_FETCH_HXX
#define API_FETCH_HXX

#include "api_config.hxx"
#include "api_request.hxx"
#include "string.hxx"

struct ApiFetchRequest {
  STRING database;
  STRING record_key;
  STRING element_set;
  STRING record_syntax;
  STRING request_id;

  ApiFetchRequest();
};

struct ApiFetchResult {
  STRING record_key;
  STRING database;
  STRING filename;
  STRING content;

  ApiFetchResult();
};

bool ParseFetchRequest(CGIAPP* cgi, const CHR* method, const CHR* body,
                       ApiFetchRequest& out, STRING& error_detail);

int ExecuteFetch(const ApiFetchRequest& req, const ApiConfig& cfg,
                 ApiFetchResult& result, STRING& error_detail);

#endif
