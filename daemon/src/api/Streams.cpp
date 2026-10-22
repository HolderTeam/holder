#include "api/Streams.h"

#include "api/generated/RuntimeJson.h"
#include "api/routes/ai/runner/AiRunnerPullEventRoutes.h"
#include "api/routes/ai/runs/AiRunPostRoute.h"
#include "api/routes/ai/runs/AiRunQueryRoutes.h"
#include "api/support/EventService.h"
#include "api/support/StreamServer.h"

namespace holder::api {
namespace {

// Streams count against the event service's limit; without one, they are unlimited.
std::shared_ptr<support::SseRegistry> registry_of(support::EventService* events) {
  return events != nullptr ? events->streams() : nullptr;
}

} // namespace

rig::host_value post_ai_runs(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::llm::RunnerRegistry* registry,
    const std::function<std::string()>& uuid_v4,
    holder::api::support::EventService* events,
    AiRunCreateRequest request
) {
  // The stream runs after dispatch returns, so it keeps its own copies of what it needs.
  return {
      200,
      support::StreamServer([&db,
                             fts,
                             secret_store,
                             registry,
                             uuid_v4,
                             streams = registry_of(events),
                             body = rig_json::encode(request).dump()](
                                boost::asio::ip::tcp::socket& socket,
                                boost::beast::http::response<boost::beast::http::string_body>& res
                            ) {
        return routes::ai::runs::execute_ai_run(
                   body,
                   res,
                   socket,
                   db,
                   fts,
                   secret_store,
                   registry,
                   uuid_v4,
                   streams
        )
            .streamed;
      })
  };
}

rig::host_value get_ai_runs_run_id_events(
    holder::platform::Db& db,
    holder::api::support::EventService* events,
    std::optional<std::string> last_event_id,
    std::string run_id
) {
  return routes::ai::runs::get_ai_runs_run_id_events_stream(
      db,
      registry_of(events),
      last_event_id.value_or(""),
      run_id
  );
}

rig::host_value get_ai_runner_pull_job_id_events(
    holder::llm::RunnerRegistry* registry,
    holder::api::support::EventService* events,
    std::optional<std::string> runner_id,
    std::optional<std::string> last_event_id,
    std::string job_id
) {
  return routes::ai::runner::get_ai_runner_pull_job_id_events_stream(
      registry,
      registry_of(events),
      runner_id,
      last_event_id.value_or(""),
      job_id
  );
}

rig::host_value get_events(
    holder::platform::Db& db,
    holder::api::support::EventService* events,
    std::optional<std::string> project_id,
    std::optional<std::string> last_revision,
    std::optional<std::string> last_event_id
) {
  if (project_id.has_value()) require_change_feed_project(db, *project_id);
  if (events == nullptr) throw ChangeFeedUnavailable();
  return {
      200,
      events->subscribe(
          project_id.value_or(""),
          last_revision.value_or(""),
          last_event_id.value_or("")
      )
  };
}

} // namespace holder::api
