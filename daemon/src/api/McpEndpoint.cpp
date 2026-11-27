#include "api/McpEndpoint.h"

#include "McpToolsJson.h"
#include "api/generated/McpBindings.h"
#include "api/support/HttpResponses.h"
#include "capi/Context.h"
#include "index/FtsIndexer.h"
#include "platform/Paths.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <optional>
#include <string_view>

namespace holder::api::mcp {
namespace {

namespace http = boost::beast::http;
using nlohmann::json;
constexpr std::string_view version = "2026-07-28";
constexpr auto max_request_bytes = 64 * 1024;
constexpr auto max_result_bytes = 256 * 1024;

struct ProjectDenied {};

GeneratedResponse error(http::status status, const json& id, int code, std::string_view message) {
  return support::json_response(
      status,
      {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}}
  );
}

std::string_view header(const GeneratedRequest& request, std::string_view name) {
  const boost::beast::string_view key(name.data(), name.size());
  if (request.count(key) != 1) return {};
  const auto value = request[key];
  return {value.data(), value.size()};
}

bool valid_host(const GeneratedRequest& request, boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  const auto local = socket.local_endpoint(ec);
  if (ec || !local.address().is_loopback()) return false;
  const auto remote = socket.remote_endpoint(ec);
  if (ec || !remote.address().is_loopback()) return false;
  const auto address = local.address().to_string();
  const auto host = local.address().is_v6() ? "[" + address + "]" : address;
  const auto port = ":" + std::to_string(local.port());
  const auto authority = header(request, "Host");
  return authority == host + port || authority == "localhost" + port ||
         (local.port() == 80 && (authority == host || authority == "localhost"));
}

bool authorized(const GeneratedRequest& request, const Access& access) {
  auto bearer = header(request, "Authorization");
  if (!bearer.starts_with("Bearer ") || access.token.empty()) return false;
  bearer.remove_prefix(7);
  return bearer.size() == access.token.size() &&
         CRYPTO_memcmp(bearer.data(), access.token.data(), bearer.size()) == 0;
}

bool name_matches(std::string_view field, std::string_view name) {
  if (field.empty()) return false;
  if (!field.starts_with("=?base64?") || !field.ends_with("?=")) return field == name;
  field.remove_prefix(9);
  field.remove_suffix(2);
  if (field.empty() || field.size() % 4 != 0) return false;
  std::string decoded(field.size(), '\0');
  const auto bytes = EVP_DecodeBlock(
      reinterpret_cast<unsigned char*>(decoded.data()),
      reinterpret_cast<const unsigned char*>(field.data()),
      static_cast<int>(field.size())
  );
  if (bytes < 0) return false;
  const auto padding = field.ends_with("==") ? 2 : field.ends_with("=") ? 1 : 0;
  decoded.resize(static_cast<std::size_t>(bytes - padding));
  return decoded == name;
}

const json& tools() {
  static const auto definitions = json::parse(holder::generated::mcp_tools_json());
  return definitions.at("tools");
}

json text_content(std::string_view text) {
  return json::array({{{"type", "text"}, {"text", text}}});
}

json tool_error(std::string_view message) {
  return {{"isError", true}, {"content", text_content(message)}};
}

json call_tool(
    const json& params,
    const Access& access,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops
) {
  try {
    std::optional<holder::index::FtsIndexer> own_fts;
    holder_context core(db, fts != nullptr ? *fts : own_fts.emplace(db), git_ops);
    core.secret_store = secret_store;
    const auto paths = holder::core::Paths::resolve("holder");
    core.projects_root = holder::core::default_projects_root();
    core.registry_path = paths.project_registry_path();
    auto data = call_read_tool(
        core,
        params.at("name").get_ref<const std::string&>(),
        params.value("arguments", json::object()),
        access.project_id,
        [&access](std::string_view project_id) {
          if (project_id != access.project_id) throw ProjectDenied{};
        }
    );
    const auto text = data.dump();
    if (text.size() > max_result_bytes)
      return tool_error("Tool result exceeds the response size limit.");
    return {
        {"structuredContent", std::move(data)},
        {"content", text_content(text)},
        {"isError", false}
    };
  } catch (const ProjectDenied&) {
    return tool_error("Access to this project was not granted.");
  } catch (const std::invalid_argument& ex) {
    return tool_error(ex.what());
  } catch (const holder::contract::InvalidArgument&) {
    return tool_error("Holder rejected the tool arguments.");
  } catch (const holder::contract::NotFound&) {
    return tool_error("The requested Holder item was not found.");
  } catch (const std::exception&) {
    return tool_error("Holder could not complete the read.");
  }
}

} // namespace

