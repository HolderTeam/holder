#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

RIG_ROUTE(GET, "/ai/runners")
RIG_SUMMARY("List configured local model runners")
RIG_RESPONSE_DESCRIPTION("OK")
AiRunnerListResponse get_ai_runners(holder::llm::RunnerRegistry* registry RIG_CONTEXT);

} // namespace holder::api
