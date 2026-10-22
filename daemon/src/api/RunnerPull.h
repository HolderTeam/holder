#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

struct RIG_OPEN_RECORD RunnerPullProgress {
  long long completed;
  long long total;
  double percent;
  std::string stage;
};

struct RIG_OPEN_RECORD RunnerPullJob {
  std::string job_id;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
  std::optional<std::string> model_ref RIG_OMIT_EMPTY;
  std::string model;
  std::string status;
  bool active RIG_FIELD_DESCRIPTION("Whether the pull is still queued, downloading or verifying.");
  long long updated_at;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
  RunnerPullProgress progress;
};

struct RIG_OPEN_RECORD RunnerPullStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerPullJob data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD RunnerPullStartResponseData {
  std::string job_id;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
  std::optional<std::string> model_ref RIG_OMIT_EMPTY;
  std::string model;
  std::string status;
};

struct RIG_OPEN_RECORD RunnerPullStartResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerPullStartResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD PostAiRunnerPullRequest {
  std::string model;
  std::optional<std::string> runner_id RIG_OMIT_EMPTY;
};

struct RunnerNotConfigured : std::runtime_error {
  RunnerNotConfigured() : std::runtime_error("Runner not configured.") {}
};

struct PullJobNotFound : std::runtime_error {
  PullJobNotFound() : std::runtime_error("Pull job not found.") {}
};

RIG_ROUTE(GET, "/ai/runner/pull/{job_id}")
RIG_SUMMARY("Fetch pull job status")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", RunnerNotConfigured, PullJobNotFound)
RunnerPullStatusResponse get_pull_status(holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                         std::optional<std::string> runner_id RIG_QUERY,
                                         std::string job_id);

// The runner is configured but not answering.
struct RunnerUnavailable : CodedError {
  RunnerUnavailable() : CodedError("runner_unavailable", "Local model runner unavailable.") {}
};

RIG_ROUTE(POST, "/ai/runner/pull")
RIG_SUMMARY("Start pulling a local model")
RIG_DESCRIPTION("Pulls on runner_id, or the auto-local runner when it is omitted or empty. Follow progress with GET /ai/runner/pull/{job_id} or its events stream.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or the runner rejected the pull", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not configured", RunnerNotConfigured)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Runner unavailable (runner_unavailable)", RunnerUnavailable)
RunnerPullStartResponse post_ai_runner_pull(holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                            PostAiRunnerPullRequest request);

} // namespace holder::api
