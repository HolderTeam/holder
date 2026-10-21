#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <vector>

namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

// Every registered runner with its live runtime, as the runner routes report it; empty without a
// registry.
std::vector<AiRunner> runners_with_runtime(holder::llm::RunnerRegistry* registry);

RIG_ROUTE(GET, "/ai/runners")
RIG_SUMMARY("List configured local model runners")
RIG_RESPONSE_DESCRIPTION("OK")
AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry RIG_CONTEXT);

} // namespace holder::api
