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
  long long updated_at;
  std::optional<std::optional<std::string>> error RIG_OMIT_EMPTY RIG_NULLABLE;
  RunnerPullProgress progress;
};

struct RIG_OPEN_RECORD RunnerPullStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  RunnerPullJob data;
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
RIG_ERROR_RESPONSE(501, ErrorResponse, "Not implemented")
RunnerPullStatusResponse get_pull_status(holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                                         std::optional<std::string> runner_id RIG_QUERY,
                                         std::string job_id);

} // namespace holder::api
