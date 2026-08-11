#include "api_search.hxx"

#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "infix2rpn.hxx"
#include "isearch.hxx"
#include "tokengen.hxx"
#include "vidb.hxx"

static const CHR *SearchTypeName(const SearchType type)
{
  switch (type) {
    case SEARCH_ADVANCED: return "advanced";
    case SEARCH_BOOLEAN: return "boolean";
    case SEARCH_SIMPLE:
    default: return "simple";
  }
}

static const CHR *OperatorToken(const BoolOperator op)
{
  switch (op) {
    case OP_AND: return " and ";
    case OP_ANDNOT: return " andnot ";
    case OP_NEAR: return " near ";
    case OP_OR:
    default: return " or ";
  }
}

static void BuildQueryFromTerms(const ApiRequest& req, STRING *query)
{
  if (query == NULL || req.terms.empty()) {
    return;
  }

  *query = "";
  for (size_t i = 0; i < req.terms.size(); i++) {
    const ApiTerm& t = req.terms[i];
    if (t.term.GetLength() == 0) {
      continue;
    }

    if (query->GetLength() > 0) {
      query->Cat(OperatorToken(req.op));
    }

    if (t.field.GetLength() > 0 && !t.field.CaseEquals("FULLTEXT")) {
      query->Cat(t.field);
      query->Cat("/");
    }

    if (t.phrase) {
      query->Cat("\"");
      query->Cat(t.term);
      query->Cat("\"");
    } else {
      query->Cat(t.term);
    }

    if (t.weight.GetLength() > 0) {
      query->Cat(":");
      query->Cat(t.weight);
    }
  }
}

static bool HasBooleanTokens(const STRING& query_text)
{
  TOKENGEN token_gen(query_text);
  STRING token;
  const INT total = token_gen.GetTotalEntries();
  for (INT i = 1; i <= total; i++) {
    token_gen.GetEntry(i, &token);
    if ((token ^= "AND") || (token ^= "OR") || (token ^= "ANDNOT") ||
        (token == "||") || (token == "&&") || (token ^= "NEAR")) {
      return true;
    }
  }
  return false;
}

static bool BuildSquery(const ApiRequest& req, SQUERY *search_query,
                        STRING *interpreted_query, STRING& error_detail)
{
  if (search_query == NULL || interpreted_query == NULL) {
    error_detail = "Internal error building query.";
    return false;
  }

  STRING query_text = req.q;
  if (query_text.GetLength() == 0) {
    BuildQueryFromTerms(req, &query_text);
  }
  query_text.Trim();

  if (query_text.GetLength() == 0) {
    error_detail = "Empty query.";
    return false;
  }

  *interpreted_query = query_text;

  const bool force_rpn = req.rpn || req.infix || req.search_type == SEARCH_BOOLEAN;

  if (!force_rpn && req.search_type == SEARCH_SIMPLE) {
    search_query->SetTerm(query_text);
    return true;
  }

  if (!force_rpn && req.search_type == SEARCH_ADVANCED) {
    if (HasBooleanTokens(query_text)) {
      STRING processed;
      INFIX2RPN parser;
      parser.SetDefaultOp("AND");
      parser.Parse(query_text, &processed);
      if (!parser.InputParsedOK()) {
        error_detail = "The query was unparseable.";
        return false;
      }
      search_query->SetRpnTerm(processed);
      return true;
    }
    search_query->SetTerm(query_text);
    return true;
  }

  STRING processed;
  INFIX2RPN parser;
  parser.SetDefaultOp("AND");
  parser.Parse(query_text, &processed);
  if (!parser.InputParsedOK()) {
    error_detail = "The query was unparseable.";
    return false;
  }
  search_query->SetRpnTerm(processed);
  return true;
}

static void BuildResultUrl(const STRING& full_name, STRING *url_out)
{
  if (url_out == NULL) {
    return;
  }
  *url_out = "";

  CHR *name = full_name.NewCString();
  if (name == NULL) {
    return;
  }

  CHR *http_path = (CHR *)getenv("DOCUMENT_ROOT");
  if (http_path != NULL) {
    CHR *url = strstr(name, http_path);
    if (url != NULL) {
      url += strlen(http_path);
      *url_out = url;
    }
  }

  delete [] name;
}

