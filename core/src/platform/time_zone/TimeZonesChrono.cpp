// The built-in provider where the C++ library has its time-zone database: Linux and Windows.
#include "platform/time_zone/TimeZones.h"

#include <chrono>
#include <stdexcept>

namespace holder::platform {
namespace {

class ChronoTimeZones final : public TimeZoneProvider {
 public:
  std::optional<long long> utc_offset(const std::string& zone, long long utc_seconds) override {
    const std::chrono::time_zone* found = nullptr;
    try {
      found = std::chrono::locate_zone(zone);
    } catch (const std::runtime_error&) {
      return std::nullopt;
    }
    const auto info = found->get_info(std::chrono::sys_seconds{std::chrono::seconds{utc_seconds}});
    return info.offset.count();
  }

  std::string current_zone() override { return std::string(std::chrono::current_zone()->name()); }
};

} // namespace

std::shared_ptr<TimeZoneProvider> built_in_time_zone_provider() { return std::make_shared<ChronoTimeZones>(); }

} // namespace holder::platform
