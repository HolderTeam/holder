#include "api/Contract.h"
#include "capi/Context.h"
#include "git/EcdsaDerSigningCredentialProvider.h"
#include "platform/time_zone/TimeZones.h"
#include "privacy/PlatformKeyring.h"

#include <memory>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

void register_ssh_signer(holder_context* context, std::string username,
                         std::vector<unsigned char> public_key_blob,
                         std::shared_ptr<SshSigner> signer) {
  context->credential_provider = std::make_shared<holder::git::EcdsaDerSigningCredentialProvider>(
      std::move(username), std::move(public_key_blob),
      [signer = std::move(signer)](const unsigned char* data, size_t length) {
        try {
          return signer->sign(std::vector<unsigned char>(data, data + length));
        } catch (const std::exception&) {
          return std::vector<unsigned char>{};
        }
      });
}

} // namespace holder::contract
