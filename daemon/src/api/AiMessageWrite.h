#pragma once

#include "api/AiMessageRead.h"
#include "api/CardLinks.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "model/ConflictError.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageCreateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageCreateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageCreateRequest {
  std::optional<std::string> message_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string thread_id;
  std::string role;
  std::string source;
  std::optional<std::optional<std::string>> provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string content;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiMessageUpdateResponseData {
  std::string message_id;
};

struct RIG_OPEN_RECORD AiMessageUpdateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiMessageUpdateResponseData data;
};

struct RIG_OPEN_RECORD AiMessageUpdateRequest {
  std::optional<std::optional<std::string>> role RIG_FIELD_DESCRIPTION("Omit or send null to leave it unchanged.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> source RIG_FIELD_DESCRIPTION("Omit or send null to leave it unchanged.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> provider RIG_FIELD_DESCRIPTION("Omit to leave it unchanged; null clears it.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> model RIG_FIELD_DESCRIPTION("Omit to leave it unchanged; null clears it.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> content RIG_FIELD_DESCRIPTION("Omit or send null to leave it unchanged.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Omit to leave it unchanged.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> prompt_hash RIG_FIELD_DESCRIPTION("Omit to leave it unchanged; null clears it.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> meta_json RIG_FIELD_DESCRIPTION("Omit to leave it unchanged; null clears it.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

// The request names a thread that cannot be found.
struct AiMessageThreadNotFound : std::invalid_argument {
  AiMessageThreadNotFound() : std::invalid_argument("Thread not found.") {}
};

// The message is in the trash (or, for restore, is not).
struct AiMessageWrongState : std::invalid_argument {
  using std::invalid_argument::invalid_argument;
};

RIG_ROUTE(POST, "/ai/messages")
RIG_SUMMARY("Create an AI message")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a thread that does not exist", std::invalid_argument)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A message with this ID already exists", holder::model::ConflictError)
AiMessageCreateResponse post_ai_messages(holder::platform::Db& db RIG_CONTEXT,
                                         holder::index::FtsIndexer* fts RIG_CONTEXT,
                                         const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                         AiMessageCreateRequest request);

RIG_ROUTE(PATCH, "/ai/messages/{message_id}")
RIG_SUMMARY("Update an AI message")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a message in the trash", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
AiMessageUpdateResponse patch_ai_messages_message_id(holder::platform::Db& db RIG_CONTEXT,
                                                     holder::index::FtsIndexer* fts RIG_CONTEXT,
                                                     std::string message_id,
                                                     AiMessageUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/messages/{message_id}")
RIG_SUMMARY("Move an AI message to trash")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The message is already in the trash", AiMessageWrongState)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
AiMessageUpdateResponse delete_ai_messages_message_id(holder::platform::Db& db RIG_CONTEXT,
                                                      holder::index::FtsIndexer* fts RIG_CONTEXT,
                                                      std::string message_id);

RIG_ROUTE(POST, "/ai/messages/{message_id}/restore")
RIG_SUMMARY("Restore an AI message from trash")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The message is not in the trash", AiMessageWrongState)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
AiMessageUpdateResponse post_ai_messages_message_id_restore(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string message_id);

RIG_ROUTE(POST, "/ai/messages/{message_id}/links")
RIG_SUMMARY("Create or update a link from an AI message")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including an invalid link target", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
routes::CardLinkCreateResponse post_ai_messages_message_id_links(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string message_id,
    routes::CardLinkCreateRequest request);

RIG_ROUTE(DELETE, "/ai/messages/{message_id}/links")
RIG_SUMMARY("Delete links from an AI message")
RIG_DESCRIPTION("Provide to_card_id (and optional to_type/kind) to delete matching links. A fully specified selector without target_project_id is rejected if more than one target project matches. Provide target_project_id together with to_card_id, to_type and kind to select one qualified target. If to_card_id is omitted, deletes all outgoing links.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiMessageNotFound)
AiMessageCreateResponse delete_ai_messages_message_id_links(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string message_id,
    std::optional<CardLinkDeleteRequest> request);

} // namespace holder::api
