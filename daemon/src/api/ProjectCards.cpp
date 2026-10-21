#include "api/ProjectCards.h"

#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/TagExtractor.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

// Cursors are the hex of a JSON object naming the query they continue.
std::string encode_cursor(const nlohmann::json& value) {
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (const char character : value.dump()) {
    const auto byte = static_cast<unsigned char>(character);
    result += hex[byte >> 4];
    result += hex[byte & 15];
  }
  return result;
}

nlohmann::json decode_cursor(const std::string& value) {
  if (value.size() % 2) throw std::invalid_argument("Invalid cursor.");
  const auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw std::invalid_argument("Invalid cursor.");
  };
  std::string decoded;
  for (std::size_t i = 0; i < value.size(); i += 2)
    decoded += static_cast<char>((digit(value[i]) << 4) | digit(value[i + 1]));
  return nlohmann::json::parse(decoded);
}

holder::card::CardPageCursor page_cursor(const std::string& cursor, const nlohmann::json& scope,
                                         bool sort_key) {
  try {
    const auto decoded = decode_cursor(cursor);
    if (decoded.at("version") != 1 || decoded.at("query") != scope)
      throw std::invalid_argument("Cursor belongs to a different query.");
    const auto id = decoded.at("card_id").get<std::string>();
    const auto& timestamp = decoded.at("updated_at");
    if (id.empty() || !timestamp.is_number_integer() ||
        (timestamp.is_number_unsigned() &&
         timestamp.get<unsigned long long>() >
             static_cast<unsigned long long>(std::numeric_limits<long long>::max())))
      throw std::invalid_argument("Invalid cursor.");
    holder::card::CardPageCursor result{id, timestamp.get<long long>()};
    if (sort_key) {
      const auto& key = decoded.at("sort_key");
      if (!key.is_number()) throw std::invalid_argument("Invalid cursor.");
      result.sort_key = key.get<double>();
    }
    return result;
  } catch (const nlohmann::json::exception&) {
    throw std::invalid_argument("Invalid cursor.");
  }
}

} // namespace

CardPageResponse get_projects_project_id_cards(holder::platform::Db& db,
                                               holder::card::CardStore* card_store,
                                               std::string project_id,
                                               std::optional<std::string> tag,
                                               std::optional<std::variant<ParentRoots, ParentUuid>> parent,
                                               std::string order,
                                               int limit,
                                               std::optional<std::string> cursor,
                                               bool include_content,
                                               bool include_child_count) {
  holder::card::CardPageQuery query;
  std::string normalized_tag;
  if (tag) {
    if (!holder::core::is_valid_tag(*tag))
      throw std::invalid_argument("Invalid tag. Omit the leading #.");
    normalized_tag = holder::core::normalize_tag(*tag);
    query.tag = normalized_tag;
  }
  std::string parent_value;
  if (parent) {
    if (const auto* children_of = std::get_if<ParentUuid>(&*parent)) {
      parent_value = children_of->value;
      query.parent = holder::card::CardPageChildrenOf{parent_value};
    } else {
      parent_value = "roots";
      query.parent = holder::card::CardPageRoots{};
    }
  }
  if (include_content && limit > 1000)
    throw std::invalid_argument("limit must be from 1 to 1000 when include_content is true.");
  if (order == "updated_desc")
    query.order = holder::card::CardPageOrder::UpdatedDesc;
  else if (order == "sort_key_asc")
    query.order = holder::card::CardPageOrder::SortKeyAsc;
  const nlohmann::json scope = {{"project_id", project_id},
                                {"tag", normalized_tag},
                                {"parent", parent_value},
                                {"order", order}};
  if (cursor) query.cursor = page_cursor(*cursor, scope, order == "sort_key_asc");
  if (!holder::project::ProjectRepo(db).get(project_id))
    throw ProjectCardsNotFound("Project not found.");

  // Content pages go through core's locked read, which selects and reads every body under
  // one project lock and fails the whole page if a body is unavailable.
  holder::card::CollectionCardPage page;
  holder::card::CardRepo cards(db);
  if (include_content) {
    if (card_store == nullptr) throw CardStoreUnavailable{};
    try {
      page = card_store->list_collection_page(project_id, query, limit, true);
    } catch (const holder::privacy::PrivacyError& error) {
      support::throw_privacy_failure(error);
    }
  } else {
    auto rows = cards.list_collection_page(project_id, query, limit + 1);
    if (rows.size() > static_cast<std::size_t>(limit)) {
      rows.resize(static_cast<std::size_t>(limit));
      page.next_cursor = holder::card::CardPageCursor{
          rows.back().card_id, rows.back().updated_at, rows.back().sort_key};
    }
    for (auto& card : rows) page.cards.push_back({std::move(card), {}});
  }

  CardPageResponse response{true, {{}, std::nullopt}};
  if (page.next_cursor) {
    response.data.next_cursor = encode_cursor({{"version", 1},
                                               {"query", scope},
                                               {"card_id", page.next_cursor->card_id},
                                               {"updated_at", *page.next_cursor->updated_at},
                                               {"sort_key", *page.next_cursor->sort_key}});
  }
  for (auto& [card, content] : page.cards) {
    CardPageItem item;
    item.card_id = card.card_id;
    item.project_id = card.project_id;
    item.title = card.title;
    item.rel_path = card.rel_path;
    item.sort_key = card.sort_key;
    item.created_at = card.created_at;
    item.updated_at = card.updated_at;
    item.parent_card_id = card.parent_card_id;
    item.deleted_at = card.deleted_at;
    if (include_child_count)
      item.child_count = cards.count_children_not_deleted(project_id, card.card_id);
    if (include_content) item.content = std::move(content);
    response.data.items.push_back(std::move(item));
  }
  return response;
}

CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    holder::platform::Db& db, std::string project_id, std::string card_id) {
  const auto project = holder::project::ProjectRepo(db).get(project_id);
  if (!project) throw ProjectCardsNotFound("Project not found.");
  holder::card::CardRepo cards(db);
  std::vector<holder::model::Card> chain;
  std::unordered_set<std::string> visited;
  std::optional<std::string> current = card_id;
  while (current) {
    if (chain.size() >= 1024 || !visited.insert(*current).second)
      throw CardHierarchyInvalid("Card ancestry is cyclic or too deep.");
    const auto card = cards.find_by_id(project_id, *current, holder::model::CardScope::Live);
    if (!card) {
      if (chain.empty()) throw ProjectCardsNotFound("Card not found in this project.");
      throw CardHierarchyInvalid("Card ancestry is broken.");
    }
    chain.push_back(*card);
    current = card->parent_card_id;
    if (current && current->empty()) current = std::nullopt;
  }
  CardBreadcrumbsResponse response{true, {}};
  response.data.push_back({"project", project_id, project->name});
  for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    response.data.push_back({"card", it->card_id, it->title});
  return response;
}

} // namespace holder::api