GeneratedResponse handle(
    const GeneratedRequest& request,
    boost::asio::ip::tcp::socket& socket,
    const Access& access,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops
) {
  if (!valid_host(request, socket) || request.count(http::field::origin) != 0) {
    return error(http::status::forbidden, nullptr, -32600, "MCP requires a local client.");
  }
  if (!authorized(request, access)) {
    auto response =
        error(http::status::unauthorized, nullptr, -32600, "Missing or invalid MCP token.");
    response.set(http::field::www_authenticate, "Bearer");
    return response;
  }
  if (request.method() != http::verb::post) {
    auto response = error(http::status::method_not_allowed, nullptr, -32600, "Use POST for MCP.");
    response.set(http::field::allow, "POST");
    return response;
  }
  if (request.body().size() > max_request_bytes) {
    return error(
        http::status::payload_too_large,
        nullptr,
        -32600,
        "MCP request exceeds the size limit."
    );
  }
  const auto content_type = header(request, "Content-Type");
  if (!boost::beast::iequals(content_type.substr(0, content_type.find(';')), "application/json")) {
    return error(http::status::unsupported_media_type, nullptr, -32600, "Send application/json.");
  }
  json request_body;
  try {
    request_body = json::parse(request.body(), [](int depth, json::parse_event_t, json&) {
      if (depth > 32) throw std::invalid_argument("MCP request exceeds the nesting limit.");
      return true;
    });
  } catch (const json::parse_error&) {
    return error(http::status::bad_request, nullptr, -32700, "Invalid JSON.");
  } catch (const std::invalid_argument&) {
    return error(
        http::status::bad_request,
        nullptr,
        -32600,
        "MCP request exceeds the nesting limit."
    );
  } catch (const json::exception&) {
    return error(http::status::bad_request, nullptr, -32600, "Invalid JSON value.");
  }
  if (!request_body.is_object() || request_body.value("jsonrpc", json()) != "2.0" ||
      !request_body.contains("method") || !request_body.at("method").is_string()) {
    return error(http::status::bad_request, nullptr, -32600, "Invalid JSON-RPC request.");
  }
  const auto id = request_body.value("id", json());
  if (!id.is_string() && !id.is_number_integer()) {
    return error(
        http::status::bad_request,
        nullptr,
        -32600,
        "MCP requests require a string or integer ID."
    );
  }

  const auto requested_version = header(request, "MCP-Protocol-Version");
  const auto& method = request_body.at("method").get_ref<const std::string&>();
  const auto params = request_body.value("params", json::object());
  if (!params.is_object()) {
    return error(http::status::bad_request, id, -32602, "MCP params must be an object.");
  }
  const auto meta = params.value("_meta", json::object());
  const auto body_version = meta.find("io.modelcontextprotocol/protocolVersion");
  if (requested_version.empty() || header(request, "Mcp-Method") != method ||
      body_version == meta.end() || !body_version->is_string() ||
      std::string_view(body_version->get_ref<const std::string&>()) != requested_version) {
    return error(
        http::status::bad_request,
        id,
        -32020,
        "Required MCP headers must match the request body."
    );
  }

  if (requested_version != version) {
    return support::json_response(
        http::status::bad_request,
        {{"jsonrpc", "2.0"},
         {"id", id},
         {"error",
          {{"code", -32022},
           {"message", "Unsupported protocol version"},
           {"data", {{"supported", json::array({version})}, {"requested", requested_version}}}}}}
    );
  }
  if (!meta.value("io.modelcontextprotocol/clientCapabilities", json()).is_object()) {
    return error(http::status::bad_request, id, -32602, "MCP client capabilities are required.");
  }

  json result;
  if (method == "server/discover") {
    result = {
        {"supportedVersions", json::array({version})},
        {"capabilities", {{"tools", json::object()}}}
    };
  } else if (method == "tools/list") {
    result = {{"tools", tools()}};
  } else if (method == "tools/call") {
    const auto name = params.value("name", json());
    if (!name.is_string() ||
        !name_matches(header(request, "Mcp-Name"), name.get_ref<const std::string&>())) {
      return error(http::status::bad_request, id, -32020, "Mcp-Name must match the tool name.");
    }
    const auto& catalog = tools();
    const auto found = std::find_if(catalog.begin(), catalog.end(), [&name](const json& tool) {
      return tool.at("name") == name;
    });
    if (found == catalog.end())
      return error(http::status::bad_request, id, -32602, "Unknown MCP tool.");
    result = call_tool(params, access, db, fts, secret_store, git_ops);
  } else {
    return error(http::status::not_found, id, -32601, "Method not found.");
  }
  if (method == "server/discover" || method == "tools/list") {
    result["ttlMs"] = 0;
    result["cacheScope"] = "private";
  }
  result["resultType"] = "complete";
  result["_meta"]["io.modelcontextprotocol/serverInfo"] = {
      {"name", "holderd"},
      {"version", access.server_version}
  };
  auto response = support::json_response(
      http::status::ok,
      {{"jsonrpc", "2.0"}, {"id", id}, {"result", result}}
  );
  response.set(http::field::cache_control, "no-store");
  return response;
}

} // namespace holder::api::mcp
