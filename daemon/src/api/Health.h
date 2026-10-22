#pragma once

#include "api/RigAnnotations.h"

#include <chrono>
#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD HealthDataPrivacy {
  std::string backend RIG_FIELD_DESCRIPTION("Encryption used for encrypted projects.");
  bool project_mode_supported RIG_FIELD_DESCRIPTION("Whether this daemon supports per-project encryption.");
};

struct RIG_OPEN_RECORD HealthData {
  bool db_ok;
  long long uptime_ms RIG_FIELD_DESCRIPTION("Milliseconds since the HTTP server started.");
  std::string api_version;
  std::string server_version;
  int pid;
  HealthDataPrivacy privacy;
};

struct RIG_OPEN_RECORD HealthResponse {
  bool ok RIG_BOOL_VALUE(true);
  HealthData data;
};

RIG_ROUTE(GET, "/health")
RIG_SUMMARY("Health check")
RIG_RESPONSE_DESCRIPTION("OK")
HealthResponse get_health(holder::platform::Db& db RIG_CONTEXT,
                          std::chrono::steady_clock::time_point started_at RIG_CONTEXT);

} // namespace holder::api
