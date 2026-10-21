#include "api/Search.h"

#include "index/FtsIndexer.h"

#include <utility>

namespace holder::api {

SearchCardsResponse get_search_cards(holder::index::FtsIndexer* fts,
                                     std::string project_id,
                                     std::string q,
                                     int limit,
                                     int offset) {
  if (fts == nullptr) throw SearchUnavailable{};
  SearchCardsResponse response{true, {}};
  for (auto& row : fts->search_cards(project_id, q, limit, offset)) {
    response.data.push_back({std::move(row.id), std::move(row.title), row.updated_at,
                             row.created_at, std::move(row.snippet), row.rank});
  }
  return response;
}

SearchMessagesResponse get_search_ai(holder::index::FtsIndexer* fts,
                                     std::string project_id,
                                     std::string q,
                                     int limit,
                                     int offset) {
  if (fts == nullptr) throw SearchUnavailable{};
  SearchMessagesResponse response{true, {}};
  for (auto& row : fts->search_messages(project_id, q, limit, offset)) {
    response.data.push_back({std::move(row.id), row.created_at, std::move(row.snippet), row.rank});
  }
  return response;
}

} // namespace holder::api
