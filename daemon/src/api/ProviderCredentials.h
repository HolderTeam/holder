#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "privacy/PrivacyError.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct RIG_OPEN_RECORD AiProviderCredential {
  std::string provider;
  bool configured;
  std::string api_key_preview;
  long long created_at;
  long long updated_at;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCredentialsListData {
  std::vector<AiProviderCredential> providers;
};

struct RIG_OPEN_RECORD AiProviderCredentialsListResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCredentialsListData data;
};

struct RIG_OPEN_RECORD AiProviderCredentialUpsertRequest {
  std::string provider;
  std::string api_key;
  std::optional<std::optional<long long>> updated_at RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD AiProviderCredentialUpsertResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCredential data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD AiProviderCredentialDeleteResponseData {
  std::string provider;
};

struct RIG_OPEN_RECORD AiProviderCredentialDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  AiProviderCredentialDeleteResponseData data;
};

struct ProviderCredentialError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

RIG_ROUTE(GET, "/ai/providers/credentials")
RIG_SUMMARY("List configured cloud provider credentials")
RIG_RESPONSE_DESCRIPTION("OK")
AiProviderCredentialsListResponse list_provider_credentials(holder::platform::Db& db RIG_CONTEXT);

RIG_ROUTE(PUT, "/ai/providers/credentials")
RIG_SUMMARY("Create or update a cloud provider API key")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", ProviderCredentialError)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Secret store unavailable",
                   holder::privacy::PrivacyError)
AiProviderCredentialUpsertResponse upsert_provider_credential(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    AiProviderCredentialUpsertRequest request);

// A privacy failure removing the stored key keeps its code.
struct ProviderCredentialSecretFailure : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

RIG_ROUTE(DELETE, "/ai/providers/credentials/{provider}")
RIG_SUMMARY("Delete a configured cloud provider API key")
RIG_DESCRIPTION("Removes the stored key and disables the provider. Deleting a provider with no key succeeds.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Invalid provider name", std::invalid_argument)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Secret store unavailable, or it failed removing the key", SecretStoreUnavailable, ProviderCredentialSecretFailure)
AiProviderCredentialDeleteResponse delete_provider_credential(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    std::string provider);

} // namespace holder::api
