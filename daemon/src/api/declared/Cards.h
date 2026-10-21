#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardCreateResponseData {
  std::string card_id;
  std::string rel_path;
};

struct RIG_OPEN_RECORD CardCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardCreateResponseData data;
};

struct RIG_OPEN_RECORD CardCreateRequest {
  std::string project_id;
  std::string title;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD CardData {
  std::string card_id;
  std::string project_id;
  std::string title;
  std::string rel_path;
  double sort_key;
  long long created_at;
  long long updated_at;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<long long>> deleted_at RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
};

struct RIG_OPEN_RECORD CardGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD CardPatchResponseData {
  std::string card_id;
};

struct RIG_OPEN_RECORD CardPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  CardPatchResponseData data;
};

struct RIG_OPEN_RECORD CardUpdateRequest {
  std::optional<std::string> content RIG_OMIT_EMPTY;
  long long updated_at;
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> parent_card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<double> sort_key RIG_OMIT_EMPTY;
};

RIG_ROUTE(POST, "/cards")
RIG_SUMMARY("Create a card")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Conflict")
CardCreateResponse post_cards(CardCreateRequest request);

RIG_ROUTE(GET, "/cards/{card_id}")
RIG_SUMMARY("Fetch a card")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardGetResponse get_cards_card_id(std::string card_id);

RIG_ROUTE(PATCH, "/cards/{card_id}")
RIG_SUMMARY("Update card content and/or title")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse patch_cards_card_id(std::string card_id, CardUpdateRequest request);

RIG_ROUTE(POST, "/cards/{card_id}/restore")
RIG_SUMMARY("Restore a card from trash")
RIG_DESCRIPTION("Restores this card only. It returns to its saved parent when that parent is live and reachable, otherwise to its nearest live, reachable ancestor or the project's top level. Children promoted when it was trashed stay where they are; descendants trashed with it stay in Trash, and reattach to it if restored afterwards.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
CardPatchResponse post_cards_card_id_restore(std::string card_id);

} // namespace holder::api
