#pragma once

#include "api/AiThreadRead.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "model/ConflictError.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct AiThreadCreateResponseData {
  std::string thread_id;
};

struct AiThreadCreateResponse {
  bool ok;
  AiThreadCreateResponseData data;
};

struct AiThreadCreateRequest {
  std::optional<std::string> thread_id;
  std::string project_id;
  std::string title;
  std::optional<std::optional<std::string>> card_id;
  std::optional<long long> created_at;
  std::optional<long long> updated_at;
};

struct AiThreadPatchResponseData {
  std::string thread_id;
};

struct AiThreadPatchResponse {
  bool ok;
  AiThreadPatchResponseData data;
};

struct AiThreadUpdateRequest {
  std::optional<std::optional<std::string>> title;
  std::optional<std::optional<std::string>> card_id;
  long long updated_at;
};

AiThreadCreateResponse post_ai_threads(holder::platform::Db& db,
                                       const std::function<std::string()>& uuid_v4,
                                       AiThreadCreateRequest request);

AiThreadPatchResponse patch_ai_threads_thread_id(holder::platform::Db& db,
                                                 std::string thread_id,
                                                 AiThreadUpdateRequest request);

AiThreadPatchResponse delete_ai_threads_thread_id(holder::platform::Db& db,
                                                  std::string thread_id);

} // namespace holder::api
