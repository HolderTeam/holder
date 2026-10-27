#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/PrivacyErrors.h"
#include "model/ConflictError.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardPatchResponseData {
  std::string card_id;
};

struct RIG_OPEN_RECORD CardPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardPatchResponseData data;
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
