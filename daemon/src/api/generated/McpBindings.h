// Generated from contract/interface (cards, resources, search) by compiler.
// Do not edit; change the contract, then run make -C contract update.
//
// Live MCP read tool bindings.
#pragma once
#include "api/generated/ContractJson.h"
#include <functional>
#include <string_view>
#include <utility>
namespace holder::api::mcp {
// Authorization must throw on denial. Domain errors propagate to the protocol adapter.
inline nlohmann::json call_read_tool(
    holder_context& context,
    std::string_view name,
    const nlohmann::json& arguments,
    const std::function<void(std::string_view)>& authorize_project
) {
  using namespace contract_json;
  if (!arguments.is_object()) throw std::invalid_argument("tool arguments must be an object");
  if (name == "get_card") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id")
        throw std::invalid_argument("unknown tool argument");
    }
    if (!arguments.contains("project_id")) throw std::invalid_argument("project_id is required");
    auto project_id = check_text(
        decode_field<std::string>(arguments.at("project_id"), "project_id"),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("card_id")) throw std::invalid_argument("card_id is required");
    auto card_id = check_text(
        decode_field<std::string>(arguments.at("card_id"), "card_id"),
        "card_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::get_card(context, std::move(project_id), std::move(card_id)))}
    };
  }
  if (name == "get_resource") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "resource_id")
        throw std::invalid_argument("unknown tool argument");
    }
    if (!arguments.contains("project_id")) throw std::invalid_argument("project_id is required");
    auto project_id = check_text(
        decode_field<std::string>(arguments.at("project_id"), "project_id"),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("resource_id")) throw std::invalid_argument("resource_id is required");
    auto resource_id = check_text(
        decode_field<std::string>(arguments.at("resource_id"), "resource_id"),
        "resource_id",
        4,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(
             holder::contract::get_resource(context, std::move(project_id), std::move(resource_id))
         )}
    };
  }
  if (name == "search_cards") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "q" && it.key() != "limit" &&
          it.key() != "offset")
        throw std::invalid_argument("unknown tool argument");
    }
    if (!arguments.contains("project_id")) throw std::invalid_argument("project_id is required");
    auto project_id = check_text(
        decode_field<std::string>(arguments.at("project_id"), "project_id"),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("q")) throw std::invalid_argument("q is required");
    auto q = check_text(
        decode_field<std::string>(arguments.at("q"), "q"),
        "q",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    auto limit = check_range<std::int32_t>(
        decode_field<std::int32_t>(arguments.value("limit", json(20)), "limit"),
        "limit",
        1,
        100
    );
    auto offset = check_range<std::int32_t>(
        decode_field<std::int32_t>(arguments.value("offset", json(0)), "offset"),
        "offset",
        0,
        std::numeric_limits<std::int32_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::
                    search_cards(context, std::move(project_id), std::move(q), limit, offset))}
    };
  }
  throw std::invalid_argument("unknown MCP tool");
}
} // namespace holder::api::mcp
