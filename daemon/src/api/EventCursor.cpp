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
  const auto data = events->current_checkpoint(project_id.value_or(""));
  if (!data.has_value()) throw ChangeFeedUnavailable();

  EventCheckpointResponse response{true, {data->value("cursor", ""), {}, {}}};
  for (const auto& [id, revision] : data->at("git_revisions").items()) {
    response.data.git_revisions[id] =
        revision.is_null() ? std::nullopt : std::optional<std::string>(revision.get<std::string>());
  }
  for (const auto& [id, url] : data->at("history_urls").items()) {
    response.data.history_urls[id] = url.get<std::string>();
  }
  return response;
}

} // namespace holder::api
