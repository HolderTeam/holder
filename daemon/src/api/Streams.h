#pragma once

#include "api/AiRuns.h"
#include "api/EventCursor.h"
#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "api/RunnerPull.h"
#include "api/Schemas.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::api::support {
class EventService;
struct RequestSocket;
} // namespace holder::api::support
namespace holder::index { class FtsIndexer; }
namespace holder::llm { class RunnerRegistry; }
namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

// Event streams. Generated dispatch decodes and checks each request; the daemon then writes
// the stream: each returns a support::StreamServer in its api_runtime::host_value, or a 204 with none
// when a reconnecting client has already received everything.

struct AiStreamResyncData {
  std::string reason;
};

api_runtime::host_value post_ai_runs(
    holder::api::support::RequestSocket* socket,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::llm::RunnerRegistry* registry,
    const std::function<std::string()>& uuid_v4,
    holder::api::support::EventService* events,
    AiRunCreateRequest request);

api_runtime::host_value get_ai_runs_run_id_events(
    holder::platform::Db& db,
    holder::api::support::EventService* events,
    std::optional<std::string> last_event_id,
    std::string run_id);

api_runtime::host_value get_ai_runner_pull_job_id_events(
    holder::llm::RunnerRegistry* registry,
    holder::api::support::EventService* events,
    std::optional<std::string> runner_id,
    std::optional<std::string> last_event_id,
    std::string job_id);

api_runtime::host_value get_events(
    holder::platform::Db& db,
    holder::api::support::EventService* events,
    std::optional<std::string> project_id,
    std::optional<std::string> last_revision,
    std::optional<std::string> last_event_id);

} // namespace holder::api
