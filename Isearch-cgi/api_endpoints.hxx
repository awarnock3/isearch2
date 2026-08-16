// ISEARCH2-CLEANUP: processed 2026-08-16
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#ifndef API_ENDPOINTS_HXX
#define API_ENDPOINTS_HXX

#include "api_config.hxx"

void HandleHealth();
void HandleCapabilities(const ApiConfig& cfg);
void HandleDatabases(const ApiConfig& cfg);
void HandleFetch(const ApiConfig& cfg, const CHR* method, const CHR* body);

#endif
