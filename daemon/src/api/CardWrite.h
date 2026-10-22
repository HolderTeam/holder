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

struct CardWriteNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
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

} // namespace holder::api
