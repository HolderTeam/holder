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

std::string placement_message(holder::card::CardPlacementErrorCode code) {
  using Code = holder::card::CardPlacementErrorCode;
  switch (code) {
  case Code::CardNotFound: return "Card not found.";
  case Code::CrossProjectMoveForbidden: return "Source card is in a different project.";
  case Code::TargetNotFound: return "Parent card not found.";
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
    throw std::invalid_argument(placement_message(error.code()));
  }
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
