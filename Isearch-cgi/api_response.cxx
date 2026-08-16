// ISEARCH2-CLEANUP: processed 2026-08-10
// See docs/PROCESSING_STATUS.md and docs/BUG_CATALOG.md.

#include "api_response.hxx"

#include <stdio.h>
#include <time.h>

static STRING g_request_id;
static LONG g_request_counter = 0;

ApiSearchMeta::ApiSearchMeta()
  : matching_record_count(0), total_retrieved(0), total_database_records(0),
    query_time_seconds(0), start(1), max_hits(0) {}

ApiHit::ApiHit()
  : score(0), has_byte_range(false), record_start(0), record_end(0) {}

ApiLinks::ApiLinks()
{
}

static const CHR *HttpStatusText(const INT status)
{
  switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 406: return "Not Acceptable";
    case 415: return "Unsupported Media Type";
    case 422: return "Unprocessable Entity";
    case 429: return "Too Many Requests";
    case 501: return "Not Implemented";
    case 500: return "Internal Server Error";
    default: return "Unknown";
  }
}

static void EnsureRequestId()
{
  if (g_request_id.GetLength() > 0) {
    return;
  }

  CHR idbuf[64];
  const LONG now = (LONG)time(nullptr);
  ++g_request_counter;
  snprintf(idbuf, sizeof(idbuf), "req-%ld-%ld", now, g_request_counter);
  g_request_id = idbuf;
}

void WriteHttpHeader(const INT status, const bool problem_json)
{
  EnsureRequestId();
  cout << "Status: " << status << " " << HttpStatusText(status) << "\n";
  if (problem_json) {
    cout << "Content-Type: application/problem+json; charset=utf-8\n";
  } else {
    cout << "Content-Type: application/json; charset=utf-8\n";
  }
  cout << "X-Request-Id: " << g_request_id << "\n";
  cout << "Cache-Control: no-store\n\n";
}

void BeginSearchResponse(const ApiSearchMeta& meta)
{
  if (meta.request_id.GetLength() > 0) {
    g_request_id = meta.request_id;
  } else {
    EnsureRequestId();
  }

  cout << "{";
  cout << "\"request_id\":";
  WriteJsonEscaped(g_request_id);
  cout << ",\"database\":";
  WriteJsonEscaped(meta.database);
  cout << ",\"search_type\":";
  WriteJsonEscaped(meta.search_type);
  cout << ",\"matching_record_count\":" << meta.matching_record_count;
  cout << ",\"total_retrieved\":" << meta.total_retrieved;
  cout << ",\"interpreted_query\":";
  WriteJsonEscaped(meta.interpreted_query);
  cout << ",\"total_database_records\":" << meta.total_database_records;
  cout << ",\"query_time_seconds\":" << meta.query_time_seconds;
  cout << ",\"start\":" << meta.start;
  cout << ",\"max_hits\":" << meta.max_hits;
  cout << ",\"results\":[";
}

void WriteSearchHit(const INT index, const ApiHit& hit, const bool first)
{
  if (!first) {
    cout << ",";
  }

  cout << "{";
  cout << "\"match_number\":" << index;
  cout << ",\"score\":" << hit.score;
  cout << ",\"filename\":";
  WriteJsonEscaped(hit.filename);
  cout << ",\"headline\":";
  WriteJsonEscaped(hit.headline);
  cout << ",\"record_key\":";
  WriteJsonEscaped(hit.record_key);
  cout << ",\"url\":";
  if (hit.url.GetLength() > 0) {
    WriteJsonEscaped(hit.url);
  } else {
    cout << "null";
  }
  if (hit.has_byte_range) {
    cout << ",\"record_start\":" << hit.record_start;
    cout << ",\"record_end\":" << hit.record_end;
  }
  cout << "}";
}

void EndSearchResponse(const ApiLinks& links)
{
  cout << "],\"links\":{";
  cout << "\"next\":";
  if (links.next.GetLength() > 0) {
    WriteJsonEscaped(links.next);
  } else {
    cout << "null";
  }
  cout << ",\"prev\":";
  if (links.prev.GetLength() > 0) {
    WriteJsonEscaped(links.prev);
  } else {
    cout << "null";
  }
  cout << "}}" << endl;
}

void WriteProblem(const INT status, const CHR* type, const CHR* title,
                  const CHR* detail)
{
  const STRING type_string = type ? type : "about:blank";
  const STRING title_string = title ? title : HttpStatusText(status);
  const STRING detail_string = detail ? detail : "";

  cout << "{";
  cout << "\"type\":";
  WriteJsonEscaped(type_string);
  cout << ",\"title\":";
  WriteJsonEscaped(title_string);
  cout << ",\"status\":" << status;
  if (detail_string.GetLength() > 0) {
    cout << ",\"detail\":";
    WriteJsonEscaped(detail_string);
  }
  cout << "}" << endl;
}

void WriteJsonEscaped(const STRING& value)
{
  CHR* text = value.NewCString();
  cout << "\"";
  for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
    const unsigned char c = *p;
    switch (c) {
      case '\"': cout << "\\\""; break;
      case '\\': cout << "\\\\"; break;
      case '\b': cout << "\\b"; break;
      case '\f': cout << "\\f"; break;
      case '\n': cout << "\\n"; break;
      case '\r': cout << "\\r"; break;
      case '\t': cout << "\\t"; break;
      default:
        if (c < 0x20) {
          CHR buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int)c);
          cout << buf;
        } else {
          cout << (CHR)c;
        }
        break;
    }
  }
  cout << "\"";
  delete [] text;
}
