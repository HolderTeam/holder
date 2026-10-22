#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/PrivacyErrors.h"
#include "model/ConflictError.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardCreateResponseData {
  std::string card_id;
  std::string rel_path;
};

struct RIG_OPEN_RECORD CardCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardCreateResponseData data;
};

struct RIG_DESCRIPTION("Holder generates the card ID, so card_id, like any other unlisted property, is rejected.") CardCreateRequest {
  std::string project_id;
  std::string title;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_FIELD_DESCRIPTION("Optional; when supplied it must be the path Holder derives from the generated card ID.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardPatchResponseData {
  std::string card_id;
};

struct RIG_OPEN_RECORD CardPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardPatchResponseData data;
};

struct RIG_OPEN_RECORD CardUpdateRequest {
  std::optional<std::string> content RIG_FIELD_DESCRIPTION("Required when title is supplied.") RIG_OMIT_EMPTY;
  long long updated_at;
  std::optional<std::optional<std::string>> title RIG_FIELD_DESCRIPTION("Null keeps the current title.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> parent_card_id RIG_FIELD_DESCRIPTION("Null moves the card to the project's top level.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD CardMoveResult {
  std::string card_id;
  std::optional<std::string> parent_card_id RIG_NULLABLE;
  double sort_key;
  long long revision;
  std::optional<std::optional<std::string>> moved_into_title RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardMoveResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardMoveResult data;
};

enum class CardMoveIntent {
  into,
  before,
  after,
  left,
  right,
  to_start,
  to_end,
  up_level,
};

struct RIG_DESCRIPTION("Unlisted properties are rejected.") CardMoveRequest {
  std::string project_id;
  CardMoveIntent intent;
  std::optional<std::optional<std::string>> target_card_id RIG_FIELD_DESCRIPTION("Required for intents into/before/after.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> parent_card_id RIG_FIELD_DESCRIPTION("Destination parent scope for intents to_start/to_end/left/right. If omitted, source card's current parent scope is used. An explicit null is the project's top level.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardTagMutationResult {
  std::string card_id;
  std::string tag RIG_FIELD_DESCRIPTION("Normalized tag without the leading");
  std::string outcome RIG_STRING_VALUES("added", "already_present", "removed", "not_present", "present_outside_editable_tag_line");
  bool changed RIG_FIELD_DESCRIPTION("True only when card content was changed.");
};

struct RIG_OPEN_RECORD CardTagMutationResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardTagMutationResult data;
};

struct RIG_OPEN_RECORD CardTagMutationRequest {
  std::string project_id RIG_FIELD_DESCRIPTION("Project that must own the live card.") RIG_MIN_LENGTH(1);
  std::string tag RIG_FIELD_DESCRIPTION("Tag without the leading") RIG_MIN_LENGTH(1);
};

struct CardWriteNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

// A request the card's state or the named project rules out, with its own error code.
struct CardWriteRejected : CodedError {
  using CodedError::CodedError;
};

struct CardWriteInvalid : CodedError {
  using CodedError::CodedError;
};

struct CardMoveTargetNotFound : CodedError {
  explicit CardMoveTargetNotFound(const std::string& message)
      : CodedError("target_not_found", message) {}
};

RIG_ROUTE(POST, "/cards")
RIG_SUMMARY("Create a card")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, unknown project or parent card, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A card with the generated ID or its file already exists", holder::model::ConflictError)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardCreateResponse post_cards(holder::platform::Db& db RIG_CONTEXT,
                              holder::card::CardStore* card_store RIG_CONTEXT,
                              CardCreateRequest request);

RIG_ROUTE(PATCH, "/cards/{card_id}")
RIG_SUMMARY("Update card content and/or title")
RIG_DESCRIPTION("Content and title are written first, then parent_card_id and sort_key, each as its own commit.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed or empty update, title without content, an unknown parent, moving a card in Trash, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found", CardWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardPatchResponse patch_cards_card_id(holder::card::CardStore* card_store RIG_CONTEXT,
                                      std::string card_id,
                                      CardUpdateRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}")
RIG_SUMMARY("Move card to trash")
RIG_DESCRIPTION("The card and every changed card are written in one commit. With children=promote (the default), the card's immediate live children take its place among its siblings (or the project's top level) in their existing order, keeping their own descendants; children already in Trash stay there. With children=trash_subtree, every live descendant goes to Trash with the card and keeps its parent there, including live cards beneath descendants that were already trashed, which keep their own trash time. Restoring is always single-card: see POST /cards/{card_id}/restore.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid children value, the card is already in Trash, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found", CardWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardPatchResponse delete_cards_card_id(
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    std::string children RIG_QUERY RIG_STRING_VALUES("promote", "trash_subtree") RIG_DEFAULT("promote") RIG_PARAMETER_DESCRIPTION("What happens to the card's live children. Any other value is rejected before anything changes."));

RIG_ROUTE(POST, "/cards/{card_id}/move")
RIG_SUMMARY("Move/reparent/reorder a card using card move intent")
RIG_DESCRIPTION("Placement is resolved and written under the project lock. When the card's new neighbours leave no room between their sort keys (tied keys, or a gap used up by repeated moves), nearby siblings are re-spaced in the same commit, keeping their order. The response describes only the moved card, so a client holding other cards' sort_key values should refresh the listing.")
RIG_RESPONSE_DESCRIPTION("Move applied")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, target_card_id missing for into/before/after (missing_target_card_id), or a privacy error writing an encrypted card", std::invalid_argument, CardWriteInvalid, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card (not_found), or target or parent card (target_not_found), not found", CardWriteNotFound, CardMoveTargetNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Invalid move: cross_project_move_forbidden, move_would_create_cycle, invalid_target, or invalid_move_intent for up_level at the top level", CardWriteRejected)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardMoveResponse post_cards_card_id_move(holder::card::CardStore* card_store RIG_CONTEXT,
                                         std::string card_id,
                                         CardMoveRequest request);

RIG_ROUTE(POST, "/cards/{card_id}/restore")
RIG_SUMMARY("Restore a card from trash")
RIG_DESCRIPTION("Restores this card only. It returns to its saved parent when that parent is live and reachable, otherwise to its nearest live, reachable ancestor or the project's top level. Children promoted when it was trashed stay where they are; descendants trashed with it stay in Trash, and reattach to it if restored afterwards.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The card is not in Trash, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Card not found", CardWriteNotFound)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardPatchResponse post_cards_card_id_restore(holder::card::CardStore* card_store RIG_CONTEXT,
                                             std::string card_id);

RIG_ROUTE(POST, "/cards/{card_id}/tags")
RIG_SUMMARY("Add a tag to a live card")
RIG_RESPONSE_DESCRIPTION("Added or already present")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, an invalid tag (invalid_tag), or a privacy error writing an encrypted card", std::invalid_argument, CardWriteInvalid, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", CardWriteNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card belongs to a different project (cross_project_tag_forbidden)", CardWriteRejected)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardTagMutationResponse post_cards_card_id_tags(holder::card::CardStore* card_store RIG_CONTEXT,
                                                std::string card_id,
                                                CardTagMutationRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/tags")
RIG_SUMMARY("Remove a tag from a live card's editable trailing tag line")
RIG_RESPONSE_DESCRIPTION("Removed, not present, or present only outside the editable tag line")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, an invalid tag (invalid_tag), or a privacy error writing an encrypted card", std::invalid_argument, CardWriteInvalid, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", CardWriteNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card belongs to a different project (cross_project_tag_forbidden)", CardWriteRejected)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
CardTagMutationResponse delete_cards_card_id_tags(holder::card::CardStore* card_store RIG_CONTEXT,
                                                  std::string card_id,
                                                  CardTagMutationRequest request);

} // namespace holder::api
