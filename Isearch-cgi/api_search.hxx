#ifndef API_SEARCH_HXX
#define API_SEARCH_HXX

#include <vector>

#include "api_config.hxx"
#include "api_request.hxx"
#include "api_response.hxx"

int ExecuteSearch(const ApiRequest& req, const ApiConfig& cfg,
                  ApiSearchMeta& meta, std::vector<ApiHit>& hits,
                  STRING& error_detail);

#endif
