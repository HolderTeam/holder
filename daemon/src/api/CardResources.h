#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/PrivacyErrors.h"

#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }
namespace holder::card { class CardStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceAttachmentResponseData {
  std::string card_id;
  std::string resource_id;
  bool changed;
  std::string outcome RIG_STRING_VALUES("attached", "already_attached", "detached", "not_attached");
};

struct RIG_OPEN_RECORD ResourceAttachmentResponse {
  bool ok RIG_BOOL_VALUE(true);
  ResourceAttachmentResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PostCardsCardIdResourcesRequest {
  std::string project_id RIG_MIN_LENGTH(1);
  std::string resource_id RIG_FIELD_DESCRIPTION("Complete exact project resource ID; no prefix or label lookup.") RIG_MIN_LENGTH(1);
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD DeleteCardsCardIdResourcesRequest {
  std::string project_id RIG_MIN_LENGTH(1);
  std::string resource_id RIG_FIELD_DESCRIPTION("Complete exact project resource ID; no prefix or label lookup.") RIG_MIN_LENGTH(1);
};

struct CardResourceNotFound : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct CardResourceProjectMismatch : CodedError {
  CardResourceProjectMismatch()
      : CodedError("cross_project_resource_forbidden",
                   "Card and resource must belong to the selected project.") {}
};

RIG_ROUTE(POST, "/cards/{card_id}/resources")
RIG_SUMMARY("Attach a project resource to a live card (idempotent)")
RIG_RESPONSE_DESCRIPTION("Attachment outcome; repeated requests do not change state")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card or resource not found", CardResourceNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card or resource belongs to a different project", CardResourceProjectMismatch)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
ResourceAttachmentResponse post_cards_card_id_resources(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    PostCardsCardIdResourcesRequest request);

RIG_ROUTE(DELETE, "/cards/{card_id}/resources")
RIG_SUMMARY("Detach one attachment without deleting the project resource (idempotent)")
RIG_DESCRIPTION("A resource that no longer exists can still be detached.")
RIG_RESPONSE_DESCRIPTION("Attachment outcome; repeated requests do not change state")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or a privacy error writing an encrypted card", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Live card not found", CardResourceNotFound)
RIG_ERROR_RESPONSE(422, ErrorResponse, "Card or resource belongs to a different project", CardResourceProjectMismatch)
RIG_ERROR_RESPONSE(501, ErrorResponse, "The daemon has no card store", CardStoreUnavailable)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to write an encrypted card is unavailable", support::KeyringUnavailable)
ResourceAttachmentResponse delete_cards_card_id_resources(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    holder::card::CardStore* card_store RIG_CONTEXT,
    std::string card_id,
    DeleteCardsCardIdResourcesRequest request);

} // namespace holder::api
