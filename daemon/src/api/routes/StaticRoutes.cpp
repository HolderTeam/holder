#include "api/routes/StaticRoutes.h"

#include "OpenApiJson.h"
#include "OpenApiYaml.h"
#include "api/support/HttpResponses.h"
#include "api/support/PathDiscovery.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

http::response<http::string_body> text_response(
    http::status status,
    std::string body,
    const std::string& content_type
) {
  http::response<http::string_body> res{status, 11};
  res.set(http::field::content_type, content_type);
  res.keep_alive(false);
  res.body() = std::move(body);
  res.prepare_payload();
  return res;
} // LCOV_EXCL_LINE

nlohmann::json yaml_to_json(const YAML::Node& node) {
  if (!node || node.IsNull()) {
    return nullptr;
  }
  if (node.IsScalar()) {
    std::string value = node.Scalar();
    if (value == "true") return true;
    if (value == "false") return false;
    char* end = nullptr;
    const auto integer = std::strtoll(value.c_str(), &end, 10);
    if (end != value.c_str() && *end == '\0') return integer;
    end = nullptr;
    const auto floating = std::strtod(value.c_str(), &end);
    if (end != value.c_str() && *end == '\0') return floating;
    return value;
  }
  if (node.IsSequence()) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& item : node) {
      out.push_back(yaml_to_json(item));
    }
    return out;
  }
  if (node.IsMap()) {
    nlohmann::json out = nlohmann::json::object();
    for (const auto& pair : node) {
      out[pair.first.as<std::string>()] = yaml_to_json(pair.second);
    }
    return out;
  }
  return nullptr; // LCOV_EXCL_LINE
}

http::response<http::string_body> not_found(const std::string& message) {
  return support::error_response(http::status::not_found, "not_found", message);
}

// Serves a YAML config file as it is, or converted to JSON.
http::response<http::string_body> config_response(
    const std::optional<std::filesystem::path>& path,
    const std::string& name,
    bool as_json
) {
  const auto content = path.has_value() ? support::read_file(path.value()) : std::nullopt;
  if (!content.has_value()) return not_found(name + " not found.");
  if (!as_json) {
    return text_response(
        http::status::ok,
        content.value(),
        support::content_type_for_extension(path->extension().string())
    );
  }
  try {
    return text_response(
        http::status::ok, yaml_to_json(YAML::Load(content.value())).dump(2), "application/json"
    );
  } catch (const std::exception&) {
    return support::error_response(
        http::status::internal_server_error, "error", "Failed to parse " + name + "."
    );
  }
}

} // namespace

bool handle_static_routes(
    const std::string& path,
    const http::request<http::string_body>&,
    http::response<http::string_body>& res
) {
  if (path == "/openapi.json") {
    res = text_response(http::status::ok, std::string(holder::generated::openapi_json()),
                        "application/json");
    return true;
  }
  if (path == "/openapi.yaml") {
    res = text_response(http::status::ok, std::string(holder::generated::openapi_yaml()),
                        "application/yaml");
    return true;
  }
  if (path == "/ai_catalog.yaml" || path == "/ai_catalog.json") {
    res = config_response(
        support::find_ai_catalog_path(), "ai_catalog.yaml", path.ends_with(".json")
    );
    return true;
  }
  if (path == "/git_providers.yaml" || path == "/git_providers.json") {
    res = config_response(
        support::find_git_providers_path(), "git_providers.yaml", path.ends_with(".json")
    );
    return true;
  }
  if (path != "/docs" && !path.starts_with("/docs/")) return false;

  const auto docs_root = support::find_docs_root();
  if (!docs_root.has_value()) {
    res = not_found("Docs assets not found.");
    return true;
  }
  const std::filesystem::path rel_path(path == "/docs" ? "index.html" : path.substr(6));
  if (!support::is_safe_relpath(rel_path)) {
    res = not_found("Not found.");
    return true;
  }
  const auto full_path = docs_root.value() / rel_path;
  const auto content = support::read_file(full_path);
  if (!content.has_value()) {
    res = not_found("Not found.");
    return true;
  }
  res = text_response(
      http::status::ok,
      content.value(),
      support::content_type_for_extension(full_path.extension().string())
  );
  return true;
}

} // namespace holder::api::routes
