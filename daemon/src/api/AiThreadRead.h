#pragma once

#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct AiThreadListResponse {
  bool ok;
  std::vector<AiThread> data;
};

struct AiThreadGetResponse {
  bool ok;
  AiThread data;
};

struct AiThreadNotFound : std::runtime_error {
  AiThreadNotFound() : std::runtime_error("AI thread not found.") {}
};

AiThreadListResponse get_ai_threads(holder::platform::Db& db,
                                    std::string project_id);

AiThreadGetResponse get_ai_thread(holder::platform::Db& db, std::string thread_id);

} // namespace holder::api
