#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::llm {
class RunnerRegistry;
struct RunnerPullJob;
} // namespace holder::llm

namespace holder::api {

struct RunnerPullProgress {
  long long completed;
  long long total;
  double percent;
  std::string stage;
};

struct RunnerPullJob {
  std::string job_id;
  std::optional<std::string> runner_id;
  std::optional<std::string> model_ref;
  std::string model;
  std::string status;
  bool active;
  long long updated_at;
  std::optional<std::optional<std::string>> error;
  RunnerPullProgress progress;
};

// A pull job's status as the status route and the pull event stream report it.
RunnerPullJob pull_job_status(const holder::llm::RunnerPullJob& job, const std::string& runner_id);

struct RunnerPullStatusResponse {
  bool ok;
  RunnerPullJob data;
};

struct RunnerPullStartResponseData {
  std::string job_id;
  std::optional<std::string> runner_id;
  std::optional<std::string> model_ref;
  std::string model;
  std::string status;
};

struct RunnerPullStartResponse {
  bool ok;
  RunnerPullStartResponseData data;
};

struct PostAiRunnerPullRequest {
  std::string model;
  std::optional<std::string> runner_id;
};

struct RunnerNotConfigured : std::runtime_error {
  RunnerNotConfigured() : std::runtime_error("Runner not configured.") {}
};

struct PullJobNotFound : std::runtime_error {
  PullJobNotFound() : std::runtime_error("Pull job not found.") {}
};

RunnerPullStatusResponse get_pull_status(holder::llm::RunnerRegistry* registry,
                                         std::optional<std::string> runner_id,
                                         std::string job_id);

// The runner is configured but not answering.
struct RunnerUnavailable : CodedError {
  RunnerUnavailable() : CodedError("runner_unavailable", "Local model runner unavailable.") {}
};

RunnerPullStartResponse post_ai_runner_pull(holder::llm::RunnerRegistry* registry,
                                            PostAiRunnerPullRequest request);

} // namespace holder::api
