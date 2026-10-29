#pragma once

#include "api/AiRuns.h"
#include "api/EventCursor.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
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
// the stream: each returns a support::StreamServer in its rig::host_value, or a 204 with none
// when a reconnecting client has already received everything.

struct RIG_OPEN_RECORD AiStreamResyncData {
  std::string reason RIG_STRING_VALUES("history_unavailable") RIG_FIELD_DESCRIPTION("The stream's retained history no longer covers Last-Event-ID, for example after a daemon restart: fetch the run, then reconnect without Last-Event-ID.");
};

RIG_ROUTE(POST, "/ai/runs")
RIG_SUMMARY("Create and execute an AI run with routing and streaming")
RIG_DESCRIPTION("Creates one run and streams run_started (with run_id), provider-dependent progress/router/fallback/chunk events, then done or failed. Every frame has an opaque replay ID. Reconnect via GET /ai/runs/{run_id}/events rather than repeating this POST. Disconnecting does not cancel execution; terminal events follow persisted status.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("run_started", AiRunEventData)
RIG_SSE_EVENT("progress", AiRunEventData)
RIG_SSE_EVENT("router", AiRunEventData)
RIG_SSE_EVENT("router_result", AiRunEventData)
RIG_SSE_EVENT("chunk", AiRunEventData)
RIG_SSE_EVENT("fallback", AiRunEventData)
RIG_SSE_EVENT("done", AiRunEventData)
RIG_SSE_EVENT("failed", AiRunEventData)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", std::invalid_argument)
RIG_ERROR_RESPONSE(503, ErrorResponse, "No local runner or configured cloud provider available, or stream limit reached", AiRunUnavailable)
rig::host_value post_ai_runs(
    holder::api::support::RequestSocket* socket RIG_CONTEXT,
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
    holder::api::support::EventService* events RIG_CONTEXT,
    AiRunCreateRequest request);

RIG_ROUTE(GET, "/ai/runs/{run_id}/events")
RIG_SUMMARY("Stream run events (SSE)")
RIG_DESCRIPTION("Replays bounded retained run events with absolute event IDs; done or failed closes the stream. Missing in-memory history can recover persisted terminal status. An expired or foreign cursor emits resync_required; fetch the run before reconnecting. A reconnect after the stream's last event is answered with 204, the event-stream standard's signal to stop reconnecting.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("run_started", AiRunEventData)
RIG_SSE_EVENT("progress", AiRunEventData)
RIG_SSE_EVENT("router", AiRunEventData)
RIG_SSE_EVENT("router_result", AiRunEventData)
RIG_SSE_EVENT("chunk", AiRunEventData)
RIG_SSE_EVENT("fallback", AiRunEventData)
RIG_SSE_EVENT("done", AiRunEventData)
RIG_SSE_EVENT("failed", AiRunEventData)
RIG_SSE_EVENT("resync_required", AiStreamResyncData)
RIG_SUCCESS_RESPONSE(204, "The run has finished and Last-Event-ID is past its last event: stop reconnecting")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", AiRunNotFound)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor", std::invalid_argument)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
rig::host_value get_ai_runs_run_id_events(
    holder::platform::Db& db RIG_CONTEXT,
    holder::api::support::EventService* events RIG_CONTEXT,
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque SSE event ID; unavailable history emits resync_required and closes."),
    std::string run_id);

RIG_ROUTE(GET, "/ai/runner/pull/{job_id}/events")
RIG_SUMMARY("Stream pull job progress (SSE)")
RIG_DESCRIPTION("Streams the job's progress snapshot whenever it changes; completed or failed closes the stream. A reconnect receives the current snapshot, and once the pull has finished it is answered with 204, the event-stream standard's signal to stop reconnecting. Disconnecting does not cancel the pull.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("progress", RunnerPullJob)
RIG_SSE_EVENT("completed", RunnerPullJob)
RIG_SSE_EVENT("failed", RunnerPullJob)
RIG_SUCCESS_RESPONSE(204, "The pull has finished: stop reconnecting")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not configured, or pull job not found", RunnerNotConfigured, PullJobNotFound)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor", std::invalid_argument)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
rig::host_value get_ai_runner_pull_job_id_events(
    holder::llm::RunnerRegistry* registry RIG_CONTEXT,
    holder::api::support::EventService* events RIG_CONTEXT,
    std::optional<std::string> runner_id RIG_QUERY,
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque SSE event ID, sent by a reconnecting client."),
    std::string job_id);

rig::host_value get_events(
    holder::platform::Db& db RIG_CONTEXT,
    holder::api::support::EventService* events RIG_CONTEXT,
    std::optional<std::string> project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> last_revision RIG_QUERY RIG_PATTERN("^[0-9a-f]{40}$") RIG_PARAMETER_DESCRIPTION("Last-known project Git revision for recovery; requires project_id. An unavailable replay explicitly requires refresh, even when HEAD is unchanged."),
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque cursor from a previous frame or /events/cursor."));

} // namespace holder::api
