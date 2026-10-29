#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::api::support { class EventService; }
namespace holder::platform { class Db; }

namespace holder::api {

// The change feed's observer is not running, so it has no current checkpoint.
struct ChangeFeedUnavailable : CodedError {
  ChangeFeedUnavailable() : CodedError("unavailable", "Change feed unavailable.") {}
};

struct EventCursorProjectNotFound : std::runtime_error {
  EventCursorProjectNotFound() : std::runtime_error("Project not found.") {}
};

// The project could not be looked up, as when the database is not yet initialized.
struct ProjectUnavailable : CodedError {
  ProjectUnavailable() : CodedError("unavailable", "Project unavailable.") {}
};

// Throws EventCursorProjectNotFound or ProjectUnavailable unless the project exists.
void require_change_feed_project(holder::platform::Db& db, const std::string& project_id);

EventCheckpointResponse get_events_cursor(
    holder::platform::Db& db RIG_CONTEXT,
    holder::api::support::EventService* events RIG_CONTEXT,
    std::optional<std::string> project_id RIG_QUERY RIG_MIN_LENGTH(1));

} // namespace holder::api
