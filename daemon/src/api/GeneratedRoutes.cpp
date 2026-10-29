#include "api/GeneratedRoutes.h"

#include "api/AssetContent.h"
#include "api/AssetImportRead.h"
#include "api/AssetImportWrite.h"
#include "api/Alive.h"
#include "api/Bye.h"
#include "api/EventCursor.h"
#include "api/GoogleDriveOAuth.h"
#include "api/Health.h"
#include "api/LocationRead.h"
#include "api/LocationWrite.h"
#include "api/Ping.h"
#include "api/StaticFiles.h"
#include "api/Streams.h"

#include "api/AiRuntime.h"
#include "api/generated/ContractRoutes.h"
#include "api/generated/AiRoutes.h"
#include "capi/Context.h"
#include "index/FtsIndexer.h"
#include "platform/Paths.h"
#include "api/support/HttpResponses.h"
#include "api/support/StreamServer.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <exception>
#include <limits>
#include <optional>
#include <regex>
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

// A failure's message, as UTF-8 a JSON body can carry: a message can quote a request's bytes, such
// as the parser's for a body that is not JSON.
std::string utf8_message(const char* what) {
  return nlohmann::json::parse(nlohmann::json(what).dump(-1, ' ', false, nlohmann::json::error_handler_t::replace))
      .get<std::string>();
}

} // namespace

ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int status,
                             const std::exception& error) {
  return {false, {error_code(status), utf8_message(error.what())}};
}

ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int,
                             const holder::privacy::PrivacyError& error) {
  return {false, {holder::privacy::privacy_error_code_name(error.code()), utf8_message(error.what())}};
}

ErrorResponse api_error_body(std::type_identity<ErrorResponse>, int,
                             const CodedError& error) {
  return {false, {error.code, utf8_message(error.what())}};
}

