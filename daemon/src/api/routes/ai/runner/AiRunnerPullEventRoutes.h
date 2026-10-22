#pragma once

#include "api/RigAnnotations.h"
#include "api/support/SseStream.h"
#include "llm/RunnerRegistry.h"

#include <memory>
#include <optional>
#include <string>

namespace holder::api::routes::ai::runner {

// GET /ai/runner/pull/{job_id}/events after generated dispatch has decoded it: a 204 when a
// reconnect finds the pull finished, otherwise a support::StreamServer.
rig::host_value get_ai_runner_pull_job_id_events_stream(
    holder::llm::RunnerRegistry* runner_registry,
    std::shared_ptr<holder::api::support::SseRegistry> streams,
    const std::optional<std::string>& requested_runner_id,
    const std::string& last_event_id,
    const std::string& job_id
);

} // namespace holder::api::routes::ai::runner
