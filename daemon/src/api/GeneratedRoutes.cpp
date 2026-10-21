#include "api/GeneratedRoutes.h"

#include "api/CardLinks.h"
#include "api/CardReferenceResolve.h"
#include "api/ProjectTags.h"
#include "api/ProviderCredentials.h"
#include "api/ProviderSettings.h"
#include "api/Rebuild.h"
#include "api/RunnerPull.h"
#include "api/generated/RuntimeRoutes.h"
#include "api/support/HttpResponses.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>

#include <exception>
#include <string>
#include <utility>

namespace holder::api {
namespace {

namespace http = boost::beast::http;

struct Context {
  holder::platform::Db& db;
  holder::llm::RunnerRegistry* registry;
  holder::index::FtsIndexer* fts;
  holder::privacy::SecretStore* secret_store;
};

Router::Response error_response(http::status status, std::string code, std::string message) {
  return support::json_response(status, rig_json::encode(ErrorResponse{
      false, {std::move(code), std::move(message)}}));
}

std::string error_code(int status) {
  switch (status) {
  case 400: return "bad_request";
  case 404: return "not_found";
  default: return "error";
  }
}

} // namespace

ErrorResponse rig_error_body(std::type_identity<ErrorResponse>, int status,
                             const std::exception& error) {
  return {false, {error_code(status), error.what()}};
}

ErrorResponse rig_error_body(std::type_identity<ErrorResponse>, int,
                             const holder::privacy::PrivacyError& error) {
  return {false, {holder::privacy::privacy_error_code_name(error.code()), error.what()}};
}

bool dispatch_generated_routes(const Router::Request& req, Router::Response& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry,
                               holder::index::FtsIndexer* fts,
                               holder::privacy::SecretStore* secret_store) {
  const auto target = req.target();
  Context context{db, registry, fts, secret_store};
  try {
    const auto generated = rig_http::dispatch(
        std::string(req.method_string()), std::string(target.data(), target.size()),
        req.body(), context);
    if (!generated) return false;
    // RIG's own request-decoding failures use its invalid_request body; keep
    // Holder's existing messages for them. Mapped exceptions arrive as ErrorResponse.
    if (generated->status == 400 && generated->body.value("code", "") == "invalid_request") {
      if (generated->operation == "list_project_tags") {
        res = error_response(http::status::not_found, "not_found", "Project not found.");
      } else if (generated->operation == "get_pull_status") {
        res = error_response(http::status::not_found, "not_found", "Runner not configured.");
      } else if (generated->operation == "delete_provider_setting") {
        res = error_response(http::status::bad_request, "bad_request", "Invalid provider.");
      } else if (generated->operation == "upsert_provider_credential" &&
                 generated->body.value("message", "") ==
                     "expected AiProviderCredentialUpsertRequest") {
        res = error_response(http::status::bad_request, "bad_request",
                             "Missing provider or api_key.");
      } else {
        res = error_response(http::status::bad_request, "bad_request",
                             generated->body.value("message", "Invalid request."));
      }
    } else {
      res = support::json_response(static_cast<http::status>(generated->status), generated->body);
    }
  } catch (const std::exception& error) {
    res = error_response(http::status::internal_server_error, "error", error.what());
  }
  return true;
}

} // namespace holder::api
