#include "api/CardWrite.h"

#include "api/support/Time.h"
#include "card/CardPaths.h"
#include "card/CardStore.h"
#include "identity/Uuid.h"
#include "project/ProjectRepo.h"

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

// Runs a card store write, giving its client failures their statuses: core reports an unknown or
// unreachable parent as "target_not_found".
template <typename Write> void write_card(Write&& write) {
  try {
    write();
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  } catch (const std::runtime_error& error) {
    if (std::string(error.what()) == "target_not_found")
      throw std::invalid_argument("Parent card not found.");
    throw;
  }
}

} // namespace

CardCreateResponse post_cards(holder::platform::Db& db,
                              holder::card::CardStore* card_store,
                              CardCreateRequest request) {
  auto& store = require_card_store(card_store);
  const auto project = holder::project::ProjectRepo(db).get(request.project_id);
  if (!project) throw std::invalid_argument("Project not found.");

  holder::model::Card card;
  card.project_id = std::move(request.project_id);
  card.card_id = holder::identity::generate_id(project->id_scheme);
  card.title = std::move(request.title);
  card.created_at = request.created_at.value_or(0);
  if (card.created_at <= 0) card.created_at = support::now_epoch_seconds();
  card.updated_at = request.updated_at.value_or(0);
  if (card.updated_at <= 0) card.updated_at = card.created_at;
  if (request.parent_card_id) card.parent_card_id = std::move(*request.parent_card_id);
  card.rel_path = holder::core::card_rel_path(card.card_id);
  if (request.rel_path && *request.rel_path && **request.rel_path != card.rel_path)
    throw std::invalid_argument("rel_path must be " + card.rel_path + ".");

  write_card([&] { store.create(card, request.content, request.sort_key); });
  return {true, {std::move(card.card_id), std::move(card.rel_path)}};
}

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

} // namespace holder::api
