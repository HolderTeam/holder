#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/RunnerPull.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

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
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(503, ErrorResponse, "No local runner or configured cloud provider available")
std::string post_ai_runs(AiRunCreateRequest request);

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
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
std::string get_ai_runs_run_id_events(
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
RIG_ERROR_RESPONSE(404, ErrorResponse, "Runner not configured, or pull job not found")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
std::string get_ai_runner_pull_job_id_events(
    std::optional<std::string> runner_id RIG_QUERY,
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque SSE event ID, sent by a reconnecting client."),
    std::string job_id);

} // namespace holder::api
