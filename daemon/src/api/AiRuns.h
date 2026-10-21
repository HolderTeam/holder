#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD AiRunItem {
  std::string run_id;
  std::optional<std::optional<std::string>> project_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string mode;
  std::string prompt;
  std::optional<std::optional<std::string>> context_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> router_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> router_runner_id RIG_OMIT_EMPTY RIG_NULLABLE RIG_FIELD_DESCRIPTION("The runner named by router_model's reference, or null.");
  std::optional<std::optional<std::string>> ranked_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> policy_trace_json RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<nlohmann::json>> policy_trace RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> chosen_model RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> chosen_runner_id RIG_OMIT_EMPTY RIG_NULLABLE RIG_FIELD_DESCRIPTION("The runner named by chosen_model's reference, or null.");
  std::string status;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD AiRunListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<AiRunItem> data;
};

struct RIG_OPEN_RECORD AiRunGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiRunItem data;
};

struct AiRunsMissingScope : std::invalid_argument {
  AiRunsMissingScope() : std::invalid_argument("Missing project_id or thread_id.") {}
};

struct AiRunNotFound : std::runtime_error {
  AiRunNotFound() : std::runtime_error("Run not found.") {}
};

RIG_ROUTE(GET, "/ai/runs")
RIG_SUMMARY("List AI runs by project or thread")
RIG_DESCRIPTION("Needs project_id or thread_id; with both, thread_id is used.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", AiRunsMissingScope)
AiRunListResponse get_ai_runs(holder::platform::Db& db RIG_CONTEXT,
                              std::optional<std::string> project_id RIG_QUERY,
                              std::optional<std::string> thread_id RIG_QUERY);

RIG_ROUTE(GET, "/ai/runs/{run_id}")
RIG_SUMMARY("Fetch an AI run")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiRunNotFound)
AiRunGetResponse get_ai_run(holder::platform::Db& db RIG_CONTEXT, std::string run_id);

} // namespace holder::api
