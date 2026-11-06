#pragma once

// What the contract's operations in src/api/ share: the time, a card store on the context, the
// cards of a project, and the links to a target.

#include "api/Contract.h"

#include "capi/Context.h"
#include "card/CardStore.h"

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace holder::contract {

inline long long now_epoch_seconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

inline holder::card::CardStore card_store(holder_context& context) {
  return holder::card::CardStore(context.db, &context.fts, nullptr, context.git);
}

// The card, live or in the trash, when it is in the project: a card in another project is not found.
inline std::optional<holder::model::Card> card_in(holder::card::CardStore& store, const std::string& project_id,
                                                  const std::string& card_id) {
  auto card = store.get(card_id);
  if (!card || card->project_id != project_id) return std::nullopt;
  return card;
}

// A live card in the project, with nothing read but its metadata.
inline holder::model::Card live_card(holder::card::CardStore& store, const std::string& project_id,
                                     const std::string& card_id) {
  auto card = card_in(store, project_id, card_id);
  if (!card || card->deleted_at) throw NotFound("card not found: " + card_id);
  return std::move(*card);
}

// The links to a target in its own project, of every kind, from cards and AI messages, newest first; a link
// whose source is gone is never listed. Defined in Links.cpp.
std::vector<Link> local_backlinks(holder_context& context, const std::string& project_id,
                                  const std::string& target_type, const std::string& target_id, bool include_deleted);

} // namespace holder::contract
