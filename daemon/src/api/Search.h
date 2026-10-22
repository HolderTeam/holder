#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <string>
#include <vector>

namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct RIG_OPEN_RECORD SearchCardItem {
  std::string card_id;
  std::string title;
  long long updated_at;
  long long created_at;
  std::string snippet;
  double rank;
};

struct RIG_OPEN_RECORD SearchCardsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<SearchCardItem> data;
};

struct RIG_OPEN_RECORD SearchMessageItem {
  std::string message_id;
  long long created_at;
  std::string snippet;
  double rank;
};

struct RIG_OPEN_RECORD SearchMessagesResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<SearchMessageItem> data;
};

// The daemon was started without a search index.
struct SearchUnavailable : CodedError {
  SearchUnavailable() : CodedError("not_implemented", "Search unavailable.") {}
};

RIG_ROUTE(GET, "/search/cards")
RIG_SUMMARY("Search cards")
RIG_DESCRIPTION("q is an SQLite FTS5 query, matched against card titles and bodies in one project. Results are ordered by relevance, best first.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a q that is not a valid FTS5 query", std::invalid_argument)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no search index", SearchUnavailable)
SearchCardsResponse get_search_cards(
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::string q RIG_QUERY RIG_MIN_LENGTH(1),
    int limit RIG_QUERY RIG_MINIMUM(1) RIG_DEFAULT(20),
    int offset RIG_QUERY RIG_MINIMUM(0) RIG_DEFAULT(0));

RIG_ROUTE(GET, "/search/ai")
RIG_SUMMARY("Search AI messages")
RIG_DESCRIPTION("q is an SQLite FTS5 query, matched against message content in one project. Results are ordered by relevance, best first.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a q that is not a valid FTS5 query", std::invalid_argument)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no search index", SearchUnavailable)
SearchMessagesResponse get_search_ai(
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::string q RIG_QUERY RIG_MIN_LENGTH(1),
    int limit RIG_QUERY RIG_MINIMUM(1) RIG_DEFAULT(20),
    int offset RIG_QUERY RIG_MINIMUM(0) RIG_DEFAULT(0));

} // namespace holder::api
