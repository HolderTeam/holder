#pragma once

// Time zones: the one platform service the calendar needs.
//
// Core asks a provider only two questions: a zone's UTC offset at an instant, and the device's
// current zone. Everything else, such as turning local midnight into an instant across
// daylight-saving changes, core works out itself in local_to_utc, so it behaves the same on
// every platform. Each provider uses the operating system's own zone data; Holder never bundles
// or maintains time-zone data.
//
// The built-in provider depends on the platform, chosen by the build: std::chrono where the
// C++ library has its time-zone database (Linux, Windows), Core Foundation on macOS, and none
// on Android, where the app registers one through holder_register_time_zone_provider.

#include <memory>
#include <optional>
#include <string>

namespace holder::platform {

class TimeZoneProvider {
 public:
  virtual ~TimeZoneProvider() = default;
  // The zone's offset from UTC, in seconds east, at utc_seconds; nullopt when the zone is unknown.
  virtual std::optional<long long> utc_offset(const std::string& zone, long long utc_seconds) = 0;
  // The device's current zone, such as "Europe/London".
  virtual std::string current_zone() = 0;
};

// This platform's own provider, or nullptr where it has none.
std::shared_ptr<TimeZoneProvider> built_in_time_zone_provider();

// Replaces the provider for this process; nullptr returns to the built-in one.
void set_time_zone_provider(std::shared_ptr<TimeZoneProvider> provider);

// The registered provider, else the built-in one. Throws when there is neither.
std::shared_ptr<TimeZoneProvider> time_zone_provider();

// The UTC instant of local_seconds (seconds since the epoch, read as wall-clock time) in zone.
// A local time that happens twice, when clocks go back, is its earlier instant. One that never
// happens, when clocks go forward, moves forward by the length of the gap, as java.time does.
// Throws std::invalid_argument for an unknown zone.
long long local_to_utc(
    TimeZoneProvider& provider,
    const std::string& zone,
    long long local_seconds
);

} // namespace holder::platform
