#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD AiThreadListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiThread> data;
};

struct RIG_OPEN_RECORD AiThreadGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiThread data;
};

struct AiThreadNotFound : std::runtime_error {
  AiThreadNotFound() : std::runtime_error("AI thread not found.") {}
};

RIG_ROUTE(GET, "/ai/threads")
RIG_SUMMARY("List AI threads by project")
RIG_RESPONSE_DESCRIPTION("OK")
AiThreadListResponse get_ai_threads(holder::platform::Db& db RIG_CONTEXT,
                                    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1));

RIG_ROUTE(GET, "/ai/threads/{thread_id}")
RIG_SUMMARY("Fetch an AI thread")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiThreadNotFound)
AiThreadGetResponse get_ai_thread(holder::platform::Db& db RIG_CONTEXT, std::string thread_id);

} // namespace holder::api
