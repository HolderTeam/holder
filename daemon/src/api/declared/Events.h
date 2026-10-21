#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

RIG_ROUTE(GET, "/events/cursor")
RIG_OPERATION_ID("get_event_cursor")
RIG_SUMMARY("Get a change-feed checkpoint before fetching a live snapshot")
RIG_DESCRIPTION("Fetch this checkpoint, read current entities, then subscribe using its cursor as Last-Event-ID. Replayed invalidations close the race between snapshot reads and subscription.")
RIG_RESPONSE_DESCRIPTION("Current checkpoint")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid query or cursor")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(503, ErrorResponse, "Observer unavailable or stream limit reached")
EventCheckpointResponse get_events_cursor(std::optional<std::string> project_id RIG_QUERY);

} // namespace holder::api
