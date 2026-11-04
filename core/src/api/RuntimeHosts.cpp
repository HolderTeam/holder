#include "api/Contract.h"
#include "platform/time_zone/TimeZones.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

class RegisteredTimeZoneProvider final : public holder::platform::TimeZoneProvider {
 public:
  explicit RegisteredTimeZoneProvider(std::shared_ptr<holder::contract::TimeZoneProvider> provider)
      : provider_(std::move(provider)) {}

  std::optional<long long> utc_offset(const std::string& zone, long long seconds) override {
    return provider_->utc_offset(zone, seconds);
  }

  std::string current_zone() override { return provider_->current_zone(); }

 private:
  std::shared_ptr<holder::contract::TimeZoneProvider> provider_;
};

} // namespace

void register_time_zone_provider(std::shared_ptr<TimeZoneProvider> provider) {
  holder::platform::set_time_zone_provider(
      std::make_shared<RegisteredTimeZoneProvider>(std::move(provider))
  );
}

} // namespace holder::contract
