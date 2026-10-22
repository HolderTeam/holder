#include "api/routes/ai/runs/AiRunQueryRoutes.h"

#include "ai/AiRunRepo.h"
#include "api/support/HttpResponses.h"
#include "api/support/RunEventStore.h"
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
namespace {

namespace http = boost::beast::http;

} // namespace

RouteDispatchResult handle_ai_runs_events_route(
    const std::string& path,
    boost::asio::ip::tcp::socket& socket,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    const std::string& last_event_id,
    std::shared_ptr<support::SseRegistry> streams
) {
  RouteDispatchResult out{.handled = true};
  const std::string prefix = "/ai/runs/";
  const std::string suffix = "/events";
  const auto run_id = path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
  if (!last_event_id.empty() && !support::EventJournal::valid_cursor(last_event_id)) {
    res =
        support::error_response(http::status::bad_request, "bad_request", "Invalid Last-Event-ID.");
    return out;
  }
  std::optional<holder::model::AiRun> run;
  try {
    holder::ai::AiRunRepo repo(db);
    run = repo.get(run_id);
  } catch (const std::exception&) {
  }
  if (!run || run_id.empty()) {
    res = support::error_response(http::status::not_found, "not_found", "Run not found.");
    return out;
  }
  // A completed run survives daemon restart even when its transient chunk history
  // does not. Return its terminal state on a fresh connection; a stale cursor
  // explicitly requests a refresh rather than silently replaying a new history.
  const auto stored = support::read_run_events(run_id, last_event_id);
  // A reconnect already past the last event of a finished stream has nothing more to receive.
  if (!last_event_id.empty() && stored && stored->finished && stored->events.empty() &&
      !stored->resync_required) {
    res = support::stream_finished_response();
    return out;
  }
  if (!stored && (run->status == "completed" || run->status == "failed")) {
    nlohmann::json terminal = {{"run_id", run_id}};
    if (run->chosen_model) {
      terminal["model_ref"] = *run->chosen_model;
      const auto parsed = holder::llm::parse_runner_model_ref(*run->chosen_model);
      terminal["model"] = parsed ? parsed->model_name : *run->chosen_model;
      if (parsed) terminal["runner_id"] = parsed->runner_id;
    }
    if (run->error) terminal["error"] = *run->error;
    support::append_run_event(
        run_id,
        run->status == "completed" ? "done" : "failed",
        terminal,
        true
    );
  }
  auto source = [run_id, cursor = last_event_id]() mutable {
    auto batch = support::read_run_events(run_id, cursor);
    if (!batch) {
      support::EventBatch result;
      result.resync_required = !cursor.empty();
      return result;
    }
    if (cursor.empty() && batch->truncated) batch->resync_required = true;
    cursor = batch->cursor;
    return *batch;
  };
  auto stream = support::SseStream::start(socket, std::move(streams), std::move(source));
  if (!stream) {
    res = support::error_response(
        http::status::service_unavailable,
        "server_busy",
        "Too many event streams."
    );
    return out;
  }
  out.streamed = true;
  return out;
}

} // namespace holder::api::routes::ai::runs
