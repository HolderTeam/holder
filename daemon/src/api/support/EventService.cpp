#include "api/support/EventService.h"

#include "api/EventCursor.h"
#include "api/support/ChangeFeed.h"

#include <spdlog/spdlog.h>

#include <future>
#include <stdexcept>

namespace holder::api::support {
namespace {

std::string history_url(const std::string& project) {
  std::string encoded;
  constexpr char hex[] = "0123456789ABCDEF";
  for (const char ch : project) {
    const auto byte = static_cast<unsigned char>(ch);
    if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' || byte == '.')
      encoded.push_back(static_cast<char>(byte));
    else {
      encoded += '%';
      encoded += hex[byte >> 4];
      encoded += hex[byte & 15];
    }
  }
  return "/projects/" + encoded + "/history";
}

EventCheckpoint checkpoint(
    const std::string& cursor,
    const nlohmann::json& revisions,
    const std::string& project
) {
  auto selected = revisions;
  if (!project.empty()) selected = {{project, revisions.value(project, nlohmann::json(nullptr))}};
  EventCheckpoint result{cursor, {}, {}};
  for (const auto& [id, revision] : selected.items()) {
    result.git_revisions[id] = revision.is_string()
                                   ? std::optional<std::string>(revision.get<std::string>())
                                   : std::nullopt;
    result.history_urls[id] = history_url(id);
  }
  return result;
}

nlohmann::json checkpoint_json(const EventCheckpoint& value) {
  nlohmann::json revisions = nlohmann::json::object();
  for (const auto& [project, revision] : value.git_revisions)
    revisions[project] = revision ? nlohmann::json(*revision) : nlohmann::json(nullptr);
  return {{"cursor", value.cursor}, {"git_revisions", std::move(revisions)},
          {"history_urls", value.history_urls}};
}

api_runtime::event<nlohmann::json> resync_required(
    EventCheckpoint at,
    std::string reason,
    const std::string& revision
) {
  auto data = checkpoint_json(at);
  data["reason"] = std::move(reason);
  data["last_revision"] = revision.empty() ? nlohmann::json(nullptr) : nlohmann::json(revision);
  return {"resync_required", std::move(data)};
}

} // namespace

EventService::EventService(const std::filesystem::path& database) {
  auto ready = std::make_shared<std::promise<void>>();
  auto future = ready->get_future();
  observer_ = std::thread([database, state = state_, ready] {
    holder::platform::Db db;
    std::unique_ptr<ChangeFeed> feed;
    std::unique_lock lock(state->mutex);
    auto observe = [&] {
      try {
        if (!feed) {
          if (database.empty()) throw std::runtime_error("No database available for change feed");
          db.open(database);
          db.exec("PRAGMA query_only=ON");
          feed = std::make_unique<ChangeFeed>(db, state->changes);
        } else {
          feed->poll();
        }
        state->revisions = feed->revisions();
        state->available = true;
      } catch (const std::exception& ex) {
        if (state->available) {
          // Each subscriber's source rewrites this for its own project and revision.
          state->changes.append(resync_required(
              checkpoint(state->changes.cursor(), state->revisions, ""),
              "observer_unavailable",
              ""
          ));
          spdlog::warn("change feed observer unavailable: {}", ex.what());
        }
        state->available = false;
      }
    };
    // An unavailable projection must not take down health/recovery routes.
    observe();
    ready->set_value();
    while (!state->stopped) {
      if (state->wake.wait_for(lock, std::chrono::milliseconds(250), [&] {
            return state->stopped;
          }))
        break;
      observe();
    }
  });
  try {
    future.get();
  } catch (...) {
    observer_.join();
    throw;
  }
}

EventService::~EventService() { stop(); }

void EventService::stop() {
  std::lock_guard stop_lock(stop_mutex_);
  {
    std::lock_guard lock(state_->mutex);
    state_->stopped = true;
  }
  state_->wake.notify_all();
  streams_->stop();
  if (observer_.joinable()) observer_.join();
}

std::optional<EventCheckpoint> EventService::current_checkpoint(const std::string& project) {
  std::lock_guard lock(state_->mutex);
  if (!state_->available || state_->stopped) return std::nullopt;
  return checkpoint(state_->changes.cursor(), state_->revisions, project);
}

StreamServer EventService::subscribe(
    const std::string& project,
    const std::string& revision,
    std::string cursor
) {
  if (!revision.empty() && project.empty())
    throw std::invalid_argument("last_revision requires project_id.");
  if (!cursor.empty() && !EventJournal::valid_cursor(cursor))
    throw std::invalid_argument("Invalid Last-Event-ID.");
  {
    std::lock_guard lock(state_->mutex);
    if (!state_->available || state_->stopped) throw ChangeFeedUnavailable();
    if (cursor.empty() && revision.empty()) cursor = state_->changes.cursor();
  }
  auto source = [state = state_, project, revision, cursor, first = true]() mutable {
    std::lock_guard lock(state->mutex);
    auto batch = state->changes.read(cursor);

    if (batch.resync_required || !state->available || (!revision.empty() && cursor.empty())) {
      batch.events = {StreamEvent{
          batch.cursor,
          resync_required(
              checkpoint(batch.cursor, state->revisions, project),
              state->available ? "history_unavailable" : "observer_unavailable",
              revision
          )
      }};
      batch.resync_required = false;
      batch.finished = true;
      return batch;
    }
    std::erase_if(batch.events, [&](const StreamEvent& event) {
      return !project.empty() &&
             event.data.value("project_id", nlohmann::json(nullptr)) != project &&
             event.name != "resync_required";
    });
    if (first) {
      // Preserve the old cursor on ready; advancing it before replay was delivered
      // could silently skip events if the connection dropped during its handshake.
      batch.events.insert(
          batch.events.begin(),
          StreamEvent{
              cursor,
              {"ready", checkpoint_json(checkpoint(batch.cursor, state->revisions, project))}
          }
      );
      first = false;
    }
    for (auto& event : batch.events) {
      if (event.name == "resync_required") {
        event = StreamEvent{
            event.id,
            resync_required(
                checkpoint(batch.cursor, state->revisions, project),
                event.data.value("reason", "observer_unavailable"),
                revision
            )
        };
        batch.finished = true;
      }
    }
    cursor = batch.cursor;
    return batch;
  };
  return serve_events(streams_, std::move(source));
}

} // namespace holder::api::support
