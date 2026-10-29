#include "api/routes/ai/runner/AiRunnerPullEventRoutes.h"

#include "api/RunnerPull.h"
#include "api/generated/AiJson.h"
#include "api/support/StreamServer.h"
#include "llm/LocalModelRunner.h"
#include "llm/RunnerModelRef.h"

#include <boost/asio/write.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace holder::api::routes::ai::runner {

api_runtime::host_value get_ai_runner_pull_job_id_events_stream(
    holder::llm::RunnerRegistry* runner_registry,
    std::shared_ptr<support::SseRegistry> streams,
    const std::optional<std::string>& requested_runner_id,
    const std::string& last_event_id,
    const std::string& job_id
) {
  const std::string runner_id = requested_runner_id.has_value() && !requested_runner_id->empty()
                                    ? *requested_runner_id
                                    : std::string(holder::llm::RunnerRegistry::kAutoLocalRunnerId);
  auto runner = runner_registry ? runner_registry->share_client(runner_id) : nullptr;
  if (!runner) throw RunnerNotConfigured();
  if (!last_event_id.empty() && !support::EventJournal::valid_cursor(last_event_id))
    throw std::invalid_argument("Invalid Last-Event-ID.");
  const auto job = runner->get_pull(job_id);
  if (!job) throw PullJobNotFound();
  // A reconnect to a finished pull has nothing more to receive.
  if (!last_event_id.empty() && (job->status == "completed" || job->status == "failed")) {
    return {204, {}};
  }
  // Pull progress is a replaceable status snapshot, not a token/delta history, so each
  // connection has its own journal and starts with the current snapshot, reconnects included.
  auto journal = std::make_shared<support::EventJournal>();
  auto source =
      [runner, runner_id, job_id, journal, cursor = std::string(), previous = std::string()](
      ) mutable {
        if (const auto current_job = runner->get_pull(job_id)) {
          const auto status = pull_job_status(*current_job, runner_id);
          auto progress = ai_events::progress(status);
          const auto current = progress.data.dump();
          if (current != previous) {
            journal->append(std::move(progress));
            if (current_job->status == "completed") {
              journal->append(ai_events::completed(status), true);
            } else if (current_job->status == "failed") {
              journal->append(ai_events::failed(status), true);
            }
            previous = current;
          }
        }
        auto batch = journal->read(cursor);
        cursor = batch.cursor;
        return batch;
      };
  return {200, support::serve_events(std::move(streams), std::move(source))};
}

} // namespace holder::api::routes::ai::runner