namespace {

// Daemon-only contract routes use the same URL and JSON checks as core-backed routes.
// Their implementations need daemon services or a live socket, so they are dispatched here.
const std::regex import_job_path("^/imports/([^/]+)$");
const std::regex asset_content_path("^/resources/([^/]+)/assets/([^/]+)/content$");

nlohmann::json import_job_data(const AssetImportJob& job) {
  const auto optional_string = [](const std::optional<std::string>& value) -> nlohmann::json {
    return value ? nlohmann::json(*value) : nlohmann::json(nullptr);
  };
  return {{"job_id", job.job_id}, {"status", job.status},
          {"resource_id", optional_string(job.resource_id)},
          {"asset_id", optional_string(job.asset_id)},
          {"duplicate_reused", job.duplicate_reused}, {"link_created", job.link_created},
          {"error", optional_string(job.error)}};
}

nlohmann::json daemon_json(const AliveResponse& value) { return {{"ok", value.ok}}; }
nlohmann::json daemon_json(const ByeResponse& value) { return {{"ok", value.ok}}; }
nlohmann::json daemon_json(const HealthResponse& value) {
  const auto& data = value.data;
  return {{"ok", value.ok}, {"data", {{"db_ok", data.db_ok}, {"uptime_ms", data.uptime_ms},
          {"api_version", data.api_version}, {"server_version", data.server_version}, {"pid", data.pid},
          {"privacy", {{"backend", data.privacy.backend},
                       {"project_mode_supported", data.privacy.project_mode_supported}}}}}};
}
nlohmann::json daemon_json(const StorageLocationListResponse& value) {
  nlohmann::json data = nlohmann::json::array();
  for (const auto& item : value.data) {
    data.push_back({{"location_id", item.location_id}, {"project_id", item.project_id},
                    {"name", item.name}, {"provider", item.provider},
                    {"configuration", item.configuration}, {"bound", item.bound},
                    {"binding_preview", item.binding_preview ? nlohmann::json(*item.binding_preview)
                                                              : nlohmann::json(nullptr)},
                    {"created_at", item.created_at}, {"updated_at", item.updated_at}});
  }
  return {{"ok", value.ok}, {"data", std::move(data)},
          {"preferred_location_id", value.preferred_location_id
                                        ? nlohmann::json(*value.preferred_location_id)
                                        : nlohmann::json(nullptr)}};
}
nlohmann::json daemon_json(const LocationActionResponse& value) {
  nlohmann::json data = {{"location_id", value.data.location_id}};
  if (value.data.bound) data["bound"] = *value.data.bound;
  if (value.data.binding_preview) {
    data["binding_preview"] = *value.data.binding_preview
                                  ? nlohmann::json(**value.data.binding_preview)
                                  : nlohmann::json(nullptr);
  }
  return {{"ok", value.ok}, {"data", std::move(data)}};
}
nlohmann::json daemon_json(const LocationTestResponse& value) {
  return {{"ok", value.ok}, {"data", {{"available", value.data.available}}}};
}
nlohmann::json daemon_json(const GoogleDriveAuthorizeResponse& value) {
  return {{"ok", value.ok}, {"data", {{"authorization_url", value.data.authorization_url}}}};
}
nlohmann::json daemon_json(const EventCheckpointResponse& value) {
  nlohmann::json revisions = nlohmann::json::object();
  for (const auto& [project, revision] : value.data.git_revisions)
    revisions[project] = revision ? nlohmann::json(*revision) : nlohmann::json(nullptr);
  return {{"ok", value.ok}, {"data", {{"cursor", value.data.cursor},
          {"git_revisions", std::move(revisions)}, {"history_urls", value.data.history_urls}}}};
}

std::string import_field(const nlohmann::json& body, const char* name);

LocationBindingRequest decode_location_binding(const nlohmann::json& body) {
  if (!body.is_object()) throw std::invalid_argument("the request body must be an object");
  for (const auto& [name, value] : body.items())
    if (name != "values" && name != "preview") throw std::invalid_argument("there is no field " + name);
  if (!body.contains("values") || !body["values"].is_object())
    throw std::invalid_argument("values must be an object");
  LocationBindingRequest request;
  for (const auto& [name, value] : body["values"].items()) {
    if (!value.is_string()) throw std::invalid_argument("binding values must be strings");
    request.values[name] = value.get<std::string>();
  }
  if (body.contains("preview")) request.preview = import_field(body, "preview");
  return request;
}

PutLocationsPreferredRequest decode_preferred_location(const nlohmann::json& body) {
  if (!body.is_object()) throw std::invalid_argument("the request body must be an object");
  for (const auto& [name, value] : body.items())
    if (name != "project_id" && name != "location_id")
      throw std::invalid_argument("there is no field " + name);
  return {import_field(body, "project_id"), import_field(body, "location_id")};
}

void resource_error(GeneratedResponse& res, int status, const std::exception& error) {
  const auto* coded = dynamic_cast<const CodedError*>(&error);
  const std::string code = coded ? coded->code :
      dynamic_cast<const std::invalid_argument*>(&error) ? "invalid_argument" :
      status == 500 ? "internal_error" : error_code(status);
  res = support::json_response(static_cast<http::status>(status),
                               {{"ok", false}, {"error", {{"code", code},
                                                            {"message", utf8_message(error.what())}}}});
}

std::string import_field(const nlohmann::json& body, const char* name) {
  if (!body.contains(name) || !body[name].is_string())
    throw std::invalid_argument(std::string(name) + " must be a string");
  return contract_json::check_text(body[name].get<std::string>(), name, 0,
                                   std::numeric_limits<std::size_t>::max());
}

bool dispatch_resource_contract_route(const std::string& method, const std::string& path,
                                      const GeneratedRequest& req, GeneratedResponse& res,
                                      Context& context, support::StreamServer* stream) {
  const auto query_start = path.find('?');
  const auto route_path = path.substr(0, query_start);
  const auto query = query_start == std::string::npos ? std::string_view{} :
      std::string_view(path).substr(query_start + 1);
  std::smatch match;
  if (method == "POST" && route_path == "/imports") {
    try {
      contract_http::check_query_names(contract_http::decode_query(query), {});
      const auto body = contract_http::decode_body(req.body());
      if (!body.is_object()) throw std::invalid_argument("the request body must be an object");
      for (const auto& [name, value] : body.items()) {
        if (name != "project_id" && name != "card_id" && name != "location_id" && name != "source_path")
          throw std::invalid_argument("there is no field " + name);
      }
      AssetImportRequest request{import_field(body, "project_id"), import_field(body, "card_id"),
                                 import_field(body, "location_id"), import_field(body, "source_path")};
      const auto result = post_imports(context.db, context.secret_store, context.git_ops,
                                       context.uuid_v4, std::move(request));
      res = support::json_response(http::status::accepted,
                                   {{"ok", true}, {"data", import_job_data(result.data)}});
    } catch (const AssetImportLocationNotFound& error) {
      resource_error(res, 404, error);
    } catch (const support::StorageBindingRequired& error) {
      resource_error(res, 409, error);
    } catch (const AssetImportUnavailable& error) {
      resource_error(res, 501, error);
    } catch (const SecretStoreUnavailable& error) {
      resource_error(res, 503, error);
    } catch (const std::invalid_argument& error) {
      resource_error(res, 400, error);
    } catch (const std::exception& error) {
      resource_error(res, 500, error);
    }
    return true;
  }
  if (method == "GET" && std::regex_match(route_path, match, import_job_path)) {
    try {
      contract_http::check_query_names(contract_http::decode_query(query), {});
      const auto job_id = contract_http::decode_uri_component(match[1].str(), false);
      const auto result = get_import_job(contract_json::check_text(job_id, "job_id", 0,
          std::numeric_limits<std::size_t>::max()));
      res = support::json_response(http::status::ok,
                                   {{"ok", true}, {"data", import_job_data(result.data)}});
    } catch (const AssetImportJobNotFound& error) {
      resource_error(res, 404, error);
    } catch (const std::invalid_argument& error) {
      resource_error(res, 400, error);
    } catch (const std::exception& error) {
      resource_error(res, 500, error);
    }
    return true;
  }
  if (method == "GET" && std::regex_match(route_path, match, asset_content_path)) {
    try {
      const auto values = contract_http::decode_query(query);
      contract_http::check_query_names(values, {"placement_id"});
      const auto resource_id = contract_http::decode_uri_component(match[1].str(), false);
      const auto asset_id = contract_http::decode_uri_component(match[2].str(), false);
      contract_json::check_text(resource_id, "resource_id", 0, std::numeric_limits<std::size_t>::max());
      contract_json::check_text(asset_id, "asset_id", 0, std::numeric_limits<std::size_t>::max());
      std::optional<std::string> placement_id;
      if (const auto found = values.find("placement_id"); found != values.end())
        placement_id = contract_json::check_text(found->second, "placement_id", 0,
                                                  std::numeric_limits<std::size_t>::max());
      auto outcome = get_resources_resource_id_assets_asset_id_content(
          context.db, context.secret_store, context.git_ops, context.uuid_v4,
          resource_id, asset_id, placement_id);
      if (stream == nullptr) throw std::logic_error("no one to serve a host-written response");
      *stream = std::move(outcome.second);
    } catch (const support::StorageConflict& error) {
      resource_error(res, 409, error);
    } catch (const support::StorageBindingRequired& error) {
      resource_error(res, 409, error);
    } catch (const support::StorageIntegrityFailed& error) {
      resource_error(res, 422, error);
    } catch (const support::StorageGatewayFailure& error) {
      resource_error(res, 502, error);
    } catch (const support::StorageUnavailable& error) {
      resource_error(res, 503, error);
    } catch (const support::StorageCapacityExceeded& error) {
      resource_error(res, 507, error);
    } catch (const AssetContentNotFound& error) {
      resource_error(res, 404, error);
    } catch (const support::StorageBadRequest& error) {
      resource_error(res, 400, error);
    } catch (const std::exception& error) {
      resource_error(res, 400, error);
    }
    return true;
  }
  return false;
}

bool dispatch_misc_contract_route(const std::string& method, const std::string& path,
                                  const GeneratedRequest& req, GeneratedResponse& res,
                                  Context& context, support::StreamServer* stream) {
  const auto split = path.find('?');
  const auto route_path = path.substr(0, split);
  const auto query_text = split == std::string::npos ? std::string_view{} :
      std::string_view(path).substr(split + 1);
  static const std::regex location_binding("^/locations/([^/]+)/binding$");
  static const std::regex location_test("^/locations/([^/]+)/test$");
  static const std::regex google_authorize("^/locations/([^/]+)/oauth/google-drive/authorize$");
  static const std::regex google_callback("^/locations/([^/]+)/oauth/google-drive/callback$");
  static const std::regex docs_file("^/docs/([^/]+)$");
  std::smatch match;
  bool matched = false;
  bool authorize = false;
  const auto no_query = [&] { contract_http::check_query_names(contract_http::decode_query(query_text), {}); };
  const auto path_text = [&](std::size_t index) { return contract_http::decode_uri_component(match[index].str(), false); };
  const auto respond = [&](const auto& value) {
    res = support::json_response(http::status::ok, daemon_json(value));
  };
  const auto raw = [&](std::string content, const char* media_type) {
    res = {http::status::ok, 11};
    res.set(http::field::content_type, media_type);
    res.keep_alive(false);
    res.body() = std::move(content);
    res.prepare_payload();
  };
  const auto hosted = [&](api_runtime::host_value outcome) {
    if (stream == nullptr) throw std::logic_error("no one to serve a host-written response");
    *stream = std::any_cast<support::StreamServer>(std::move(outcome.value));
  };
  try {
    if (method == "GET" && route_path == "/alive") {
      matched = true;
      no_query();
      respond(check_alive());
    } else if (method == "GET" && route_path == "/ping") {
      matched = true;
      no_query();
      raw(get_ping(), "text/plain");
    } else if (method == "GET" && route_path == "/health") {
      matched = true;
      no_query();
      respond(get_health(context.db, context.started_at));
    } else if (method == "POST" && route_path == "/bye") {
      matched = true;
      no_query();
      respond(say_goodbye());
    } else if (method == "GET" && route_path == "/locations") {
      matched = true;
      const auto values = contract_http::decode_query(query_text);
      contract_http::check_query_names(values, {"project_id"});
      const auto found = values.find("project_id");
      if (found == values.end()) throw std::invalid_argument("missing query parameter: project_id");
      contract_json::check_text(found->second, "project_id", 1, std::numeric_limits<std::size_t>::max());
      respond(get_locations(context.db, context.secret_store, found->second));
    } else if (method == "PUT" && route_path == "/locations/preferred") {
      matched = true;
      no_query();
      const auto body = contract_http::decode_body(req.body());
      respond(put_locations_preferred(context.db, context.secret_store,
          decode_preferred_location(body)));
    } else if (method == "PUT" && std::regex_match(route_path, match, location_binding)) {
      matched = true;
      no_query();
      const auto body = contract_http::decode_body(req.body());
      respond(put_locations_location_id_binding(context.db, context.secret_store, path_text(1),
          decode_location_binding(body)));
    } else if (method == "DELETE" && std::regex_match(route_path, match, location_binding)) {
      matched = true;
      no_query();
      respond(delete_locations_location_id_binding(context.db, context.secret_store, path_text(1)));
    } else if (method == "POST" && std::regex_match(route_path, match, location_test)) {
      matched = true;
      no_query();
      respond(post_locations_location_id_test(context.db, context.secret_store,
                                               context.uuid_v4, path_text(1)));
    } else if (method == "POST" && std::regex_match(route_path, match, google_authorize)) {
      matched = true;
      authorize = true;
      no_query();
      respond(post_locations_location_id_oauth_google_drive_authorize(context.db, context.host,
                                                                       path_text(1)));
    } else if (method == "GET" && std::regex_match(route_path, match, google_callback)) {
      matched = true;
      const auto values = contract_http::decode_query(query_text);
      contract_http::check_query_names(values, {"code", "state", "error"});
      const auto optional = [&](const char* name) -> std::optional<std::string> {
        const auto found = values.find(name);
        return found == values.end() ? std::nullopt : std::optional<std::string>(found->second);
      };
      hosted(get_locations_location_id_oauth_google_drive_callback(
          context.db, context.secret_store, context.git_ops, context.host, path_text(1),
          optional("code"), optional("state"), optional("error")));
    } else if (method == "GET" && route_path == "/events/cursor") {
      matched = true;
      const auto values = contract_http::decode_query(query_text);
      contract_http::check_query_names(values, {"project_id"});
      std::optional<std::string> project_id;
      if (const auto found = values.find("project_id"); found != values.end()) {
        contract_json::check_text(found->second, "project_id", 1, std::numeric_limits<std::size_t>::max());
        project_id = found->second;
      }
      respond(get_events_cursor(context.db, context.events, project_id));
    } else if (method == "GET" && route_path == "/events") {
      matched = true;
      const auto values = contract_http::decode_query(query_text);
      contract_http::check_query_names(values, {"project_id", "last_revision"});
      std::optional<std::string> project_id;
      std::optional<std::string> last_revision;
      if (const auto found = values.find("project_id"); found != values.end()) {
        project_id = found->second;
      }
      if (const auto found = values.find("last_revision"); found != values.end()) {
        static const std::regex revision("^[0-9a-f]{40}$");
        if (!std::regex_match(found->second, revision)) throw std::invalid_argument("invalid last_revision");
        last_revision = found->second;
      }
      const auto header = req.find("Last-Event-ID");
      const std::optional<std::string> last_event_id = header == req.end() ? std::nullopt :
          std::optional<std::string>(std::string(header->value()));
      hosted(get_events(context.db, context.events, project_id, last_revision, last_event_id));
    } else if (method == "GET" && route_path == "/openapi.yaml") {
      matched = true;
      no_query();
      raw(download_openapi_yaml(), "application/yaml");
    } else if (method == "GET" && route_path == "/openapi.json") {
      matched = true;
      no_query();
      res = support::json_response(http::status::ok, download_openapi_json());
    } else if (method == "GET" && route_path == "/ai_catalog.yaml") {
      matched = true;
      no_query();
      raw(download_ai_catalog_yaml(), "application/yaml");
    } else if (method == "GET" && route_path == "/ai_catalog.json") {
      matched = true;
      no_query();
      auto catalog = download_ai_catalog_json();
      if (!catalog.is_object()) throw std::runtime_error("ai_catalog.yaml must contain an object");
      res = support::json_response(http::status::ok, catalog);
    } else if (method == "GET" && route_path == "/git_providers.yaml") {
      matched = true;
      no_query();
      raw(download_git_providers_yaml(), "application/yaml");
    } else if (method == "GET" && route_path == "/git_providers.json") {
      matched = true;
      no_query();
      res = support::json_response(http::status::ok, download_git_providers_json());
    } else if (method == "GET" && route_path == "/docs") {
      matched = true;
      no_query();
      hosted(get_docs());
    } else if (method == "GET" && std::regex_match(route_path, match, docs_file)) {
      matched = true;
      no_query();
      hosted(get_docs_file(path_text(1)));
    }
  } catch (const std::exception& error) {
    int status = 500;
    if (dynamic_cast<const LocationNotFound*>(&error) ||
        dynamic_cast<const EventCursorProjectNotFound*>(&error) ||
        dynamic_cast<const StaticFileNotFound*>(&error)) status = 404;
    else if (dynamic_cast<const support::StorageConflict*>(&error) ||
             dynamic_cast<const support::StorageBindingRequired*>(&error)) status = 409;
    else if (dynamic_cast<const support::StorageIntegrityFailed*>(&error)) status = 422;
    else if (dynamic_cast<const support::StorageGatewayFailure*>(&error)) status = 502;
    else if (dynamic_cast<const support::StorageBadRequest*>(&error)) status = 400;
    else if (dynamic_cast<const support::StorageCapacityExceeded*>(&error)) status = 507;
    else if (dynamic_cast<const support::StorageUnavailable*>(&error) ||
             dynamic_cast<const SecretStoreUnavailable*>(&error) ||
             dynamic_cast<const ChangeFeedUnavailable*>(&error) ||
             dynamic_cast<const ProjectUnavailable*>(&error)) status = 503;
    else if (authorize || dynamic_cast<const std::invalid_argument*>(&error) ||
             dynamic_cast<const std::out_of_range*>(&error)) status = 400;
    resource_error(res, status, error);
  }
  return matched;
}

// The methods both dispatchers serve on a path, for a 405's Allow header.
std::string allowed_methods(const std::string& path, const std::string& rig_allow) {
  std::set<std::string> methods;
  for (const auto method : contract_http::allowed_methods(path)) methods.emplace(method);
  const auto route_path = path.substr(0, path.find('?'));
  if (route_path == "/imports") methods.emplace("POST");
  std::smatch match;
  if (std::regex_match(route_path, match, import_job_path) ||
      std::regex_match(route_path, match, asset_content_path)) methods.emplace("GET");
  for (std::size_t start = 0; methods.empty() && start < rig_allow.size();) {
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
  const std::string method(req.method_string());
  const std::string path(target.data(), target.size());
  return ai_http::is_public(method, path) || contract_http::is_public(method, path);
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
    if (dispatch_resource_contract_route(method, path, req, res, context, stream) ||
        dispatch_misc_contract_route(method, path, req, res, context, stream)) return true;
    // RIG's routes first, so that a literal segment in one of RIG's keeps its place before a
    // parameter in one of the contract's; the contract's tooling checks that no RIG route hides
    // one of the contract's. Then the contract's, for a request RIG has no route for or
    // would answer with a 405.
    auto generated = ai_http::dispatch(
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
      core.secret_store = secret_store;
      const auto paths = holder::core::Paths::resolve("holder");
      core.projects_root = holder::core::default_projects_root();
      core.registry_path = paths.project_registry_path();
      if (const auto answered = contract_http::dispatch(method, path, req.body(), core)) {
        res = support::json_response(static_cast<http::status>(answered->status), answered->body);
        return true;
      }
    }
    if (!generated) {
      // A path only the contract serves, with another method.
      const auto allow = allowed_methods(path, "");
      if (allow.empty()) return false;
      res = support::json_response(http::status::method_not_allowed, ai_json::encode(api_error_body(
          std::type_identity<ErrorResponse>{}, 405, ai_http::method_not_allowed("method not allowed"))));
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
    res = support::json_response(http::status::internal_server_error, ai_json::encode(
        api_error_body(std::type_identity<ErrorResponse>{}, 500, error)));
  }
  return true;
}

} // namespace holder::api