int ExecuteSearch(const ApiRequest& req, const ApiConfig& cfg,
                  ApiSearchMeta& meta, std::vector<ApiHit>& hits,
                  STRING& error_detail)
{
  hits.clear();
  error_detail = "";

  meta = ApiSearchMeta();
  meta.request_id = req.request_id;
  meta.database = req.database;
  meta.search_type = SearchTypeName(req.search_type);
  meta.start = req.start;
  meta.max_hits = req.max_hits;

  STRING db_path = cfg.db_path;
  if (db_path.GetLength() == 0) {
    db_path = ".";
  }

  SQUERY query;
  STRING interpreted_query;
  if (!BuildSquery(req, &query, &interpreted_query, error_detail)) {
    return 422;
  }
  meta.interpreted_query = interpreted_query;

  VIDB *pdb = new VIDB(db_path, req.database);
  if (pdb == NULL) {
    error_detail = "Failed to open database.";
    return 500;
  }

  if (pdb->GetTotalRecords() <= 0) {
    error_detail = "Database does not exist or is corrupted.";
    delete pdb;
    return 404;
  }

  const time_t start_time = time(NULL);
  PIRSET pirset = NULL;
  if ((req.op == OP_AND || req.and_mode) && req.search_type == SEARCH_SIMPLE &&
      !req.rpn && !req.infix) {
    pirset = pdb->AndSearch(query);
  } else {
    pirset = pdb->Search(query);
  }
  const time_t end_time = time(NULL);

  if (pirset == NULL) {
    error_detail = "Search execution failed.";
    delete pdb;
    return 500;
  }

  pirset->SortByScore();

  INT hit_count = pirset->GetTotalEntries();
  INT present_start = req.start;
  INT present_limit = req.max_hits;
  if (req.has_start_doc) present_start = req.start_doc;
  if (req.has_end_doc && req.end_doc >= present_start) {
    const INT ranged = req.end_doc - present_start + 1;
    if (ranged < present_limit) present_limit = ranged;
  }
  PRSET prset = pirset->GetRset(0, hit_count);
  pirset->Fill(0, hit_count, prset);
  prset->SetScoreRange(pirset->GetMaxScore(), pirset->GetMinScore());
  hit_count = (INT)prset->GetTotalEntries();

  INT fetch_count = 0;
  if (present_start <= hit_count) {
    fetch_count = hit_count > (present_start + present_limit - 1)
                  ? present_limit
                  : (hit_count - present_start + 1);
  }

  meta.matching_record_count = hit_count;
  meta.total_retrieved = fetch_count;
  meta.total_database_records = pdb->GetTotalRecords();
  meta.query_time_seconds = (DOUBLE)(end_time - start_time);

  RESULT rs_record;
  STRING full_name;
  STRING file;
  STRING record_key;
  STRING headline;

  for (INT i = present_start; i <= (present_start + fetch_count - 1); i++) {
    prset->GetEntry(i, &rs_record);
    pdb->Present(rs_record, req.element_set, req.record_syntax, &headline);
    rs_record.GetFullFileName(&full_name);
    rs_record.GetFileName(&file);
    rs_record.GetKey(&record_key);

    ApiHit hit;
    hit.score = prset->GetScaledScore(rs_record.GetScore(), req.score_scale);
    if (req.include_headline) hit.headline = headline;
    if (req.include_record_key) hit.record_key = record_key;
    if (req.include_url) BuildResultUrl(full_name, &hit.url);
    if (req.byte_range) {
      hit.has_byte_range = true;
      hit.record_start = (LONG)rs_record.GetRecordStart();
      hit.record_end = (LONG)rs_record.GetRecordEnd();
    }
    hit.filename = file;
    hits.push_back(hit);
  }

  delete prset;
  delete pirset;
  delete pdb;
  return 200;
}
