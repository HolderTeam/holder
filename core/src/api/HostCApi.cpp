// Generated from contract/interface/ai/models.py, contract/interface/ai/operations.py, contract/interface/cards/errors.py, contract/interface/cards/models.py, contract/interface/cards/operations.py, contract/interface/cards/references.py, contract/interface/daemon/models.py, contract/interface/daemon/operations.py, contract/interface/documents/operations.py, contract/interface/errors.py, contract/interface/events/models.py, contract/interface/events/operations.py, contract/interface/git/models.py, contract/interface/git/operations.py, contract/interface/history/errors.py, contract/interface/history/models.py, contract/interface/history/operations.py, contract/interface/links/models.py, contract/interface/links/operations.py, contract/interface/locations/models.py, contract/interface/locations/operations.py, contract/interface/maintenance/errors.py, contract/interface/maintenance/models.py, contract/interface/maintenance/operations.py, contract/interface/maintenance/rebuild.py, contract/interface/milestones/models.py, contract/interface/milestones/operations.py, contract/interface/pagination/models.py, contract/interface/projects/home.py, contract/interface/projects/ids.py, contract/interface/projects/models.py, contract/interface/projects/operations.py, contract/interface/recovery/errors.py, contract/interface/recovery/models.py, contract/interface/recovery/operations.py, contract/interface/recovery/snapshot.py, contract/interface/resources/models.py, contract/interface/resources/operations.py, contract/interface/runtime/hosts.py, contract/interface/runtime/models.py, contract/interface/runtime/operations.py, contract/interface/search/models.py, contract/interface/search/operations.py, contract/interface/trash/errors.py, contract/interface/trash/models.py, contract/interface/trash/operations.py by compiler.
// Do not edit; change the contract, then run make -C contract update.
//
// The C callback adapters for host-implemented contract interfaces.
#include "Contract.h"
#include "holder/holder_api.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

class CTimeZoneProviderCallbacks final : public holder::contract::TimeZoneProvider {
 public:
  CTimeZoneProviderCallbacks(holder_time_zone_provider_callbacks callbacks, void* user_data, holder_destroy_fn destroy)
      : callbacks_(callbacks), user_data_(user_data), destroy_(destroy) {}
  ~CTimeZoneProviderCallbacks() override { if (destroy_) destroy_(user_data_); }

  std::optional<long long> utc_offset(const std::string& zone, long long utc_seconds) override {
    int64_t value = 0;
    int present = 0;
    char* error = nullptr;
    const int status = callbacks_.utc_offset(user_data_, zone.c_str(), utc_seconds, &value, &present, &error);
    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);
    if (status != 0) throw std::runtime_error(error ? error : "time_zone utc_offset failed");
    if (present != 0 && present != 1) throw std::runtime_error("TimeZoneProvider.utc_offset: invalid presence flag");
    return present ? std::optional<long long>(value) : std::nullopt;
  }

  std::string current_zone() override {
    char* value = nullptr;
    char* error = nullptr;
    const int status = callbacks_.current_zone(user_data_, &value, &error);
    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);
    std::unique_ptr<char, decltype(&std::free)> value_owner(value, &std::free);
    if (status != 0) throw std::runtime_error(error ? error : "time_zone current_zone failed");
    if (!value) throw std::runtime_error("TimeZoneProvider.current_zone: null string result");
    return std::string(value);
  }

 private:
  holder_time_zone_provider_callbacks callbacks_;
  void* user_data_;
  holder_destroy_fn destroy_;
};

class CKeyringProviderCallbacks final : public holder::contract::KeyringProvider {
 public:
  CKeyringProviderCallbacks(holder_keyring_provider_callbacks callbacks, void* user_data, holder_destroy_fn destroy)
      : callbacks_(callbacks), user_data_(user_data), destroy_(destroy) {}
  ~CKeyringProviderCallbacks() override { if (destroy_) destroy_(user_data_); }

  std::optional<std::string> lookup(int kind, const std::string& service, const std::string& account, const std::optional<std::string>& project_id) override {
    char* value = nullptr;
    char* error = nullptr;
    const int status = callbacks_.lookup(user_data_, kind, service.c_str(), account.c_str(), project_id ? project_id->c_str() : nullptr, &value, &error);
    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);
    std::unique_ptr<char, decltype(&std::free)> value_owner(value, &std::free);
    if (status != 0) throw std::runtime_error(error ? error : "keyring lookup failed");
    return value ? std::optional<std::string>(value) : std::nullopt;
  }

  void store(int kind, const std::string& service, const std::string& account, const std::optional<std::string>& project_id, const std::string& label, const std::string& secret) override {
    char* error = nullptr;
    const int status = callbacks_.store(user_data_, kind, service.c_str(), account.c_str(), project_id ? project_id->c_str() : nullptr, label.c_str(), secret.c_str(), &error);
    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);
    if (status != 0) throw std::runtime_error(error ? error : "keyring store failed");
  }

  void remove(int kind, const std::string& service, const std::string& account, const std::optional<std::string>& project_id) override {
    char* error = nullptr;
    const int status = callbacks_.remove(user_data_, kind, service.c_str(), account.c_str(), project_id ? project_id->c_str() : nullptr, &error);
    std::unique_ptr<char, decltype(&std::free)> error_owner(error, &std::free);
    if (status != 0) throw std::runtime_error(error ? error : "keyring remove failed");
  }

 private:
  holder_keyring_provider_callbacks callbacks_;
  void* user_data_;
  holder_destroy_fn destroy_;
};

} // namespace

extern "C" int holder_register_time_zone_provider(const holder_time_zone_provider_callbacks* callbacks, void* user_data,
                                       holder_destroy_fn destroy_user_data, holder_error** out_error) {
  if (out_error) *out_error = nullptr;
  std::shared_ptr<CTimeZoneProviderCallbacks> provider;
  try {
    provider = std::make_shared<CTimeZoneProviderCallbacks>(callbacks ? *callbacks : holder_time_zone_provider_callbacks{},
                                           user_data, destroy_user_data);
  } catch (const std::bad_alloc&) {
    if (destroy_user_data) destroy_user_data(user_data);
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  }
  if (!callbacks || callbacks->utc_offset == nullptr || callbacks->current_zone == nullptr)
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "TimeZoneProvider callbacks are required");
  try {
    holder::contract::register_time_zone_provider(provider);
    return HOLDER_OK;
  } catch (const std::bad_alloc&) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& error) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, error.what());
  } catch (...) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown holder error");
  }
}

extern "C" int holder_register_keyring_provider(const holder_keyring_provider_callbacks* callbacks, void* user_data,
                                       holder_destroy_fn destroy_user_data, holder_error** out_error) {
  if (out_error) *out_error = nullptr;
  std::shared_ptr<CKeyringProviderCallbacks> provider;
  try {
    provider = std::make_shared<CKeyringProviderCallbacks>(callbacks ? *callbacks : holder_keyring_provider_callbacks{},
                                           user_data, destroy_user_data);
  } catch (const std::bad_alloc&) {
    if (destroy_user_data) destroy_user_data(user_data);
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  }
  if (!callbacks || callbacks->lookup == nullptr || callbacks->store == nullptr || callbacks->remove == nullptr)
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_INVALID_ARGUMENT, "KeyringProvider callbacks are required");
  try {
    holder::contract::register_keyring_provider(provider);
    return HOLDER_OK;
  } catch (const std::bad_alloc&) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_ALLOCATION, "allocation failed");
  } catch (const std::exception& error) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, error.what());
  } catch (...) {
    return holder::contract::detail::fail(out_error, HOLDER_ERROR_RUNTIME, "unknown holder error");
  }
}
