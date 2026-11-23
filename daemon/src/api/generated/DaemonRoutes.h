// Generated from contract/interface (ai, cards, daemon, documents, errors.py, events, git, history,
// links, locations, maintenance, milestones, pagination, projects, recovery, resources, runtime,
// search, trash) by compiler. Do not edit; change the contract, then run make -C contract update.
//
// Complete daemon request/response boundaries.
#pragma once
#include "api/LocationWrite.h"
#include "api/generated/ContractRoutes.h"
#include "api/generated/DaemonJson.h"
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace daemon_http {
struct response {
  int status;
  nlohmann::json body;
};

inline response failure(int status, const std::exception& error) {
  const auto type = std::type_identity<holder::api::ErrorResponse>{};
  if (const auto* coded = dynamic_cast<const holder::api::CodedError*>(&error))
    return {status, daemon_json::encode(holder::api::api_error_body(type, status, *coded))};
  auto result = holder::api::api_error_body(type, status, error);
  if (status == 400) result.error.code = "invalid_argument";
  if (status == 500) result.error.code = "internal_error";
  return {status, daemon_json::encode(result)};
}

template <typename Context>
std::optional<response> dispatch(
    // Match the method/path/body signature shared by the generated dispatchers.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const std::string& method,
    const std::string& path,
    const std::string& body,
    Context& context
) {
  const auto split = path.find('?');
  const auto route_path = path.substr(0, split);
  const auto query = split == std::string::npos ? std::string_view{}
                                                : std::string_view(path).substr(split + 1);
  std::smatch match;
  static const std::regex route_0("^/locations/([^/]+)/binding$");
  if (method == "PUT" && std::regex_match(route_path, match, route_0)) {
    try {
      contract_http::check_query_names(contract_http::decode_query(query), {});
      auto arg_location_id = daemon_json::check_text(
          contract_http::decode_uri_component(match[1].str(), false),
          "location_id",
          0,
          std::numeric_limits<std::size_t>::max()
      );
      auto arg_request = daemon_json::decode<holder::api::LocationBindingRequest>(
          contract_http::decode_body(body)
      );
      const auto result = holder::api::put_locations_location_id_binding(
          context.db,
          context.secret_store,
          std::move(arg_location_id),
          std::move(arg_request)
      );
      return response{200, daemon_json::encode(result)};
    } catch (const holder::api::LocationNotFound& error) {
      return failure(404, error);
    } catch (const holder::api::SecretStoreUnavailable& error) {
      return failure(503, error);
    } catch (const std::invalid_argument& error) {
      return failure(400, error);
    } catch (const std::out_of_range& error) {
      return failure(400, error);
    } catch (const std::exception& error) {
      return failure(500, error);
    }
  }
  return std::nullopt;
}
} // namespace daemon_http
