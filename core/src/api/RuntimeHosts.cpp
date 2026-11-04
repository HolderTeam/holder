#include "api/Contract.h"
#include "platform/time_zone/TimeZones.h"
#include "privacy/PlatformKeyring.h"

#include <memory>
#include <exception>
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

void register_keyring_provider(std::shared_ptr<KeyringProvider> provider) {
  using holder::privacy::PlatformKeyringSecretRef;
  const auto kind = [](const PlatformKeyringSecretRef& ref) {
    return ref.kind == holder::privacy::PlatformKeyringSecretKind::ProjectKey ? 1 : 0;
  };
  holder::privacy::platform_keyring_set_external_provider(
      [provider, kind](const PlatformKeyringSecretRef& ref) {
        holder::privacy::PlatformKeyringLookupResult result;
        try {
          result.secret = provider->lookup(kind(ref), ref.service, ref.account, ref.project_id);
        } catch (const std::exception& error) {
          result.error_message = error.what();
        }
        return result;
      },
      [provider, kind](const PlatformKeyringSecretRef& ref, const std::string& label,
                       const std::string& secret) -> std::optional<std::string> {
        try {
          provider->store(kind(ref), ref.service, ref.account, ref.project_id, label, secret);
          return std::nullopt;
        } catch (const std::exception& error) {
          return error.what();
        }
      },
      [provider, kind](const PlatformKeyringSecretRef& ref) -> std::optional<std::string> {
        try {
          provider->remove(kind(ref), ref.service, ref.account, ref.project_id);
          return std::nullopt;
        } catch (const std::exception& error) {
          return error.what();
        }
      }
  );
}

} // namespace holder::contract
