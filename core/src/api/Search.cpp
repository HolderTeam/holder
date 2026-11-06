#include "api/Contract.h"

#include "capi/Context.h"
#include "index/FtsIndexer.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace holder::contract {

std::vector<SearchCard> search_cards(holder_context& context, std::string project_id, std::string q,
                                     std::int32_t limit, std::int32_t offset) {
  try {
    std::vector<SearchCard> items;
    for (auto& row : context.fts.search_cards(project_id, q, limit, offset)) {
      items.push_back({std::move(row.id), std::move(row.title), row.updated_at, row.created_at,
                       std::move(row.snippet), row.rank});
    }
    return items;
  } catch (const holder::index::FtsQueryError& error) {
    throw InvalidArgument(error.what());
  }
}

std::vector<SearchMessage> search_ai_messages(holder_context& context, std::string project_id, std::string q,
                                               std::int32_t limit, std::int32_t offset) {
  try {
    std::vector<SearchMessage> items;
    for (auto& row : context.fts.search_messages(project_id, q, limit, offset)) {
      items.push_back({std::move(row.id), row.created_at, std::move(row.snippet), row.rank});
    }
    return items;
  } catch (const holder::index::FtsQueryError& error) {
    throw InvalidArgument(error.what());
  }
}

} // namespace holder::contract
