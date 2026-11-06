#pragma once

#include "api/Schemas.h"
#include "api/support/SseStream.h"
#include "api/support/StreamServer.h"
#include "platform/Db.h"

#include <condition_variable>
#include <filesystem>
#include <optional>
#include <thread>

namespace holder::api::support {

class EventService {
 public:
  explicit EventService(const std::filesystem::path& database);
  ~EventService();
  void stop();
  std::shared_ptr<SseRegistry> streams() const { return streams_; }
  // The checkpoint GET /events/cursor answers, for every project or only the named one;
  // nullopt while the change feed is unavailable.
  std::optional<EventCheckpoint> current_checkpoint(const std::string& project);
  // Subscribes to changes for every project or only the named one, resuming after cursor;
  // revision is the client's last-known Git revision, for recovery.
  StreamServer subscribe(const std::string& project, const std::string& revision, std::string cursor);

 private:
  struct State {
    std::mutex mutex;
    std::condition_variable wake;
    EventJournal changes{4096, 1024 * 1024};
    nlohmann::json revisions = nlohmann::json::object();
    bool stopped = false;
    bool available = true;
  };
  std::shared_ptr<State> state_ = std::make_shared<State>();
  std::shared_ptr<SseRegistry> streams_ = std::make_shared<SseRegistry>();
  std::thread observer_;
  std::mutex stop_mutex_;
};

} // namespace holder::api::support
