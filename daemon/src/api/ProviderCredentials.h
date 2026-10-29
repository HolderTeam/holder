#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"
#include "privacy/PrivacyError.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct AiProviderCredential {
  std::string provider;
  bool configured;
  std::string api_key_preview;
  long long created_at;
  long long updated_at;
};

struct AiProviderCredentialsListData {
  std::vector<AiProviderCredential> providers;
};

struct AiProviderCredentialsListResponse {
  bool ok;
  AiProviderCredentialsListData data;
};

struct AiProviderCredentialUpsertRequest {
  std::string provider;
  std::string api_key;
  std::optional<std::optional<long long>> updated_at;
};

struct AiProviderCredentialUpsertResponse {
  bool ok;
  AiProviderCredential data;
};

struct AiProviderCredentialDeleteResponseData {
  std::string provider;
};

struct AiProviderCredentialDeleteResponse {
  bool ok;
  AiProviderCredentialDeleteResponseData data;
};

// A secret store failure keeps its privacy code.
struct ProviderCredentialSecretFailure : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

AiProviderCredentialsListResponse list_provider_credentials(holder::platform::Db& db);

AiProviderCredentialUpsertResponse upsert_provider_credential(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    AiProviderCredentialUpsertRequest request);

AiProviderCredentialDeleteResponse delete_provider_credential(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    std::string provider);

} // namespace holder::api
