#pragma once

#include "api/AiRunnerRead.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <functional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

struct AiRunnerCreateRequest {
  std::string name;
  std::string kind;
  std::string base_url;
  std::optional<bool> enabled;
};

struct AiRunnerUpdateRequest {
  std::optional<std::string> name;
  std::optional<std::optional<std::string>> base_url;
  std::optional<bool> enabled;
};

struct AiRunnerDeleteResponseData {
  std::string runner_id;
};

struct AiRunnerDeleteResponse {
  bool ok;
  AiRunnerDeleteResponseData data;
};

struct AiRunnerNotConfigured : std::runtime_error {
  AiRunnerNotConfigured() : std::runtime_error("Runner not configured.") {}
};

AiRunnerResponse post_ai_runners(holder::platform::Db& db,
                                 holder::llm::RunnerRegistry* registry,
                                 const std::function<std::string()>& uuid_v4,
                                 AiRunnerCreateRequest request);

AiRunnerResponse patch_ai_runners_runner_id(holder::platform::Db& db,
                                            holder::llm::RunnerRegistry* registry,
                                            std::string runner_id,
                                            AiRunnerUpdateRequest request);

AiRunnerDeleteResponse delete_ai_runners_runner_id(holder::platform::Db& db,
                                                   holder::llm::RunnerRegistry* registry,
                                                   std::string runner_id);

AiRunnerResponse post_ai_runners_runner_id_retry(holder::llm::RunnerRegistry* registry,
                                                 std::string runner_id);

} // namespace holder::api
