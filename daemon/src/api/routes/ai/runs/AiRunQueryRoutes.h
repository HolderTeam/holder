#pragma once

#include "api/RigAnnotations.h"
#include "api/support/SseStream.h"
#include "platform/Db.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <functional>
#include <string>

namespace holder::api::routes::ai::runs {

// GET /ai/runs/{run_id}/events after generated dispatch has decoded it: a 204 when a
// reconnect is past a finished run's last event, otherwise a support::StreamServer.
rig::host_value get_ai_runs_run_id_events_stream(
    holder::platform::Db& db,
    std::shared_ptr<holder::api::support::SseRegistry> streams,
    const std::string& last_event_id,
    const std::string& run_id
);

} // namespace holder::api::routes::ai::runs
