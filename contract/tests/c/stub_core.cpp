// A stand-in for core: the three Card operations over an in-memory map, enough to compile and run
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

Card get_card(holder_context& context, std::string card_id) {
  auto it = context.cards.find(card_id);
  if (it == context.cards.end() || it->second.deleted_at) throw NotFound("card not found: " + card_id);
  return it->second;
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

Card create_card(holder_context& context, const NewCard& request) {
  if (request.project_id != "p1") throw NotFound("project not found: " + request.project_id);
  if (request.parent_card_id && !context.cards.contains(*request.parent_card_id))
    throw NotFound("parent not found: " + *request.parent_card_id);
  Card card;
  card.card_id = "c" + std::to_string(context.next_id++);
  card.project_id = request.project_id;
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

}  // namespace holder::contract

extern "C" holder_context* stub_context_new(void) { return new holder_context; }
extern "C" void stub_context_free(holder_context* context) { delete context; }
extern "C" const char* holder_error_message(const holder_error* error) { return error ? error->message.c_str() : ""; }
extern "C" void holder_error_destroy(holder_error* error) { delete error; }
