#include "api/GeneratedRoutes.h"

#include "api/RigRuntime.h"
#include "api/generated/ContractRoutes.h"
#include "api/generated/RuntimeRoutes.h"
#include "capi/Context.h"
#include "index/FtsIndexer.h"
#include "api/support/HttpResponses.h"
#include "api/support/StreamServer.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>

#include <chrono>
#include <exception>
#include <optional>
#include <set>
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
  holder::api::support::EventService* events;
  // The request's Host header: where the caller reached this daemon.
  std::optional<std::string> host;
  // The request's connection, for a route that streams as it runs.
  support::RequestSocket* socket;
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

namespace {

// The methods both dispatchers serve on a path, for a 405's Allow header.
std::string allowed_methods(const std::string& path, const std::string& rig_allow) {
  std::set<std::string> methods;
  for (const auto method : contract_http::allowed_methods(path)) methods.emplace(method);
  for (std::size_t start = 0; start < rig_allow.size();) {
    const auto end = rig_allow.find(", ", start);
    methods.insert(rig_allow.substr(start, end - start));
    if (end == std::string::npos) break;
    start = end + 2;
  }
  std::string allow;
  for (const auto& method : methods) allow += (allow.empty() ? "" : ", ") + method;
  return allow;
}

} // namespace

bool is_public_generated_route(const GeneratedRequest& req) {
  const auto target = req.target();
  return rig_http::is_public(std::string(req.method_string()),
                             std::string(target.data(), target.size()));
}

bool dispatch_generated_routes(
    const GeneratedRequest& req,
    GeneratedResponse& res,
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* registry,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    std::function<std::string()> uuid_v4,
    std::chrono::steady_clock::time_point started_at,
    holder::card::CardStore* card_store,
    holder::api::support::EventService* events,
    support::StreamServer* stream,
    boost::asio::ip::tcp::socket* socket
) {
  const auto target = req.target();
  std::optional<support::RequestSocket> request_socket;
  if (socket != nullptr) request_socket.emplace(support::RequestSocket{*socket});
  Context context{
      db,
      registry,
      fts,
      secret_store,
      git_ops,
      std::move(uuid_v4),
      started_at,
      card_store,
      events,
      req.count(http::field::host) > 0
          ? std::optional<std::string>(std::string(req[http::field::host]))
          : std::nullopt,
      request_socket ? &*request_socket : nullptr
  };
  const std::string method(req.method_string());
  const std::string path(target.data(), target.size());
  try {
    // RIG's routes first, so that a literal route such as GET /cards/context keeps its place
    // before the contract's GET /cards/{card_id}; the contract's tooling checks that no RIG route
    // hides one of the contract's. Then the contract's, for a request RIG has no route for or
    // would answer with a 405.
    auto generated = rig_http::dispatch(
        method, path, req.body(), context, [&req](std::string_view name) -> std::optional<std::string> {
          const auto field = req.find(boost::beast::string_view(name.data(), name.size()));
          if (field == req.end()) return std::nullopt;
          return std::string(field->value());
        });
    if (!generated || generated->status == 405) {
      // The contract's operations run in core with this worker's services. An indexer only
      // refers to its database, so a caller without one gets one on the same database.
      std::optional<holder::index::FtsIndexer> own_fts;
      holder_context core(db, fts != nullptr ? *fts : own_fts.emplace(db), git_ops);
      if (const auto answered = contract_http::dispatch(method, path, req.body(), core)) {
        res = support::json_response(static_cast<http::status>(answered->status), answered->body);
        return true;
      }
    }
    if (!generated) {
      // A path only the contract serves, with another method.
      const auto allow = allowed_methods(path, "");
      if (allow.empty()) return false;
      res = support::json_response(http::status::method_not_allowed, rig_json::encode(rig_error_body(
          std::type_identity<ErrorResponse>{}, 405, rig_http::method_not_allowed("method not allowed"))));
      res.set(http::field::allow, allow);
      return true;
    }
    if (generated->host.has_value()) {
      // A host-value route: the session serves the body with the route's StreamServer.
      auto server = std::any_cast<support::StreamServer>(std::move(generated->host));
      if (stream == nullptr) throw std::logic_error("no one to serve a host-written response");
      *stream = std::move(server);
      return true;
    }
    if (generated->status == 204) {
      res = support::stream_finished_response();
      return true;
    }
    if (generated->media_type != "application/json") {
      // A text route: its body is the string to send.
      res = {static_cast<http::status>(generated->status), 11};
      res.set(http::field::content_type, std::string(generated->media_type));
      res.keep_alive(false);
      res.body() = generated->body.get<std::string>();
      res.prepare_payload();
      return true;
    }
    res = support::json_response(static_cast<http::status>(generated->status), generated->body);
    if (!generated->allow.empty()) res.set(http::field::allow, allowed_methods(path, generated->allow));
  } catch (const std::exception& error) {
    // Operations' own failures are already 500s from the generated dispatcher.
    // This only keeps a failure outside them from escaping to the worker.
    res = support::json_response(http::status::internal_server_error, rig_json::encode(
        rig_error_body(std::type_identity<ErrorResponse>{}, 500, error)));
  }
  return true;
}

} // namespace holder::api
