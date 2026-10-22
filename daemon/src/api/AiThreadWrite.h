#pragma once

#include "api/AiThreadRead.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "model/ConflictError.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadCreateResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadCreateResponseData data;
};

struct RIG_OPEN_RECORD AiThreadCreateRequest {
  std::optional<std::string> thread_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string project_id;
  std::string title;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiThreadPatchResponseData {
  std::string thread_id;
};

struct RIG_OPEN_RECORD AiThreadPatchResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThreadPatchResponseData data;
};

struct RIG_OPEN_RECORD AiThreadUpdateRequest {
  std::optional<std::optional<std::string>> title RIG_FIELD_DESCRIPTION("Omit or send null to leave it unchanged.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> card_id RIG_FIELD_DESCRIPTION("Omit to leave it unchanged; null detaches the thread from its card.") RIG_OMIT_EMPTY RIG_NULLABLE;
  long long updated_at;
};

RIG_ROUTE(POST, "/ai/threads")
RIG_SUMMARY("Create an AI thread")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a project or card that does not exist", std::invalid_argument)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A thread with this ID already exists", holder::model::ConflictError)
AiThreadCreateResponse post_ai_threads(holder::platform::Db& db RIG_CONTEXT,
                                       const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                       AiThreadCreateRequest request);

RIG_ROUTE(PATCH, "/ai/threads/{thread_id}")
RIG_SUMMARY("Update AI thread metadata")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a card that does not exist", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiThreadNotFound)
AiThreadPatchResponse patch_ai_threads_thread_id(holder::platform::Db& db RIG_CONTEXT,
                                                 std::string thread_id,
                                                 AiThreadUpdateRequest request);

RIG_ROUTE(DELETE, "/ai/threads/{thread_id}")
RIG_SUMMARY("Delete an AI thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiThreadNotFound)
AiThreadPatchResponse delete_ai_threads_thread_id(holder::platform::Db& db RIG_CONTEXT,
                                                  std::string thread_id);

} // namespace holder::api
