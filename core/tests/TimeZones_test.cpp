// Time zones: core's rule for turning local time into an instant, the platform's built-in provider,
// and a host's provider registered through the C API.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "platform/time_zone/TimeZones.h"

#include <holder/holder_api.h>

#include <cstring>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using holder::platform::local_to_utc;

// A London-like zone: UTC+0, and UTC+1 from 01:00Z on 29 March 2026 to 01:00Z on 25 October 2026.
constexpr long long spring_forward = 1774746000;  // 2026-03-29T01:00:00Z
constexpr long long fall_back = 1792890000;       // 2026-10-25T01:00:00Z

class FakeZones final : public holder::platform::TimeZoneProvider {
 public:
  std::optional<long long> utc_offset(const std::string& zone, long long utc_seconds) override {
    if (zone != "Test/London") return std::nullopt;
    return utc_seconds >= spring_forward && utc_seconds < fall_back ? 3600 : 0;
  }
  std::string current_zone() override { return "Test/London"; }
};

// Wall-clock time as seconds since the epoch: 2026-03-29 and 2026-10-25 start at these.
constexpr long long march_29 = 1774742400;
constexpr long long october_25 = 1792886400;
constexpr long long hour = 3600;

} // namespace

TEST_CASE("local_to_utc reads ordinary local times with the offset in force", "[time_zone]") {
  FakeZones zones;
  REQUIRE(local_to_utc(zones, "Test/London", march_29) == march_29);            // Midnight, before the change.
  REQUIRE(local_to_utc(zones, "Test/London", march_29 + 3 * hour) == march_29 + 2 * hour);  // 03:00 BST.
  REQUIRE(local_to_utc(zones, "Test/London", october_25 + 3 * hour) == october_25 + 3 * hour);
}

TEST_CASE("local_to_utc moves a skipped local time forward by the gap", "[time_zone]") {
  FakeZones zones;
  // 01:30 never happens on 29 March: clocks go from 01:00 GMT to 02:00 BST. It reads as 02:30 BST.
  REQUIRE(local_to_utc(zones, "Test/London", march_29 + hour + hour / 2) == spring_forward + hour / 2);
}

TEST_CASE("local_to_utc takes the earlier of a repeated local time", "[time_zone]") {
  FakeZones zones;
  // 01:30 happens twice on 25 October: first in BST (00:30Z), then in GMT (01:30Z).
  REQUIRE(local_to_utc(zones, "Test/London", october_25 + hour + hour / 2) == october_25 + hour / 2);
}

TEST_CASE("local_to_utc refuses an unknown zone", "[time_zone]") {
  FakeZones zones;
  REQUIRE_THROWS_AS(local_to_utc(zones, "Mars/Olympus_Mons", march_29), std::invalid_argument);
}

#if !defined(__ANDROID__)
TEST_CASE("the built-in provider uses the system's zone data", "[time_zone]") {
  const auto zones = holder::platform::built_in_time_zone_provider();
  REQUIRE(zones);
  REQUIRE(zones->utc_offset("Europe/London", spring_forward - 1) == 0);
  REQUIRE(zones->utc_offset("Europe/London", spring_forward) == 3600);
  REQUIRE(zones->utc_offset("Europe/London", fall_back) == 0);
  REQUIRE(zones->utc_offset("America/Los_Angeles", spring_forward) == -7 * 3600);
  REQUIRE_FALSE(zones->utc_offset("Mars/Olympus_Mons", 0));
  REQUIRE_FALSE(zones->current_zone().empty());
  // The same rule as the fake, on real data.
  REQUIRE(local_to_utc(*zones, "Europe/London", march_29 + hour + hour / 2) == spring_forward + hour / 2);
  REQUIRE(local_to_utc(*zones, "Europe/London", october_25 + hour + hour / 2) == october_25 + hour / 2);
}
#endif

namespace {

struct HostZones {
  int destroyed = 0;
  int offset_calls = 0;
};

int host_offset(void* user_data, const char* zone, int64_t utc_seconds, int64_t* out_offset,
                int* out_present, char**) {
  auto* host = static_cast<HostZones*>(user_data);
  ++host->offset_calls;
  if (std::strcmp(zone, "Test/Plus2") != 0) { *out_present = 0; return 0; }
  (void)utc_seconds;
  *out_offset = 2 * 3600;
  *out_present = 1;
  return 0;
}

int host_current(void*, char** out_zone, char**) {
  *out_zone = static_cast<char*>(std::malloc(std::strlen("Test/Plus2") + 1));
  if (!*out_zone) return 1;
  std::strcpy(*out_zone, "Test/Plus2");
  return 0;
}

void host_destroy(void* user_data) { ++static_cast<HostZones*>(user_data)->destroyed; }

int failing_current(void*, char**, char**) { return 1; }

} // namespace

TEST_CASE("a host's provider replaces the built-in one and is released once", "[time_zone][capi]") {
  HostZones host;
  holder_error* error = nullptr;
  holder_time_zone_provider_callbacks callbacks{host_offset, host_current};
  REQUIRE(holder_register_time_zone_provider(&callbacks, &host, host_destroy, &error) == HOLDER_OK);
  {
    const auto zones = holder::platform::time_zone_provider();
    REQUIRE(zones->current_zone() == "Test/Plus2");
    REQUIRE(local_to_utc(*zones, "Test/Plus2", 10 * hour) == 8 * hour);
    REQUIRE_THROWS_AS(local_to_utc(*zones, "Europe/London", 0), std::invalid_argument);
  }
  REQUIRE(host.offset_calls > 0);
  REQUIRE(host.destroyed == 0);

  // An active reader keeps the replaced provider alive until its last reference is released.
  auto active = holder::platform::time_zone_provider();
  holder::platform::set_time_zone_provider(nullptr);
  REQUIRE(host.destroyed == 0);
  active.reset();
  REQUIRE(host.destroyed == 1);

  // A provider missing a function is refused, and its user data still released.
  HostZones refused;
  holder_time_zone_provider_callbacks incomplete{host_offset, nullptr};
  REQUIRE(holder_register_time_zone_provider(&incomplete, &refused, host_destroy, &error) ==
          HOLDER_ERROR_INVALID_ARGUMENT);
  REQUIRE(refused.destroyed == 1);
  holder_error_destroy(error);
}

TEST_CASE("a failed host callback is distinct from an unknown zone", "[time_zone][capi]") {
  HostZones host;
  holder_time_zone_provider_callbacks callbacks{host_offset, failing_current};
  holder_error* error = nullptr;
  REQUIRE(holder_register_time_zone_provider(&callbacks, &host, host_destroy, &error) == HOLDER_OK);
  {
    const auto provider = holder::platform::time_zone_provider();
    REQUIRE_THROWS_AS(provider->current_zone(), std::runtime_error);
    REQUIRE_FALSE(provider->utc_offset("Unknown/Zone", 0));
    holder::platform::set_time_zone_provider(nullptr);
    REQUIRE(host.destroyed == 0);
  }
  REQUIRE(host.destroyed == 1);
}
