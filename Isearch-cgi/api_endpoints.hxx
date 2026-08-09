// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef API_ENDPOINTS_HXX
#define API_ENDPOINTS_HXX

#include "api_config.hxx"

void HandleHealth();
void HandleCapabilities(const ApiConfig& cfg);
void HandleDatabases(const ApiConfig& cfg);

#endif
