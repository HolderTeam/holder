#include "api/CardRead.h"

#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/TagExtractor.h"
#include "card/TagRepo.h"
#include "project/ProjectRepo.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace holder::api {
namespace {

enum class CardOrder { TreeDefault, UpdatedDesc, TitleAsc };

CardOrder card_order(const std::optional<std::string>& order, CardOrder fallback) {
  if (!order) return fallback;
  if (*order == "updated_desc") return CardOrder::UpdatedDesc;
  if (*order == "title_asc") return CardOrder::TitleAsc;
  return CardOrder::TreeDefault;
}

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

// Case-insensitive title, then card ID.
bool title_before(const holder::model::Card& a, const holder::model::Card& b) {
  const auto a_title = lowercase(a.title);
  const auto b_title = lowercase(b.title);
  if (a_title != b_title) return a_title < b_title;
  return a.card_id < b.card_id;
}

void sort_cards(std::vector<holder::model::Card>& cards, CardOrder order) {
  std::sort(cards.begin(), cards.end(), [order](const auto& a, const auto& b) {
    if (order == CardOrder::TreeDefault) {
      if (a.sort_key != b.sort_key) return a.sort_key < b.sort_key;
      if (a.updated_at != b.updated_at) return a.updated_at > b.updated_at;
    } else if (order == CardOrder::UpdatedDesc) {
      if (a.updated_at != b.updated_at) return a.updated_at > b.updated_at;
      if (a.created_at != b.created_at) return a.created_at > b.created_at;
    }
    return title_before(a, b);
  });
}

CardListItem list_item(const holder::model::Card& card, std::optional<int> child_count) {
  return {
      card.card_id,
      card.project_id,
      card.title,
      card.rel_path,
      card.sort_key,
      card.created_at,
      card.updated_at,
      card.parent_card_id,
      card.deleted_at,
      child_count,
  };
}

// A parent ID with surrounding whitespace removed; blank means the project's top level.
std::optional<std::string> trimmed_parent(const std::optional<std::string>& parent) {
  if (!parent) return std::nullopt;
  const auto start = parent->find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return std::nullopt;
  const auto end = parent->find_last_not_of(" \t\r\n");
  return parent->substr(start, end - start + 1);
}

} // namespace

CardListResponse get_cards(holder::platform::Db& db,
                           std::string project_id,
                           std::optional<std::string> tag,
                           std::string view,
                           std::optional<std::string> parent_card_id,
                           std::optional<std::string> order,
                           bool count,
                           int limit,
                           std::optional<int> include_deleted) {
  holder::card::CardRepo repo(db);
  CardListResponse response{true, {}};
  if (tag && !tag->empty()) {
    // TagRepo returns only live cards, so every ID resolves.
    for (const auto& card_id : holder::card::TagRepo(db).list_card_ids_with_tag(project_id, *tag)) {
      if (const auto card = repo.get(card_id)) response.data.push_back(list_item(*card, {}));
    }
    return response;
  }
  const auto child_count = [&](const holder::model::Card& card) -> std::optional<int> {
    if (!count) return std::nullopt;
    return repo.count_children_not_deleted(project_id, card.card_id);
  };
  if (view == "recent") {
    std::vector<holder::model::Card> visible;
    for (auto& card : repo.list_all(project_id)) {
      if (!card.deleted_at) visible.push_back(std::move(card));
    }
    sort_cards(visible, card_order(order, CardOrder::UpdatedDesc));
    const auto shown = std::min(static_cast<std::size_t>(limit), visible.size());
    for (std::size_t i = 0; i < shown; ++i) {
      response.data.push_back(list_item(visible[i], child_count(visible[i])));
    }
    return response;
  }
  auto cards = parent_card_id && !parent_card_id->empty()
                   ? repo.list_children(project_id, *parent_card_id)
                   : repo.list_roots(project_id);
  sort_cards(cards, card_order(order, CardOrder::TreeDefault));
  for (const auto& card : cards) {
    if (card.deleted_at && include_deleted.value_or(0) != 1) continue;
    response.data.push_back(list_item(card, child_count(card)));
  }
  return response;
}

CardContextResponse get_cards_context(holder::platform::Db& db,
                                      std::string project_id,
                                      std::optional<std::string> parent_card_id,
                                      std::string order,
                                      bool count) {
  const auto project = holder::project::ProjectRepo(db).get(project_id);
  if (!project) throw CardReadNotFound("Project not found.");

  holder::card::CardRepo repo(db);
  std::unordered_map<std::string, holder::model::Card> live;
  for (auto& card : repo.list_all(project_id)) {
    if (!card.deleted_at) live.emplace(card.card_id, std::move(card));
  }
  const auto parent = trimmed_parent(parent_card_id);
  if (parent && !live.contains(*parent)) throw CardReadNotFound("Parent card not found.");

  std::unordered_map<std::string, int> live_children;
  for (const auto& [card_id, card] : live) {
    const auto card_parent = trimmed_parent(card.parent_card_id);
    if (card_parent && live.contains(*card_parent)) live_children[*card_parent] += 1;
  }

  CardContextResponse response{
      true, {{project->project_id, project->name}, parent, {}, {}}};
  auto& data = response.data;
  data.breadcrumbs.push_back({"project", project->name, project->project_id, std::nullopt});
  if (parent) {
    // Walk up the live ancestors, bounded in case the chain is cyclic.
    std::vector<const holder::model::Card*> chain;
    for (auto cursor = parent; cursor && chain.size() < 1024;) {
      const auto found = live.find(*cursor);
      if (found == live.end()) break;
      chain.push_back(&found->second);
      cursor = trimmed_parent(found->second.parent_card_id);
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
      data.breadcrumbs.push_back({"card", (*it)->title, std::nullopt, (*it)->card_id});
    }
  }

  auto cards = parent ? repo.list_children(project_id, *parent) : repo.list_roots(project_id);
  sort_cards(cards, card_order(order, CardOrder::TreeDefault));
  for (const auto& card : cards) {
    if (card.deleted_at) continue; // LCOV_EXCL_LINE: level listings hold only live cards.
    std::optional<int> children;
    if (count) {
      const auto found = live_children.find(card.card_id);
      children = found == live_children.end() ? 0 : found->second;
    }
    CardContextCard item;
    static_cast<CardListItem&>(item) = list_item(card, children);
    data.cards.push_back(std::move(item));
  }
  return response;
}

CardGetResponse get_card(holder::platform::Db& db,
                         holder::card::CardStore* card_store,
                         std::string card_id) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  const auto card = card_store->get(card_id);
  if (!card || card->deleted_at) throw CardReadNotFound("Card not found.");
  auto content = card_store->get_content(*card);
  if (!content) throw CardReadNotFound("Card content missing.");
  CardGetResponse response{true,
                           {card->card_id,
                            card->project_id,
                            card->title,
                            card->rel_path,
                            card->sort_key,
                            card->created_at,
                            card->updated_at,
                            card->parent_card_id,
                            card->deleted_at,
                            {},
                            holder::card::TagRepo(db).list_tags_for_card(card->project_id,
                                                                          card->card_id),
                            {}}};
  for (const auto& occurrence : holder::core::extract_tag_occurrences(*content)) {
    response.data.tag_occurrences.push_back({occurrence.tag,
                                             static_cast<long long>(occurrence.byte_start),
                                             static_cast<long long>(occurrence.byte_end)});
  }
  response.data.content = std::move(*content);
  return response;
}

} // namespace holder::api
