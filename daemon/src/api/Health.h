#pragma once

#include "api/AiRuntimeTypes.h"

#include <chrono>
#include <string>

namespace holder::platform { class Db; }

namespace holder::api {

struct HealthDataPrivacy {
  std::string backend;
  bool project_mode_supported;
};

struct HealthData {
  bool db_ok;
  long long uptime_ms;
  std::string api_version;
  std::string server_version;
  int pid;
  HealthDataPrivacy privacy;
};

struct HealthResponse {
  bool ok;
  HealthData data;
};

HealthResponse get_health(holder::platform::Db& db,
                          std::chrono::steady_clock::time_point started_at);

} // namespace holder::api
