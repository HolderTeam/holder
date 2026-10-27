// The contract's Card operations (contract/interface/cards.py). The generated glue checks their
// arguments first; these do the work.
#include "api/Contract.h"

#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "card/CardPaths.h"
#include "card/CardPlacementError.h"
#include "card/CardPlacementResolver.h"
#include "card/CardRepo.h"
#include "card/CardStore.h"
#include "card/TagExtractor.h"
#include "card/TagLineEditor.h"
#include "card/TagRepo.h"
#include "identity/Uuid.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <set>
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
  return holder::card::CardStore(context.db, &context.fts, nullptr, context.git);
}

Card full_card(holder_context& context, holder::card::CardStore& store, const holder::model::Card& card) {
  auto content = store.get_content(card);
  if (!content) throw NotFound("card content missing: " + card.card_id);
  // Tags are read from the content, so a card in the trash, which the tag index leaves out, has them too.
  Card out{card, std::move(*content), {}, {}};
  std::set<std::string> tags;
  for (const auto& occurrence : holder::core::extract_tag_occurrences(out.content)) {
    out.tag_occurrences.push_back({occurrence.tag, static_cast<long long>(occurrence.byte_start),
                                   static_cast<long long>(occurrence.byte_end)});
    tags.insert(occurrence.tag);
  }
  out.tags.assign(tags.begin(), tags.end());
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

// The card, live or in the trash, when it is in the project: a card in another project is not found.
std::optional<holder::model::Card> card_in(holder::card::CardStore& store, const std::string& project_id,
                                           const std::string& card_id) {
  auto card = store.get(card_id);
  if (!card || card->project_id != project_id) return std::nullopt;
  return card;
}

// A live card in the project, with nothing read but its metadata.
holder::model::Card live_card(holder::card::CardStore& store, const std::string& project_id,
                              const std::string& card_id) {
  auto card = card_in(store, project_id, card_id);
  if (!card || card->deleted_at) throw NotFound("card not found: " + card_id);
  return std::move(*card);
}

Card find_card(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto store = card_store(context);
  return full_card(context, store, live_card(store, project_id, card_id));
}

Page<CardListItem> list_page(holder_context& context, const std::string& project_id,
                             std::optional<ListCardsParent> parent, std::optional<std::string> tag,
                             ListCardsOrder order, std::int32_t limit, std::optional<std::string> cursor,
                             bool include_content, bool include_child_count) {
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

Card create(holder_context& context, const std::string& project_id, const NewCard& request) {
  const auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("project not found: " + project_id);

  holder::model::Card card;
  card.card_id = holder::identity::generate_id(project->id_scheme);
  card.project_id = project_id;
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

// Whether parent is the card itself or one of its descendants: a parent the card cannot go under.
bool under_itself(holder::card::CardStore& store, const std::string& card_id, std::optional<std::string> parent) {
  std::set<std::string> seen;
  while (parent && seen.insert(*parent).second) {
    if (*parent == card_id) return true;
    const auto card = store.get(*parent);
    if (!card) return false;
    parent = card->parent_card_id;
  }
  return false;
}

// Checks a new parent before anything is written: live, in the card's project, and not under it.
void check_parent(holder::card::CardStore& store, const holder::model::Card& card,
                  const std::optional<std::string>& parent) {
  if (!parent) return;
  const auto found = store.get(*parent);
  if (!found || found->deleted_at || found->project_id != card.project_id)
    throw NotFound("parent card not found: " + *parent);
  if (under_itself(store, card.card_id, parent))
    throw InvalidMove("a card cannot go under itself or one of its descendants");
}

// The card as a write left it.
Card read_back(holder_context& context, holder::card::CardStore& store, const std::string& card_id) {
  const auto card = store.get(card_id);
  if (!card) throw Error("the card cannot be read back: " + card_id);
  return full_card(context, store, *card);
}

// A placement core refuses, as the contract's error kinds. A target or parent in another project is
// one that is not found.
[[noreturn]] void placement_failed(const holder::card::CardPlacementError& error) {
  using Code = holder::card::CardPlacementErrorCode;
  switch (error.code()) {
  case Code::CardNotFound: throw NotFound("card not found");
  case Code::CrossProjectMoveForbidden:
  case Code::TargetNotFound: throw NotFound("the target or parent card is not found");
  case Code::MoveWouldCreateCycle: throw InvalidMove("a card cannot go under itself or one of its descendants");
  case Code::InvalidTarget: throw InvalidMove("the target cannot be the card's sibling");
  case Code::AlreadyAtProjectRoot: throw InvalidMove("the card is already at the top level");
  case Code::MissingTargetCardId: throw InvalidArgument("target_card_id is required for into, before and after");
  case Code::InvalidMoveIntent: break;
  }
  throw InvalidArgument(error.what());
}

Card update(holder_context& context, const std::string& project_id, const std::string& card_id,
            const CardUpdate& update) {
  auto store = card_store(context);
  const auto card = live_card(store, project_id, card_id);
  // A field set to its current value changes nothing, and nothing is written for it.
  std::optional<std::string> content;
  if (update.title || update.content) {
    content = store.get_content(card);
    if (!content) throw NotFound("card content missing: " + card_id);
  }
  const bool retitle = update.title && *update.title != card.title;
  const bool rewrite = retitle || (update.content && *update.content != *content);
  const bool reparent = update.parent_card_id && *update.parent_card_id != card.parent_card_id;
  const bool resort = update.sort_key && *update.sort_key != card.sort_key;
  // Every check comes first, since the content and the placement are written separately.
  if (reparent) check_parent(store, card, *update.parent_card_id);
  const auto updated_at = update.updated_at.value_or(now_epoch_seconds());
  try {
    if (rewrite) {
      // The title is written in the card's file, so a new title rewrites the file with its content.
      store.update_content(card_id, update.content.value_or(*content), retitle ? update.title : std::nullopt,
                           updated_at);
    }
    if (reparent || resort) {
      store.move(card_id, reparent, reparent ? *update.parent_card_id : std::nullopt, update.sort_key, updated_at);
    }
  } catch (const holder::card::CardPlacementError& error) {
    placement_failed(error);
  }
  return read_back(context, store, card_id);
}

Card trash(holder_context& context, const std::string& project_id, const std::string& card_id,
           TrashCardChildren children) {
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  store.trash(card_id, now_epoch_seconds(),
              children == TrashCardChildren::TrashSubtree ? holder::card::TrashChildren::TrashSubtree
                                                          : holder::card::TrashChildren::Promote);
  return read_back(context, store, card_id);
}

Card restore(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto store = card_store(context);
  const auto card = card_in(store, project_id, card_id);
  if (!card) throw NotFound("card not found: " + card_id);
  if (!card->deleted_at) throw Conflict("the card is not in the trash: " + card_id);
  store.restore(card_id, now_epoch_seconds());
  return read_back(context, store, card_id);
}

holder::card::CardPlacementIntent placement_intent(CardMoveIntent intent) {
  using Intent = holder::card::CardPlacementIntent;
  switch (intent) {
  case CardMoveIntent::Into: return Intent::Into;
  case CardMoveIntent::Before: return Intent::Before;
  case CardMoveIntent::After: return Intent::After;
  case CardMoveIntent::Left: return Intent::Left;
  case CardMoveIntent::Right: return Intent::Right;
  case CardMoveIntent::ToStart: return Intent::ToStart;
  case CardMoveIntent::ToEnd: return Intent::ToEnd;
  case CardMoveIntent::UpLevel: break;
  }
  return Intent::UpLevel;
}

Card move(holder_context& context, const std::string& project_id, const std::string& card_id,
          const CardMove& move) {
  auto store = card_store(context);
  const auto card = live_card(store, project_id, card_id);
  holder::card::CardPlacementRequest placement;
  placement.intent = placement_intent(move.intent);
  placement.target_card_id = move.target_card_id;
  // An omitted parent keeps the card's own; null is the top level.
  placement.has_parent_card_id = move.parent_card_id.has_value();
  placement.parent_card_id = move.parent_card_id.value_or(std::nullopt);
  // The parent the card would have. The resolver catches a cycle only for into.
  std::optional<std::string> parent;
  if (move.intent == CardMoveIntent::Into) {
    parent = move.target_card_id;
  } else if (move.intent == CardMoveIntent::Before || move.intent == CardMoveIntent::After) {
    if (move.target_card_id) {
      if (const auto target = store.get(*move.target_card_id)) parent = target->parent_card_id;
    }
  } else if (move.intent != CardMoveIntent::UpLevel) {
    parent = placement.parent_card_id;
  }
  if (under_itself(store, card_id, parent))
    throw InvalidMove("a card cannot go under itself or one of its descendants");
  try {
    store.move_to(card.project_id, card_id, placement, now_epoch_seconds());
  } catch (const holder::card::CardPlacementError& error) {
    placement_failed(error);
  }
  return read_back(context, store, card_id);
}

// A tag, checked before anything is read: an argument that is not a tag is invalid whatever the card.
std::string checked_tag(const std::string& tag) {
  if (!holder::core::is_valid_tag(tag)) throw InvalidArgument("not a tag: " + tag);
  return tag;
}

Card add_tag(holder_context& context, const std::string& project_id, const std::string& card_id,
             const std::string& tag) {
  checked_tag(tag);
  auto store = card_store(context);
  live_card(store, project_id, card_id);
  // A card that already carries the tag, anywhere in its content, is left as it is.
  store.add_tag(card_id, tag, now_epoch_seconds());
  return read_back(context, store, card_id);
}

Card remove_tag(holder_context& context, const std::string& project_id, const std::string& card_id,
                const std::string& tag) {
  checked_tag(tag);
  const auto normalized = holder::core::normalize_tag(tag);
  auto store = card_store(context);
  const auto card = live_card(store, project_id, card_id);
  const auto content = store.get_content(card);
  if (!content) throw NotFound("card content missing: " + card_id);
  // Only the trailing tag line is edited. A card that would still carry the tag, in its prose, is left as it is.
  const auto carries = [&](const std::string& body) {
    const auto tags = holder::core::extract_tags(body);
    return std::find(tags.begin(), tags.end(), normalized) != tags.end();
  };
  if (!carries(*content)) return read_back(context, store, card_id);
  if (carries(holder::core::remove_from_trailing_tag_line(*content, normalized).new_body))
    throw Conflict("the card carries #" + normalized +
                   " in its prose, which Holder does not rewrite; edit its content to remove it");
  store.remove_tag(card_id, tag, now_epoch_seconds());
  return read_back(context, store, card_id);
}

std::vector<TagCount> project_tags(holder_context& context, const std::string& project_id) {
  if (!holder::project::ProjectRepo(context.db).get(project_id)) throw NotFound("project not found: " + project_id);
  std::vector<TagCount> out;
  for (auto& [tag, card_count] : holder::card::TagRepo(context.db).list_project_tags(project_id))
    out.push_back({std::move(tag), card_count});
  return out;
}

} // namespace

Card get_card(holder_context& context, std::string project_id, std::string card_id) {
  return with_contract_errors([&] { return find_card(context, project_id, card_id); });
}

Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent,
                              std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit,
                              std::optional<std::string> cursor, bool include_content, bool include_child_count) {
  return with_contract_errors([&] {
    return list_page(context, project_id, std::move(parent), std::move(tag), order, limit, std::move(cursor),
                     include_content, include_child_count);
  });
}

Card create_card(holder_context& context, std::string project_id, const NewCard& request) {
  return with_contract_errors([&] { return create(context, project_id, request); });
}

Card update_card(holder_context& context, std::string project_id, std::string card_id, const CardUpdate& update) {
  return with_contract_errors([&] { return holder::contract::update(context, project_id, card_id, update); });
}

Card trash_card(holder_context& context, std::string project_id, std::string card_id, TrashCardChildren children) {
  return with_contract_errors([&] { return trash(context, project_id, card_id, children); });
}

Card restore_card(holder_context& context, std::string project_id, std::string card_id) {
  return with_contract_errors([&] { return restore(context, project_id, card_id); });
}

Card move_card(holder_context& context, std::string project_id, std::string card_id, const CardMove& move) {
  return with_contract_errors([&] { return holder::contract::move(context, project_id, card_id, move); });
}

Card add_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  return with_contract_errors([&] { return add_tag(context, project_id, card_id, tag); });
}

Card remove_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  return with_contract_errors([&] { return remove_tag(context, project_id, card_id, tag); });
}

std::vector<TagCount> list_project_tags(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return project_tags(context, project_id); });
}

} // namespace holder::contract
