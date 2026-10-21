#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <string>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD RIG_INLINE_RECORD HealthDataCryptFilter {
  std::string path;
  bool exists;
  bool executable;
};

struct RIG_OPEN_RECORD HealthData {
  bool db_ok;
  long long uptime_ms;
  std::string api_version;
  std::string server_version;
  int pid;
  HealthDataCryptFilter crypt_filter;
};

struct RIG_OPEN_RECORD HealthResponse {
  bool ok RIG_BOOL_VALUE(true);
  HealthData data;
};

RIG_ROUTE(GET, "/health")
RIG_SUMMARY("Health check")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
HealthResponse get_health();

} // namespace holder::api
