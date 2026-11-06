#include "api/routes/ai/runs/AiRunQueryRoutes.h"

#include "ai/AiRunRepo.h"
#include "api/AiRuns.h"
#include "api/generated/AiJson.h"
#include "api/support/RunEventStore.h"
#include "api/support/StreamServer.h"
#include "api/support/Time.h"
#include "llm/RunnerModelRef.h"

#include <boost/asio/write.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace holder::api::routes::ai::runs {

api_runtime::host_value get_ai_runs_run_id_events_stream(
    holder::platform::Db& db,
    std::shared_ptr<support::SseRegistry> streams,
    const std::string& last_event_id,
    const std::string& run_id
) {
  if (!last_event_id.empty() && !support::EventJournal::valid_cursor(last_event_id))
    throw std::invalid_argument("Invalid Last-Event-ID.");
  std::optional<holder::model::AiRun> run;
  try {
    run = holder::ai::AiRunRepo(db).get(run_id);
  } catch (const std::exception&) {
  }
  if (!run || run_id.empty()) throw AiRunNotFound();
  // A completed run survives daemon restart even when its transient chunk history
  // does not. Return its terminal state on a fresh connection; a stale cursor
  // explicitly requests a refresh rather than silently replaying a new history.
  const auto stored = support::read_run_events(run_id, last_event_id);
  // A reconnect already past the last event of a finished stream has nothing more to receive.
  if (!last_event_id.empty() && stored && stored->finished && stored->events.empty() &&
      !stored->resync_required) {
    return {204, {}};
  }
  if (!stored && (run->status == "completed" || run->status == "failed")) {
    AiRunEventData terminal{.run_id = run_id, .error = run->error};
    if (run->chosen_model) {
      terminal.model_ref = *run->chosen_model;
      const auto parsed = holder::llm::parse_runner_model_ref(*run->chosen_model);
      terminal.model = parsed ? parsed->model_name : *run->chosen_model;
      if (parsed) terminal.runner_id = parsed->runner_id;
    }
    support::append_run_event(
        run_id,
        run->status == "completed" ? static_cast<support::RunEventConstructor>(ai_events::done)
                                   : static_cast<support::RunEventConstructor>(ai_events::failed),
        std::move(terminal),
        true
    );
  }
  auto source = [run_id, cursor = last_event_id]() mutable {
    auto batch = support::read_run_events(run_id, cursor);
    if (!batch) {
      batch.emplace();
      batch->resync_required = !cursor.empty();
    } else if (cursor.empty() && batch->truncated) {
      batch->resync_required = true;
    }
    if (batch->resync_required) {
      // The history no longer covers the cursor: the client fetches the run and reconnects.
      batch->events = {support::StreamEvent{
          batch->cursor,
          ai_events::resync_required(AiStreamResyncData{"history_unavailable"})
      }};
      batch->resync_required = false;
      batch->finished = true;
      return *batch;
    }
    cursor = batch->cursor;
    return *batch;
  };
  return {200, support::serve_events(std::move(streams), std::move(source))};
}

} // namespace holder::api::routes::ai::runs
