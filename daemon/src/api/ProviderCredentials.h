#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

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

struct ProviderCredentialError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

RIG_ROUTE(GET, "/ai/providers/credentials")
RIG_SUMMARY("List configured cloud provider credentials")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(401, ErrorResponse, "Unauthorized")
AiProviderCredentialsListResponse list_provider_credentials(holder::platform::Db& db RIG_CONTEXT);

} // namespace holder::api
