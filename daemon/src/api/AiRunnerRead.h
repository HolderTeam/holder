#pragma once

#include "api/AiRuntimeTypes.h"
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

AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry);

struct AiRunnerNotFound : std::runtime_error {
  AiRunnerNotFound() : std::runtime_error("Runner not found.") {}
};

AiRunnerResponse get_ai_runner(holder::llm::RunnerRegistry* registry,
                               std::string runner_id);

} // namespace holder::api
