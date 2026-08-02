#ifndef API_ENDPOINTS_HXX
#define API_ENDPOINTS_HXX

#include "api_config.hxx"

void HandleHealth();
void HandleCapabilities(const ApiConfig& cfg);
void HandleDatabases(const ApiConfig& cfg);

#endif
