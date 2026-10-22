#include "api/EventCursor.h"

#include "api/support/EventService.h"
#include "project/ProjectRepo.h"

namespace holder::api {

void require_change_feed_project(holder::platform::Db& db, const std::string& project_id) {
  bool exists = false;
  try {
    exists = holder::project::ProjectRepo(db).get(project_id).has_value();
  } catch (const std::exception&) {
    throw ProjectUnavailable();
  }
  if (!exists) throw EventCursorProjectNotFound();
}

EventCheckpointResponse get_events_cursor(holder::platform::Db& db,
                                          holder::api::support::EventService* events,
                                          std::optional<std::string> project_id) {
  if (project_id.has_value()) require_change_feed_project(db, *project_id);
  if (events == nullptr) throw ChangeFeedUnavailable();
  auto checkpoint = events->current_checkpoint(project_id.value_or(""));
  if (!checkpoint.has_value()) throw ChangeFeedUnavailable();
  return {true, std::move(*checkpoint)};
}

} // namespace holder::api
