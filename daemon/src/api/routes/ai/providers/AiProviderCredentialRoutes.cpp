#include "api/routes/ai/providers/AiProviderCredentialRoutes.h"

#include "ai/AiProviderCredentialRepo.h"
#include "ai/AiProviderSettingRepo.h"
#include "api/support/HttpResponses.h"
#include "api/support/ProviderUtils.h"
#include "api/support/Time.h"
#include "platform/DeviceConfigStore.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace holder::api::routes::ai::providers {
namespace {

namespace http = boost::beast::http;

} // namespace

bool handle_ai_provider_credential_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::privacy::SecretStore& secret_store
) {
  static constexpr const char* kSecretService = "holder.ai_provider_credentials";
  if (path.rfind("/ai/providers/credentials/", 0) == 0 && req.method() == http::verb::delete_) {
    try {
      const std::string provider = support::normalize_provider_name(
          path.substr(std::string("/ai/providers/credentials/").size())
      );
      if (provider.empty()) {
        res =
            support::error_response(http::status::bad_request, "bad_request", "Invalid provider.");
        return true;
      }
      secret_store.remove(kSecretService, provider);
      holder::ai::AiProviderCredentialRepo repo(db);
      repo.remove(provider);
      holder::ai::AiProviderSettingRepo setting_repo(db);
      setting_repo.upsert(provider, false, support::now_epoch_seconds());
      holder::core::persist_device_config(db);
      nlohmann::json payload;
      payload["ok"] = true;
      payload["data"] = {{"provider", provider}};
      res = support::json_response(http::status::ok, payload);
      return true;
    } catch (const holder::privacy::PrivacyError& ex) {
      res = support::error_response(
          http::status::service_unavailable,
          holder::privacy::privacy_error_code_name(ex.code()),
          ex.what()
      );
      return true;
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
      return true;
    }
  }

  return false;
}

} // namespace holder::api::routes::ai::providers
