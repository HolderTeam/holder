#include "api/ProviderCredentials.h"

#include "ai/AiProviderCredentialRepo.h"
#include "ai/AiProviderSettingRepo.h"
#include "api/support/ProviderUtils.h"
#include "api/support/Time.h"
#include "platform/DeviceConfigStore.h"
#include "privacy/SecretStore.h"

#include <exception>
#include <utility>

namespace holder::api {
namespace {

std::string preview_for_output(const std::string& stored) {
  return stored.find('*') != std::string::npos ? stored : support::mask_api_key(stored);
}

} // namespace

AiProviderCredentialsListResponse list_provider_credentials(holder::platform::Db& db) {
  try {
    AiProviderCredentialsListData data;
    for (const auto& credential : holder::ai::AiProviderCredentialRepo(db).list()) {
      data.providers.push_back({credential.provider, true,
                                preview_for_output(credential.api_key_preview),
                                credential.created_at, credential.updated_at});
    }
    return {true, std::move(data)};
  } catch (const std::exception& error) {
    throw ProviderCredentialError(error.what());
  }
}

AiProviderCredentialUpsertResponse upsert_provider_credential(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    AiProviderCredentialUpsertRequest request) {
  static constexpr const char* kSecretService = "holder.ai_provider_credentials";
  try {
    if (!secret_store) throw ProviderCredentialError("Secret store not configured.");
    auto provider = support::normalize_provider_name(request.provider);
    if (provider.empty()) {
      throw ProviderCredentialError(
          "provider must be alphanumeric and may include '-', '_' or '.'.");
    }
    auto api_key = support::trim_ascii(request.api_key);
    if (api_key.empty()) throw ProviderCredentialError("api_key cannot be empty.");

    const long long ts = request.updated_at && *request.updated_at
                             ? **request.updated_at : support::now_epoch_seconds();
    const auto api_key_preview = support::mask_api_key(api_key);
    holder::ai::AiProviderCredentialRepo repo(db);
    const auto existing = repo.get(provider);
    const long long created_at = existing ? existing->created_at : ts;
    secret_store->set(kSecretService, provider, api_key, api_key_preview, created_at, ts);
    repo.upsert(provider, api_key_preview, created_at, ts);
    holder::ai::AiProviderSettingRepo(db).upsert(provider, true, ts);
    holder::core::persist_device_config(db);
    return {true, {std::move(provider), true, std::move(api_key_preview), created_at, ts}};
  } catch (const holder::privacy::PrivacyError&) {
    throw;
  } catch (const std::exception& error) {
    throw ProviderCredentialError(error.what());
  }
}

} // namespace holder::api
