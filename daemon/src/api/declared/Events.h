#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; support::EventService writes this response.

RIG_ROUTE(GET, "/events")
RIG_OPERATION_ID("stream_changes")
RIG_SUMMARY("Subscribe to committed daemon state changes (SSE)")
RIG_DESCRIPTION("Sends ready, then project.changed, card.changed, resource.changed,\nlocation.changed, thread.changed, message.changed and run.changed invalidations.\nIntermediate states can coalesce. Each frame has an opaque id and JSON data.\nWithout a cursor starts at the current tail. Last-Event-ID replays bounded\nin-memory history. Restart, an expired cursor or last_revision without a\nreplayable cursor returns resync_required with current Git revisions and\nhistory URLs, then closes. This endpoint does not synthesize events from Git.\nRefresh current state on resync; Git is the durable project history.\nComments provide heartbeats. Disconnecting unsubscribes without cancelling jobs.\n")
RIG_RESPONSE_DESCRIPTION("Live SSE stream; ready or resync_required is the first event")
RIG_HOST_RESPONSE("text/event-stream")
RIG_SSE_EVENT("ready", EventCheckpoint)
RIG_SSE_EVENT("project.changed", ChangeEventData)
RIG_SSE_EVENT("card.changed", ChangeEventData)
RIG_SSE_EVENT("resource.changed", ChangeEventData)
RIG_SSE_EVENT("location.changed", ChangeEventData)
RIG_SSE_EVENT("thread.changed", ChangeEventData)
RIG_SSE_EVENT("message.changed", ChangeEventData)
RIG_SSE_EVENT("run.changed", ChangeEventData)
RIG_SSE_EVENT("resync_required", EventResyncData)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
std::string get_events(
    std::optional<std::string> project_id RIG_QUERY,
    std::optional<std::string> last_revision RIG_QUERY RIG_PATTERN("^[0-9a-f]{40}$") RIG_PARAMETER_DESCRIPTION("Last-known project Git revision for recovery; requires project_id. An unavailable replay explicitly requires refresh, even when HEAD is unchanged."),
    std::optional<std::string> last_event_id RIG_HEADER("Last-Event-ID") RIG_PARAMETER_DESCRIPTION("Opaque cursor from a previous frame or /events/cursor."));

} // namespace holder::api
