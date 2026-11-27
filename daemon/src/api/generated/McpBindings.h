// Generated from contract/interface (cards, links, milestones, resources, search) by compiler.
// Do not edit; change the contract, then run make -C contract update.
//
// Live MCP read tool bindings.
#pragma once
#include "api/generated/ContractJson.h"
#include <functional>
#include <string_view>
#include <utility>
#include <vector>
namespace contract_json {
template <> struct codec<holder::contract::ListCardsOrder> {
  static json encode(holder::contract::ListCardsOrder value) {
    switch (value) {
    case holder::contract::ListCardsOrder::CardIdAsc:
      return "card_id_asc";
    case holder::contract::ListCardsOrder::UpdatedDesc:
      return "updated_desc";
    case holder::contract::ListCardsOrder::SortKeyAsc:
      return "sort_key_asc";
    }
    throw std::invalid_argument("is not a known value");
  }
  static holder::contract::ListCardsOrder decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("must be a string");
    const auto& text = value.get_ref<const std::string&>();
    if (text == "card_id_asc") return holder::contract::ListCardsOrder::CardIdAsc;
    if (text == "updated_desc") return holder::contract::ListCardsOrder::UpdatedDesc;
    if (text == "sort_key_asc") return holder::contract::ListCardsOrder::SortKeyAsc;
    throw std::invalid_argument("must be one of card_id_asc, updated_desc, sort_key_asc");
  }
};

} // namespace contract_json
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
  if (name == "get_card_breadcrumbs") {
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
         encode(holder::contract::get_card_breadcrumbs(
             context,
             std::move(project_id),
             std::move(card_id)
         ))}
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
  if (name == "list_card_backlinks") {
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
    auto links = holder::contract::list_card_backlinks(
        context,
        project_id,
        std::move(card_id),
        holder::contract::ListCardBacklinksScope::Local,
        false
    );
    std::erase_if(links, [&project_id](const auto& link) {
      return link.source_project_id != project_id || link.target_project_id != project_id;
    });
    return json{{"data", encode(links)}};
  }
  if (name == "list_card_links") {
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
    auto links = holder::contract::list_card_links(context, project_id, std::move(card_id), false);
    std::erase_if(links, [&project_id](const auto& link) {
      return link.source_project_id != project_id || link.target_project_id != project_id;
    });
    return json{{"data", encode(links)}};
  }
  if (name == "list_card_milestones") {
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
         encode(holder::contract::list_card_milestones(
             context,
             std::move(project_id),
             std::move(card_id)
         ))}
    };
  }
  if (name == "list_cards") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "parent" && it.key() != "tag" &&
          it.key() != "order" && it.key() != "limit" && it.key() != "cursor" &&
          it.key() != "include_child_count")
        throw std::invalid_argument("unknown tool argument");
    }
    if (!arguments.contains("project_id")) throw std::invalid_argument("project_id is required");
    auto project_id = check_text(
        decode_field<std::string>(arguments.at("project_id"), "project_id"),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    std::optional<holder::contract::ListCardsParent> parent;
    if (const auto value = arguments.find("parent");
        value != arguments.end() && !value->is_null()) {
      auto text = decode_field<std::string>(*value, "parent");
      if (text == "roots")
        parent = holder::contract::ListCardsParent{holder::contract::ListCardsParentRoots{}};
      else
        parent = holder::contract::ListCardsParent{check_text(
            decode_field<std::string>(*value, "parent"),
            "parent",
            1,
            std::numeric_limits<std::size_t>::max()
        )};
    }
    std::optional<std::string> tag;
    if (const auto value = arguments.find("tag"); value != arguments.end() && !value->is_null())
      tag = check_text(
          decode_field<std::string>(*value, "tag"),
          "tag",
          1,
          std::numeric_limits<std::size_t>::max()
      );
    auto order = decode_field<holder::contract::ListCardsOrder>(
        arguments.value("order", json("card_id_asc")),
        "order"
    );
    auto limit = check_range<std::int32_t>(
        decode_field<std::int32_t>(arguments.value("limit", json(50)), "limit"),
        "limit",
        1,
        100
    );
    std::optional<std::string> cursor;
    if (const auto value = arguments.find("cursor"); value != arguments.end() && !value->is_null())
      cursor = check_text(decode_field<std::string>(*value, "cursor"), "cursor", 1, 8192);
    auto include_child_count = decode_field<bool>(
        arguments.value("include_child_count", json(false)),
        "include_child_count"
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::list_cards(
             context,
             std::move(project_id),
             std::move(parent),
             std::move(tag),
             order,
             limit,
             std::move(cursor),
             false,
             include_child_count
         ))}
    };
  }
  if (name == "list_project_tags") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id") throw std::invalid_argument("unknown tool argument");
    }
    if (!arguments.contains("project_id")) throw std::invalid_argument("project_id is required");
    auto project_id = check_text(
        decode_field<std::string>(arguments.at("project_id"), "project_id"),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data", encode(holder::contract::list_project_tags(context, std::move(project_id)))}
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
