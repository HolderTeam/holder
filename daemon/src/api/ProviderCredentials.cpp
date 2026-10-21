#include "api/ProviderCredentials.h"

#include "ai/AiProviderCredentialRepo.h"
#include "api/support/ProviderUtils.h"

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

} // namespace holder::api
