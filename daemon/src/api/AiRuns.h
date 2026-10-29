#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "api/Schemas.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct AiRunItem {
  std::string run_id;
  std::optional<std::optional<std::string>> project_id;
  std::optional<std::optional<std::string>> thread_id;
  std::optional<std::optional<std::string>> message_id;
  std::string mode;
  std::string prompt;
  std::optional<std::optional<std::string>> context_json;
  std::optional<std::optional<std::string>> router_model;
  std::optional<std::optional<std::string>> router_runner_id;
  std::optional<std::optional<std::string>> ranked_json;
  std::optional<std::optional<std::string>> policy_trace_json;
  std::optional<std::optional<nlohmann::json>> policy_trace;
  std::optional<std::optional<std::string>> chosen_model;
  std::optional<std::optional<std::string>> chosen_runner_id;
  std::string status;
  std::optional<std::optional<std::string>> error;
  long long created_at;
  long long updated_at;
};

struct AiRunListResponse {
  bool ok;
  std::vector<AiRunItem> data;
};

struct AiRunGetResponse {
  bool ok;
  AiRunItem data;
};

struct AiRunsMissingScope : std::invalid_argument {
  AiRunsMissingScope() : std::invalid_argument("Missing project_id or thread_id.") {}
};

struct AiRunNotFound : std::runtime_error {
  AiRunNotFound() : std::runtime_error("Run not found.") {}
};

// A run cannot start: no runner or cloud provider is available for it, or the daemon has
// too many streams open. The code says which.
struct AiRunUnavailable : CodedError {
  using CodedError::CodedError;
};

AiRunListResponse get_ai_runs(holder::platform::Db& db,
                              std::optional<std::string> project_id,
                              std::optional<std::string> thread_id);

AiRunGetResponse get_ai_run(holder::platform::Db& db, std::string run_id);

} // namespace holder::api
