#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::llm { class RunnerRegistry; }
namespace holder::model { struct AiRunner; }

namespace holder::api {

// One runner with its live runtime, which reports it unconfigured without a registry.
AiRunner runner_with_runtime(const holder::model::AiRunner& runner,
                             holder::llm::RunnerRegistry* registry);

// Every registered runner with its live runtime, as the runner routes report it; empty without a
// registry.
std::vector<AiRunner> runners_with_runtime(holder::llm::RunnerRegistry* registry);

RIG_ROUTE(GET, "/ai/runners")
RIG_SUMMARY("List configured local model runners")
RIG_RESPONSE_DESCRIPTION("OK")
AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry RIG_CONTEXT);

struct AiRunnerNotFound : std::runtime_error {
  AiRunnerNotFound() : std::runtime_error("Runner not found.") {}
};

RIG_ROUTE(GET, "/ai/runners/{runner_id}")
RIG_SUMMARY("Get a local model runner")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiRunnerNotFound)
AiRunnerResponse get_ai_runner(holder::llm::RunnerRegistry* registry RIG_CONTEXT,
                               std::string runner_id);

} // namespace holder::api
