#include "api/StaticFiles.h"

#include "OpenApiJson.h"
#include "OpenApiYaml.h"
#include "api/support/PathDiscovery.h"
#include "api/support/StreamServer.h"

#include <boost/beast/http.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <filesystem>
#include <optional>

namespace holder::api {
namespace {

namespace http = boost::beast::http;

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

std::string read_config(const std::optional<std::filesystem::path>& path, const std::string& name) {
  auto content = path.has_value() ? support::read_file(path.value()) : std::nullopt;
  if (!content.has_value()) throw StaticFileNotFound(name + " not found.");
  return std::move(*content);
}

// Writes a documentation file with the media type its extension implies.
rig::host_value serve_docs_file(const std::filesystem::path& relative) {
  const auto docs_root = support::find_docs_root();
  if (!docs_root.has_value()) throw StaticFileNotFound("Docs assets not found.");
  if (!support::is_safe_relpath(relative)) throw StaticFileNotFound("Not found.");
  const auto full_path = docs_root.value() / relative;
  auto content = support::read_file(full_path);
  if (!content.has_value()) throw StaticFileNotFound("Not found.");
  return {
      200,
      support::StreamServer([content = std::move(*content),
                             media_type = support::content_type_for_extension(
                                 full_path.extension().string()
                             )](boost::asio::ip::tcp::socket&,
                                http::response<http::string_body>& res) {
        res = {http::status::ok, 11};
        res.set(http::field::content_type, media_type);
        res.keep_alive(false);
        res.body() = content;
        res.prepare_payload();
        return false;
      })
  };
}

} // namespace

std::string download_openapi_yaml() { return std::string(holder::generated::openapi_yaml()); }

nlohmann::json download_openapi_json() {
  static const auto contract = nlohmann::json::parse(holder::generated::openapi_json());
  return contract;
}

std::string download_ai_catalog_yaml() {
  return read_config(support::find_ai_catalog_path(), "ai_catalog.yaml");
}

std::string download_git_providers_yaml() {
  return read_config(support::find_git_providers_path(), "git_providers.yaml");
}

nlohmann::json download_ai_catalog_json() {
  return yaml_to_json(YAML::Load(download_ai_catalog_yaml()));
}

nlohmann::json download_git_providers_json() {
  return yaml_to_json(YAML::Load(download_git_providers_yaml()));
}

rig::host_value get_docs() { return serve_docs_file("index.html"); }

rig::host_value get_docs_file(std::string file) { return serve_docs_file(file); }

} // namespace holder::api
