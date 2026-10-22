#include "api/GeneratedRoutes.h"

#include "api/AiLocalModelConfig.h"
#include "api/AiMessageRead.h"
#include "api/AiMessageWrite.h"
#include "api/AiProviderCatalog.h"
#include "api/AiRunnerRead.h"
#include "api/AssetImportRead.h"
#include "api/AiRuns.h"
#include "api/AiStatus.h"
#include "api/AiThreadRead.h"
#include "api/AiThreadWrite.h"
#include "api/Alive.h"
#include "api/Calendar.h"
#include "api/CardMilestones.h"
#include "api/CardRead.h"
#include "api/CardResources.h"
#include "api/CardWrite.h"
#include "api/CardLinks.h"
#include "api/CardReferenceResolve.h"
#include "api/Health.h"
#include "api/History.h"
#include "api/LocationRead.h"
#include "api/LocationWrite.h"
#include "api/MilestonesWithDates.h"
#include "api/ProjectCreate.h"
#include "api/ProjectDelete.h"
#include "api/ProjectRead.h"
#include "api/ProjectCards.h"
#include "api/ProjectUpdate.h"
#include "api/ProjectTags.h"
#include "api/ProviderCredentials.h"
#include "api/ProviderSettings.h"
#include "api/Rebuild.h"
#include "api/ResourceRead.h"
#include "api/ResourceWrite.h"
#include "api/RunnerPull.h"
#include "api/Search.h"
#include "api/TrashList.h"
#include "api/generated/RuntimeRoutes.h"
#include "api/support/HttpResponses.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>

#include <chrono>
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
  holder::git::GitOps* git_ops;
  std::function<std::string()> uuid_v4;
  std::chrono::steady_clock::time_point started_at;
  holder::card::CardStore* card_store;
};

std::string error_code(int status) {
  switch (status) {
  case 400: return "bad_request";
  case 404: return "not_found";
  case 405: return "method_not_allowed";
  case 409: return "conflict";
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

ErrorResponse rig_error_body(std::type_identity<ErrorResponse>, int,
                             const CodedError& error) {
  return {false, {error.code, error.what()}};
}

bool is_public_generated_route(const Router::Request& req) {
  const auto target = req.target();
  return rig_http::is_public(std::string(req.method_string()),
                             std::string(target.data(), target.size()));
}

bool dispatch_generated_routes(const Router::Request& req, Router::Response& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry,
                               holder::index::FtsIndexer* fts,
                               holder::privacy::SecretStore* secret_store,
                               holder::git::GitOps* git_ops,
                               std::function<std::string()> uuid_v4,
                               std::chrono::steady_clock::time_point started_at,
                               holder::card::CardStore* card_store) {
  const auto target = req.target();
  Context context{
      db, registry, fts, secret_store, git_ops, std::move(uuid_v4), started_at, card_store};
  try {
    const auto generated = rig_http::dispatch(
        std::string(req.method_string()), std::string(target.data(), target.size()),
        req.body(), context);
    if (!generated) return false;
    res = support::json_response(static_cast<http::status>(generated->status), generated->body);
    if (!generated->allow.empty()) res.set(http::field::allow, generated->allow);
  } catch (const std::exception& error) {
    // Operations' own failures are already 500s from the generated dispatcher.
    // This only keeps a failure outside them from escaping to the worker.
    res = support::json_response(http::status::internal_server_error, rig_json::encode(
        rig_error_body(std::type_identity<ErrorResponse>{}, 500, error)));
  }
  return true;
}

} // namespace holder::api
