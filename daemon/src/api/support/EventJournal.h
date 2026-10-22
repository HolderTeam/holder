#pragma once

#include "api/RigAnnotations.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace holder::api::support {

// An event as sent: its ID and a declared event, built with the generated rig_events.
struct StreamEvent {
  StreamEvent(std::string event_id, rig::event<nlohmann::json> event)
      : id(std::move(event_id)),
        name(std::move(event.name)),
        data(std::move(event.data)) {}
  std::string id;
  std::string name;
  nlohmann::json data;
};

struct EventBatch {
  std::vector<StreamEvent> events;
  std::string cursor;
  bool resync_required = false;
  bool finished = false;
  bool truncated = false;
};

// Process-local replay. Cursors identify a journal incarnation and an absolute
// sequence, so trimming storage never changes the meaning of a reader's cursor.
class EventJournal {
 public:
  explicit EventJournal(std::size_t capacity = 512, std::size_t byte_limit = 1024 * 1024);
  StreamEvent append(rig::event<nlohmann::json> event, bool finished = false);
  EventBatch read(const std::string& after = "") const;
  std::string cursor() const;
  static bool valid_cursor(const std::string& cursor);

 private:
  mutable std::mutex mutex_;
  std::string epoch_;
  std::uint64_t sequence_ = 0;
  std::uint64_t oldest_sequence_ = 1;
  std::deque<StreamEvent> events_;
  std::deque<std::size_t> sizes_;
  std::size_t bytes_ = 0;
  std::size_t capacity_;
  std::size_t byte_limit_;
  bool finished_ = false;
};

std::string encode_sse(const StreamEvent& event);

} // namespace holder::api::support
