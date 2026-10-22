#include "api/EventCursor.h"

#include "api/support/EventService.h"
#include "project/ProjectRepo.h"

namespace holder::api {

EventCheckpointResponse get_events_cursor(holder::platform::Db& db,
                                          holder::api::support::EventService* events,
                                          std::optional<std::string> project_id) {
  if (project_id.has_value() && !holder::project::ProjectRepo(db).get(*project_id)) {
    throw EventCursorProjectNotFound();
  }
  if (events == nullptr) throw ChangeFeedUnavailable();
  auto checkpoint = events->current_checkpoint(project_id.value_or(""));
  if (!checkpoint.has_value()) throw ChangeFeedUnavailable();
  return {true, std::move(*checkpoint)};
}

} // namespace holder::api
