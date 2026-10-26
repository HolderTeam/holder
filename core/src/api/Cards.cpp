// The contract's Card operations (contract/interface/cards.py). The generated glue checks their
// arguments first; these do the work.
#include "api/Contract.h"

#include "capi/Context.h"
#include "card/CardPaths.h"
#include "card/CardPlacementError.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/TagExtractor.h"
#include "card/TagRepo.h"
#include "identity/Uuid.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <limits>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

// A content page reads every body under one project lock, so it is kept smaller.
constexpr int MAX_CONTENT_PAGE = 1000;

long long now_epoch_seconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

holder::card::CardStore card_store(holder_context& context) {
  return holder::card::CardStore(context.db, &context.fts);
}

Card full_card(holder_context& context, holder::card::CardStore& store, const holder::model::Card& card) {
  auto content = store.get_content(card);
  if (!content) throw NotFound("card content missing: " + card.card_id);
  Card out{card, std::move(*content), holder::card::TagRepo(context.db).list_tags_for_card(card.project_id, card.card_id),
           {}};
  for (const auto& occurrence : holder::core::extract_tag_occurrences(out.content)) {
    out.tag_occurrences.push_back({occurrence.tag, static_cast<long long>(occurrence.byte_start),
                                   static_cast<long long>(occurrence.byte_end)});
  }
  return out;
}

// Cursors are the hex of a JSON object naming the query they continue, so a cursor cannot be
// used with a different project, filter or order. The daemon's card page uses the same format.
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

holder::card::CardPageCursor decode_cursor(const std::string& cursor, const nlohmann::json& scope, bool sort_key) {
  if (cursor.size() % 2) throw InvalidArgument("invalid cursor");
  const auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw InvalidArgument("invalid cursor");
  };
  try {
    std::string text;
    for (std::size_t i = 0; i < cursor.size(); i += 2)
      text += static_cast<char>((digit(cursor[i]) << 4) | digit(cursor[i + 1]));
    const auto decoded = nlohmann::json::parse(text);
    if (decoded.at("version") != 1 || decoded.at("query") != scope)
      throw InvalidArgument("the cursor belongs to a different query");
    const auto id = decoded.at("card_id").get<std::string>();
    const auto& timestamp = decoded.at("updated_at");
    if (id.empty() || !timestamp.is_number_integer() ||
        (timestamp.is_number_unsigned() &&
         timestamp.get<unsigned long long>() >
             static_cast<unsigned long long>(std::numeric_limits<long long>::max())))
      throw InvalidArgument("invalid cursor");
    holder::card::CardPageCursor result{id, timestamp.get<long long>()};
    if (sort_key) {
      const auto& key = decoded.at("sort_key");
      if (!key.is_number()) throw InvalidArgument("invalid cursor");
      result.sort_key = key.get<double>();
    }
    return result;
  } catch (const nlohmann::json::exception&) {
    throw InvalidArgument("invalid cursor");
  }
}

const char* order_name(ListCardsOrder order) {
  switch (order) {
  case ListCardsOrder::UpdatedDesc: return "updated_desc";
  case ListCardsOrder::SortKeyAsc: return "sort_key_asc";
  case ListCardsOrder::CardIdAsc: break;
  }
  return "card_id_asc";
}

} // namespace

Card get_card(holder_context& context, std::string card_id) {
  auto store = card_store(context);
  const auto card = store.get(card_id);
  if (!card || card->deleted_at) throw NotFound("card not found: " + card_id);
  return full_card(context, store, *card);
}

Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent,
                              std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit,
                              std::optional<std::string> cursor, bool include_content, bool include_child_count) {
  holder::card::CardPageQuery query;
  std::string normalized_tag;
  if (tag) {
    if (!holder::core::is_valid_tag(*tag)) throw InvalidArgument("invalid tag; omit the leading #");
    normalized_tag = holder::core::normalize_tag(*tag);
    query.tag = normalized_tag;
  }
  std::string parent_value;
  if (parent) {
    if (const auto* card_id = std::get_if<std::string>(&*parent)) {
      parent_value = *card_id;
      query.parent = holder::card::CardPageChildrenOf{parent_value};
    } else {
      parent_value = "roots";
      query.parent = holder::card::CardPageRoots{};
    }
  }
  if (include_content && limit > MAX_CONTENT_PAGE)
    throw InvalidArgument("limit must be from 1 to 1000 when include_content is true");
  if (order == ListCardsOrder::UpdatedDesc) query.order = holder::card::CardPageOrder::UpdatedDesc;
  if (order == ListCardsOrder::SortKeyAsc) query.order = holder::card::CardPageOrder::SortKeyAsc;
  const nlohmann::json scope = {{"project_id", project_id},
                                {"tag", normalized_tag},
                                {"parent", parent_value},
                                {"order", order_name(order)}};
  if (cursor) query.cursor = decode_cursor(*cursor, scope, order == ListCardsOrder::SortKeyAsc);
  if (!holder::project::ProjectRepo(context.db).get(project_id)) throw NotFound("project not found: " + project_id);

  holder::card::CollectionCardPage page;
  holder::card::CardRepo cards(context.db);
  if (include_content) {
    page = card_store(context).list_collection_page(project_id, query, limit, true);
  } else {
    auto rows = cards.list_collection_page(project_id, query, limit + 1);
    if (rows.size() > static_cast<std::size_t>(limit)) {
      rows.resize(static_cast<std::size_t>(limit));
      page.next_cursor = holder::card::CardPageCursor{rows.back().card_id, rows.back().updated_at, rows.back().sort_key};
    }
    for (auto& card : rows) page.cards.push_back({std::move(card), {}});
  }

  Page<CardListItem> out;
  if (page.next_cursor) {
    out.next_cursor = encode_cursor({{"version", 1},
                                     {"query", scope},
                                     {"card_id", page.next_cursor->card_id},
                                     {"updated_at", *page.next_cursor->updated_at},
                                     {"sort_key", *page.next_cursor->sort_key}});
  }
  for (auto& [card, content] : page.cards) {
    CardListItem item{card, std::nullopt, std::nullopt};
    if (include_child_count) item.child_count = cards.count_children_not_deleted(project_id, card.card_id);
    if (include_content) item.content = std::move(content);
    out.items.push_back(std::move(item));
  }
  return out;
}

Card create_card(holder_context& context, const NewCard& request) {
  const auto project = holder::project::ProjectRepo(context.db).get(request.project_id);
  if (!project) throw NotFound("project not found: " + request.project_id);

  holder::model::Card card;
  card.card_id = holder::identity::generate_id(project->id_scheme);
  card.project_id = request.project_id;
  card.title = request.title;
  card.parent_card_id = request.parent_card_id;
  card.created_at = request.created_at.value_or(now_epoch_seconds());
  card.updated_at = request.updated_at.value_or(card.created_at);
  card.rel_path = holder::core::card_rel_path(card.card_id);

  auto store = card_store(context);
  try {
    store.create(card, request.content, request.sort_key);
  } catch (const holder::card::CardPlacementError& error) {
    if (error.code() == holder::card::CardPlacementErrorCode::TargetNotFound)
      throw NotFound("parent card not found: " + request.parent_card_id.value_or(""));
    throw InvalidArgument(error.what());
  }
  const auto created = store.get(card.card_id);
  if (!created) throw Error("the created card cannot be read back: " + card.card_id);
  return full_card(context, store, *created);
}

} // namespace holder::contract
