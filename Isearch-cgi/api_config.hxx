#ifndef API_CONFIG_HXX
#define API_CONFIG_HXX

#include "gdt.h"
#include "string.hxx"
#include "strlist.hxx"

extern const CHR *API_VERSION;
extern const INT API_DEFAULT_MAX_HITS;
extern const INT API_HARD_MAX_HITS;

struct ApiConfig {
  STRING db_path;
  INT max_hits_ceiling;
  STRLIST allow_list;

  ApiConfig();
};

ApiConfig LoadApiConfig();

#endif
