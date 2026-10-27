// A stand-in for core: the Card operations over an in-memory map, enough to compile and run
// the generated C API end to end. Core's real implementation replaces this.
#include "Contract.h"

#include <algorithm>
#include <map>
#include <string>

struct holder_context {
  std::map<std::string, holder::contract::Card> cards;
  int next_id = 1;
};

struct holder_error {
  std::string message;
};

namespace holder::contract {

int detail::fail(holder_error** out_error, int code, const std::string& message) {
  if (out_error != nullptr) *out_error = new holder_error{message};
  return code;
}

// The card, live or in the trash, when it is in the project.
Card& card_in(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto it = context.cards.find(card_id);
  if (it == context.cards.end() || it->second.project_id != project_id) throw NotFound("card not found: " + card_id);
  return it->second;
}

Card& live_card(holder_context& context, const std::string& project_id, const std::string& card_id) {
  auto& card = card_in(context, project_id, card_id);
  if (card.deleted_at) throw NotFound("card not found: " + card_id);
  return card;
}

Card get_card(holder_context& context, std::string project_id, std::string card_id) {
  return live_card(context, project_id, card_id);
}

Page<CardListItem> list_cards(holder_context& context, std::string project_id, std::optional<ListCardsParent> parent,
                              std::optional<std::string> tag, ListCardsOrder order, std::int32_t limit,
                              std::optional<std::string> cursor, bool include_content, bool include_child_count) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  std::vector<CardListItem> matching;
  for (const auto& [id, card] : context.cards) {
    if (card.deleted_at) continue;
    if (parent && std::holds_alternative<ListCardsParentRoots>(*parent) && card.parent_card_id) continue;
    if (parent && std::holds_alternative<std::string>(*parent) && card.parent_card_id != std::get<std::string>(*parent)) continue;
    if (tag && std::find(card.tags.begin(), card.tags.end(), *tag) == card.tags.end()) continue;
    CardListItem item{card, std::nullopt, std::nullopt};
    if (include_content) item.content = card.content;
    if (include_child_count) {
      item.child_count = static_cast<std::int32_t>(std::count_if(context.cards.begin(), context.cards.end(),
          [&](const auto& other) { return other.second.parent_card_id == card.card_id; }));
    }
    matching.push_back(item);
  }
  if (order == ListCardsOrder::UpdatedDesc) std::reverse(matching.begin(), matching.end());
  std::size_t start = cursor ? std::stoul(*cursor) : 0;
  Page<CardListItem> page;
  for (std::size_t i = start; i < matching.size() && page.items.size() < static_cast<std::size_t>(limit); ++i)
    page.items.push_back(matching[i]);
  if (start + page.items.size() < matching.size()) page.next_cursor = std::to_string(start + page.items.size());
  return page;
}

Card create_card(holder_context& context, std::string project_id, const NewCard& request) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  if (request.parent_card_id && !context.cards.contains(*request.parent_card_id))
    throw NotFound("parent not found: " + *request.parent_card_id);
  Card card;
  card.card_id = "c" + std::to_string(context.next_id++);
  card.project_id = project_id;
  card.title = request.title;
  card.rel_path = "cards/" + card.card_id + ".md";
  card.parent_card_id = request.parent_card_id;
  card.sort_key = request.sort_key.value_or(0.0);
  card.created_at = request.created_at.value_or(1793000000);
  card.updated_at = request.updated_at.value_or(card.created_at);
  card.content = request.content;
  if (card.content.find("#draft") != std::string::npos) {
    card.tags = {"draft"};
    auto at = static_cast<long long>(card.content.find("#draft"));
    card.tag_occurrences = {{"draft", at, at + 6}};
  }
  context.cards[card.card_id] = card;
  return card;
}

Card update_card(holder_context& context, std::string project_id, std::string card_id, const CardUpdate& update) {
  auto& card = live_card(context, project_id, card_id);
  if (update.title) card.title = *update.title;
  if (update.content) card.content = *update.content;
  if (update.parent_card_id) card.parent_card_id = *update.parent_card_id;
  if (update.sort_key) card.sort_key = *update.sort_key;
  if (update.updated_at) card.updated_at = *update.updated_at;
  return card;
}

Card trash_card(holder_context& context, std::string project_id, std::string card_id, TrashCardChildren children) {
  auto& card = live_card(context, project_id, card_id);
  card.deleted_at = children == TrashCardChildren::TrashSubtree ? 1793000200 : 1793000100;
  return card;
}

Card restore_card(holder_context& context, std::string project_id, std::string card_id) {
  auto& card = card_in(context, project_id, card_id);
  if (!card.deleted_at) throw Conflict("the card is not in the trash: " + card_id);
  card.deleted_at.reset();
  return card;
}

Card move_card(holder_context& context, std::string project_id, std::string card_id, const CardMove& move) {
  auto& card = live_card(context, project_id, card_id);
  if (move.intent == CardMoveIntent::Into) {
    if (!move.target_card_id) throw InvalidArgument("target_card_id is required for into");
    if (*move.target_card_id == card_id) throw InvalidMove("a card cannot go under itself");
    card.parent_card_id = *move.target_card_id;
  } else if (move.intent == CardMoveIntent::UpLevel) {
    if (!card.parent_card_id) throw InvalidMove("the card is already at the top level");
    card.parent_card_id = std::nullopt;
  } else if (move.parent_card_id) {
    card.parent_card_id = *move.parent_card_id;
  }
  return card;
}


Card add_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  if (tag.front() == '#') throw InvalidArgument("not a tag: " + tag);
  auto& card = live_card(context, project_id, card_id);
  if (std::find(card.tags.begin(), card.tags.end(), tag) == card.tags.end()) {
    card.content += "\n#" + tag;
    card.tags.push_back(tag);
  }
  return card;
}

// The stand-in treats every tag as one in the card's prose.
Card remove_card_tag(holder_context& context, std::string project_id, std::string card_id, std::string tag) {
  auto& card = live_card(context, project_id, card_id);
  if (std::find(card.tags.begin(), card.tags.end(), tag) != card.tags.end())
    throw Conflict("the card carries #" + tag + " only in its prose");
  return card;
}

std::vector<TagCount> list_project_tags(holder_context& context, std::string project_id) {
  if (project_id != "p1") throw NotFound("project not found: " + project_id);
  std::map<std::string, std::int32_t> counts;
  for (const auto& [id, card] : context.cards)
    if (!card.deleted_at)
      for (const auto& tag : card.tags) ++counts[tag];
  std::vector<TagCount> out;
  for (const auto& [tag, count] : counts) out.push_back({tag, count});
  return out;
}
}  // namespace holder::contract

extern "C" holder_context* stub_context_new(void) { return new holder_context; }
extern "C" void stub_context_free(holder_context* context) { delete context; }
extern "C" const char* holder_error_message(const holder_error* error) { return error ? error->message.c_str() : ""; }
extern "C" void holder_error_destroy(holder_error* error) { delete error; }
