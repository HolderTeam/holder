#include "platform/time_zone/TimeZones.h"

#include <mutex>
#include <stdexcept>
#include <utility>

namespace holder::platform {
namespace {

std::mutex& provider_mutex() {
  static std::mutex mutex;
  return mutex;
}

std::shared_ptr<TimeZoneProvider>& registered_provider() {
  static std::shared_ptr<TimeZoneProvider> provider;
  return provider;
}

long long offset(TimeZoneProvider& provider, const std::string& zone, long long utc_seconds) {
  const auto value = provider.utc_offset(zone, utc_seconds);
  if (!value) throw std::invalid_argument("unknown time zone: " + zone);
  return *value;
}

} // namespace

void set_time_zone_provider(std::shared_ptr<TimeZoneProvider> provider) {
  std::shared_ptr<TimeZoneProvider> previous;
  {
    std::lock_guard lock(provider_mutex());
    previous.swap(registered_provider());
    registered_provider() = std::move(provider);
  }
  // Release after unlocking: a host's destructor may register another provider.
}

std::shared_ptr<TimeZoneProvider> time_zone_provider() {
  {
    std::lock_guard lock(provider_mutex());
    if (registered_provider()) return registered_provider();
  }
  static const auto built_in = built_in_time_zone_provider();
  if (!built_in) throw std::runtime_error("no time zone provider is registered on this platform");
  return built_in;
}

long long local_to_utc(TimeZoneProvider& provider, const std::string& zone, long long local_seconds) {
  // A zone changes its offset at most once within a day, so the offsets a day either side are the
  // ones in force before and after any change around this local time.
  constexpr long long day = 24 * 60 * 60;
  const auto before = offset(provider, zone, local_seconds - day);
  const auto after = offset(provider, zone, local_seconds + day);
  const auto valid = [&](long long utc) { return offset(provider, zone, utc) == local_seconds - utc; };
  if (valid(local_seconds - before)) return local_seconds - before;  // Unique, or the earlier of two.
  if (valid(local_seconds - after)) return local_seconds - after;
  return local_seconds - before;  // In a gap: forward by its length.
}

} // namespace holder::platform
