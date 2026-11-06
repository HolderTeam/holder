#include "api/Health.h"

#include "platform/Db.h"
#include "platform/ServerInfo.h"

#ifndef CARD_SERVER_API_VERSION
#define CARD_SERVER_API_VERSION "0.0"
#endif

namespace holder::api {

HealthResponse get_health(holder::platform::Db& db,
                          std::chrono::steady_clock::time_point started_at) {
  bool db_ok = true;
  try {
    db.exec("SELECT 1;");
  } catch (...) {
    db_ok = false;
  }
  const auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - started_at)
                             .count();
  return {
      true,
      {
          db_ok,
          uptime_ms,
          CARD_SERVER_API_VERSION,
          CARD_SERVER_VERSION,
          holder::core::current_pid(),
          {"libsodium_xchacha20poly1305_ietf", true},
      },
  };
}

} // namespace holder::api
