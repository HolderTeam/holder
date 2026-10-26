#include "api/CardWrite.h"

#include "api/support/Time.h"
#include "card/CardPlacementError.h"
#include "card/CardStore.h"
#include "card/TagExtractor.h"

#include <utility>

namespace holder::api {
namespace {

holder::card::CardStore& require_card_store(holder::card::CardStore* card_store) {
  if (card_store == nullptr) throw CardStoreUnavailable{};
  return *card_store;
}

holder::model::Card find_card(holder::card::CardStore& card_store, const std::string& card_id) {
  auto card = card_store.get(card_id);
  if (!card) throw CardWriteNotFound("Card not found.");
  return std::move(*card);
}

std::string placement_message(holder::card::CardPlacementErrorCode code, bool target_style) {
  using Code = holder::card::CardPlacementErrorCode;
  switch (code) {
  case Code::CardNotFound: return "Card not found.";
  case Code::CrossProjectMoveForbidden: return "Source card is in a different project.";
  case Code::TargetNotFound: return target_style ? "Target card not found." : "Parent card not found.";
  case Code::MoveWouldCreateCycle: return "Move would create a cycle.";
  case Code::InvalidTarget: return "Target card is invalid for requested move.";
  case Code::AlreadyAtProjectRoot: return "Card is already at project root.";
  case Code::MissingTargetCardId: return "target_card_id is required for this intent.";
  case Code::InvalidMoveIntent: break;
  }
  return "Unknown move intent.";
}

// Runs a card store write, turning a privacy failure into its status and a placement the request
// rules out (an unknown parent, or one that would make a cycle) into a 400.
template <typename Write> void write_card(Write&& write) {
  try {
    write();
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  } catch (const holder::card::CardPlacementError& error) {
    throw std::invalid_argument(placement_message(error.code(), false));
  }
}

[[noreturn]] void throw_move_failure(const holder::card::CardPlacementError& error,
                                     bool target_style) {
  using Code = holder::card::CardPlacementErrorCode;
  const auto message = placement_message(error.code(), target_style);
  switch (error.code()) {
  case Code::CardNotFound: throw CardWriteNotFound(message);
  case Code::TargetNotFound: throw CardMoveTargetNotFound(message);
  case Code::MissingTargetCardId:
  case Code::InvalidMoveIntent: throw CardWriteInvalid(error.what(), message);
  case Code::AlreadyAtProjectRoot: throw CardWriteRejected("invalid_move_intent", message);
  case Code::CrossProjectMoveForbidden:
  case Code::MoveWouldCreateCycle:
  case Code::InvalidTarget: break;
  }
  throw CardWriteRejected(error.what(), message);
}

holder::card::CardPlacementIntent placement_intent(CardMoveIntent intent) {
  using Intent = holder::card::CardPlacementIntent;
  switch (intent) {
  case CardMoveIntent::into: return Intent::Into;
  case CardMoveIntent::before: return Intent::Before;
  case CardMoveIntent::after: return Intent::After;
  case CardMoveIntent::left: return Intent::Left;
  case CardMoveIntent::right: return Intent::Right;
  case CardMoveIntent::to_start: return Intent::ToStart;
  case CardMoveIntent::to_end: return Intent::ToEnd;
  case CardMoveIntent::up_level: break;
  }
  return Intent::UpLevel;
}

// The live card in project_id whose tags change.
holder::model::Card tag_card(holder::card::CardStore* card_store,
                             const std::string& card_id,
                             const std::string& project_id) {
  auto card = find_card(require_card_store(card_store), card_id);
  if (card.deleted_at) throw CardWriteNotFound("Card not found.");
  if (card.project_id != project_id)
    throw CardWriteRejected("cross_project_tag_forbidden", "Card is in a different project.");
  return card;
}

CardTagMutationResponse tag_result(std::string card_id, const std::string& tag,
                                   std::string outcome, bool changed) {
  return {true,
          {std::move(card_id), holder::core::normalize_tag(tag), std::move(outcome), changed}};
}

[[noreturn]] void throw_invalid_tag(const std::string& tag) {
  throw CardWriteInvalid("invalid_tag", "Invalid tag: " + tag + ".");
}

} // namespace

CardPatchResponse patch_cards_card_id(holder::card::CardStore* card_store,
                                      std::string card_id,
                                      CardUpdateRequest request) {
  auto& store = require_card_store(card_store);
  const bool placing = request.parent_card_id || request.sort_key;
  if (!request.content && !request.title && !placing)
    throw std::invalid_argument("No updatable fields supplied.");
  if (request.title && !request.content)
    throw std::invalid_argument("content is required when updating title.");
  const auto card = find_card(store, card_id);
  if (placing && card.deleted_at) throw std::invalid_argument("Card is in Trash.");

  write_card([&] {
    if (request.content) {
      store.update_content(card_id, *request.content, request.title.value_or(std::nullopt),
                           request.updated_at);
    }
    if (placing) {
      store.move(card_id, request.parent_card_id.has_value(),
                 request.parent_card_id.value_or(std::nullopt), request.sort_key,
                 request.updated_at);
    }
  });
  return {true, {std::move(card_id)}};
}

CardPatchResponse delete_cards_card_id(holder::card::CardStore* card_store,
                                       std::string card_id,
                                       std::string children) {
  auto& store = require_card_store(card_store);
  if (find_card(store, card_id).deleted_at)
    throw std::invalid_argument("Card is already in Trash.");
  const auto mode = children == "trash_subtree" ? holder::card::TrashChildren::TrashSubtree
                                                : holder::card::TrashChildren::Promote;
  write_card([&] { store.trash(card_id, support::now_epoch_seconds(), mode); });
  return {true, {std::move(card_id)}};
}

CardMoveResponse post_cards_card_id_move(holder::card::CardStore* card_store,
                                         std::string card_id,
                                         CardMoveRequest request) {
  auto& store = require_card_store(card_store);
  holder::card::CardPlacementRequest placement;
  placement.intent = placement_intent(request.intent);
  placement.target_card_id = request.target_card_id.value_or(std::nullopt);
  // An omitted parent_card_id keeps the card's current parent; null is the project's top level.
  placement.has_parent_card_id = request.parent_card_id.has_value();
  placement.parent_card_id = request.parent_card_id.value_or(std::nullopt);

  holder::card::CardMoveResult result;
  try {
    result = store.move_to(request.project_id, card_id, placement, support::now_epoch_seconds());
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  } catch (const holder::card::CardPlacementError& error) {
    const bool target_style = request.intent == CardMoveIntent::into ||
                              request.intent == CardMoveIntent::before ||
                              request.intent == CardMoveIntent::after;
    throw_move_failure(error, target_style);
  }
  auto& moved = result.card;
  return {true,
          {std::move(moved.card_id), std::move(moved.parent_card_id), moved.sort_key,
           moved.updated_at,
           std::optional<std::optional<std::string>>(std::move(result.moved_into_title))}};
}

CardPatchResponse post_cards_card_id_restore(holder::card::CardStore* card_store,
                                             std::string card_id) {
  auto& store = require_card_store(card_store);
  if (!find_card(store, card_id).deleted_at) throw std::invalid_argument("Card is not in Trash.");
  write_card([&] { store.restore(card_id, support::now_epoch_seconds()); });
  return {true, {std::move(card_id)}};
}

CardTagMutationResponse post_cards_card_id_tags(holder::card::CardStore* card_store,
                                                std::string card_id,
                                                CardTagMutationRequest request) {
  tag_card(card_store, card_id, request.project_id);
  holder::card::AddTagResult result{};
  write_card([&] {
    result = card_store->add_tag(card_id, request.tag, support::now_epoch_seconds());
  });
  switch (result) {
  case holder::card::AddTagResult::Added:
    return tag_result(std::move(card_id), request.tag, "added", true);
  case holder::card::AddTagResult::AlreadyPresent:
    return tag_result(std::move(card_id), request.tag, "already_present", false);
  case holder::card::AddTagResult::InvalidTag: break;
  }
  throw_invalid_tag(request.tag);
}

CardTagMutationResponse delete_cards_card_id_tags(holder::card::CardStore* card_store,
                                                  std::string card_id,
                                                  CardTagMutationRequest request) {
  tag_card(card_store, card_id, request.project_id);
  holder::card::RemoveTagResult result{};
  write_card([&] {
    result = card_store->remove_tag(card_id, request.tag, support::now_epoch_seconds());
  });
  switch (result) {
  case holder::card::RemoveTagResult::Removed:
    return tag_result(std::move(card_id), request.tag, "removed", true);
  case holder::card::RemoveTagResult::NotPresent:
    return tag_result(std::move(card_id), request.tag, "not_present", false);
  case holder::card::RemoveTagResult::PresentOutsideEditableTagLine:
    return tag_result(std::move(card_id), request.tag, "present_outside_editable_tag_line", false);
  case holder::card::RemoveTagResult::InvalidTag: break;
  }
  throw_invalid_tag(request.tag);
}

} // namespace holder::api
