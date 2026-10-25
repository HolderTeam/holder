#pragma once

#include "api/Schemas.h"
#include "api/support/EventJournal.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace holder::api::support {

struct RunEvent {
  std::string name;
  nlohmann::json data;
  std::string id;
};

struct RunEventStream {
  std::vector<RunEvent> events;
  bool finished = false;
  long long updated_at = 0;
};

// A generated constructor for one of the run events, such as rig_events::chunk.
using RunEventConstructor = rig::event<nlohmann::json> (*)(const AiRunEventData&);

// Records a run event for replay; it carries run_id whatever data says.
StreamEvent append_run_event(
    const std::string& run_id,
    RunEventConstructor make,
    AiRunEventData data,
    bool finished
);
std::optional<RunEventStream> get_run_event_stream(const std::string& run_id);
std::optional<EventBatch> read_run_events(const std::string& run_id, const std::string& after);

} // namespace holder::api::support
