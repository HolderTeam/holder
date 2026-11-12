#include "api/Contract.h"
#include "capi/Context.h"
#include "capi/StorageProviders.h"
#include "git/EcdsaDerSigningCredentialProvider.h"
#include "platform/time_zone/TimeZones.h"
#include "privacy/PlatformKeyring.h"
#include "resource/LocalDirectoryProvider.h"
#include "resource/StorageProvider.h"

#include <exception>
#include <map>
#include <memory>
#include <mutex>
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

holder::resource::StorageErrorCode storage_code(int code) {
  using holder::resource::StorageErrorCode;
  switch (code) {
  case 1:
    return StorageErrorCode::Authentication;
  case 2:
    return StorageErrorCode::Permission;
  case 3:
    return StorageErrorCode::Capacity;
  case 4:
    return StorageErrorCode::Integrity;
  case 5:
    return StorageErrorCode::Conflict;
  case 6:
    return StorageErrorCode::InvalidConfiguration;
  case 7:
    return StorageErrorCode::Transient;
  default:
    return StorageErrorCode::Unavailable;
  }
}

class RegisteredStorageProvider final : public holder::resource::StorageProvider {
 public:
  explicit RegisteredStorageProvider(std::shared_ptr<holder::contract::StorageProvider> provider)
      : provider_(std::move(provider)) {}
  void put(
      const std::string& key,
      const std::filesystem::path& staged,
      long long size,
      const std::string& sha256
  ) override {
    try {
      provider_->put(key, staged.string(), size, sha256);
    } catch (const StorageProviderFailure& error) {
      throw_storage(error);
    }
  }
  void get(const std::string& key, const std::filesystem::path& destination) override {
    try {
      provider_->get(key, destination.string());
    } catch (const StorageProviderFailure& error) {
      throw_storage(error);
    }
  }
  bool exists(const std::string& key) override {
    try {
      return provider_->exists(key);
    } catch (const StorageProviderFailure& error) {
      throw_storage(error);
    }
  }
  void remove(const std::string& key) override {
    try {
      provider_->remove(key);
    } catch (const StorageProviderFailure& error) {
      throw_storage(error);
    }
  }

 private:
  [[noreturn]] static void throw_storage(const StorageProviderFailure& error) {
    throw holder::resource::StorageError(storage_code(error.code), error.what());
  }
  std::shared_ptr<holder::contract::StorageProvider> provider_;
};

std::mutex& storage_mutex() {
  static std::mutex mutex;
  return mutex;
}

std::map<std::string, std::shared_ptr<holder::resource::StorageProvider>>& storage_registry() {
  static std::map<std::string, std::shared_ptr<holder::resource::StorageProvider>> registry;
  return registry;
}

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
      }, // LCOV_EXCL_LINE - GCC cleanup-only counter; normal expression is measured on adjacent
         // lines.
      [provider, kind](
          const PlatformKeyringSecretRef& ref,
          const std::string& label,
          const std::string& secret
      ) -> std::optional<std::string> {
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

void register_ssh_signer(
    holder_context* context,
    std::string username,
    std::vector<unsigned char> public_key_blob,
    std::shared_ptr<SshSigner> signer
) {
  context->credential_provider = std::make_shared<holder::git::EcdsaDerSigningCredentialProvider>(
      std::move(username),
      std::move(public_key_blob),
      [signer = std::move(signer)](const unsigned char* data, size_t length) {
        try {
          return signer->sign(std::vector<unsigned char>(data, data + length));
        } catch (const std::exception&) {
          return std::vector<unsigned char>{};
        }
      }
  );
}

void register_storage_provider(
    RegisterStorageProviderKind kind,
    std::shared_ptr<StorageProvider> provider
) {
  // Keyed by the provider name a location declares; local_directory is built in and not a kind.
  const std::string provider_name = [kind] {
    switch (kind) {
    case RegisterStorageProviderKind::S3Compatible:
      return "s3_compatible";
    case RegisterStorageProviderKind::GoogleDrive:
      return "google-drive";
    }
    throw std::invalid_argument("unknown storage provider kind");
  }();
  std::shared_ptr<holder::resource::StorageProvider> next =
      std::make_shared<RegisteredStorageProvider>(std::move(provider));
  {
    std::lock_guard lock(storage_mutex());
    storage_registry()[provider_name].swap(next);
  }
  // Cleanup may reenter registration, so release after unlocking.
}

} // namespace holder::contract

std::shared_ptr<holder::resource::StorageProvider> holder::capi::storage_provider(
    holder_context& context,
    const std::string& name
) {
  if (name == "local_directory")
    return std::make_shared<holder::resource::LocalDirectoryProvider>(
        context.data_dir / "resource-store"
    );
  std::lock_guard lock(holder::contract::storage_mutex());
  const auto it = holder::contract::storage_registry().find(name);
  if (it == holder::contract::storage_registry().end())
    throw std::runtime_error("no storage provider registered for: " + name);
  return it->second;
}

void holder::capi::clear_storage_providers() {
  std::map<std::string, std::shared_ptr<holder::resource::StorageProvider>> released;
  {
    std::lock_guard lock(holder::contract::storage_mutex());
    released.swap(holder::contract::storage_registry());
  }
  // Cleanup may reenter registration, so release after unlocking.
}
