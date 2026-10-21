#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD AiThreadListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiThread> data;
};

RIG_ROUTE(GET, "/ai/threads")
RIG_SUMMARY("List AI threads by project")
RIG_RESPONSE_DESCRIPTION("OK")
AiThreadListResponse get_ai_threads(holder::platform::Db& db RIG_CONTEXT,
                                    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1));

} // namespace holder::api
