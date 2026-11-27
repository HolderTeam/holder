// Generated from contract/interface (cards, links, milestones, resources, search, trash) by
// compiler. Do not edit; change the contract, then run make -C contract update.
//
// Live MCP tool bindings.
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

template <> struct codec<holder::contract::RemoveCardLinkTargetType> {
  static json encode(holder::contract::RemoveCardLinkTargetType value) {
    switch (value) {
    case holder::contract::RemoveCardLinkTargetType::Card:
      return "card";
    case holder::contract::RemoveCardLinkTargetType::AiMessage:
      return "ai_message";
    case holder::contract::RemoveCardLinkTargetType::AiThread:
      return "ai_thread";
    case holder::contract::RemoveCardLinkTargetType::Resource:
      return "resource";
    }
    throw std::invalid_argument("is not a known value");
  }
  static holder::contract::RemoveCardLinkTargetType decode(const json& value) {
    if (!value.is_string()) throw std::invalid_argument("must be a string");
    const auto& text = value.get_ref<const std::string&>();
    if (text == "card") return holder::contract::RemoveCardLinkTargetType::Card;
    if (text == "ai_message") return holder::contract::RemoveCardLinkTargetType::AiMessage;
    if (text == "ai_thread") return holder::contract::RemoveCardLinkTargetType::AiThread;
    if (text == "resource") return holder::contract::RemoveCardLinkTargetType::Resource;
    throw std::invalid_argument("must be one of card, ai_message, ai_thread, resource");
  }
};

} // namespace contract_json
namespace holder::api::mcp {
inline bool is_write_tool(std::string_view name) {
  return name == "add_card_link" || name == "add_card_milestone" || name == "add_card_tag" ||
         name == "create_card" || name == "move_card" || name == "remove_card_link" ||
         name == "remove_card_milestone" || name == "remove_card_tag" || name == "restore_card" ||
         name == "trash_card" || name == "update_card" || name == "update_card_milestone";
}
// Authorization must throw on denial. Domain errors propagate to the protocol adapter.
inline nlohmann::json invoke_tool(
    holder_context& context,
    std::string_view name,
    const nlohmann::json& arguments,
    std::string_view default_project,
    const std::function<void(std::string_view)>& authorize_project,
    bool allow_writes = false
) {
  using namespace contract_json;
  if (!arguments.is_object()) throw std::invalid_argument("tool arguments must be an object");
  if (name == "add_card_link") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "link")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("link")) throw std::invalid_argument("link is required");
    auto link = codec<holder::contract::NewLink>::decode(arguments.at("link"));
    authorize_project(project_id);
    if (link.target_project_id && *link.target_project_id != project_id)
      throw std::invalid_argument("MCP links must stay inside the authorized project");
    return json{
        {"data",
         encode(holder::contract::add_card_link(
             context,
             std::move(project_id),
             std::move(card_id),
             link
         ))}
    };
  }
  if (name == "add_card_milestone") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "milestone")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("milestone")) throw std::invalid_argument("milestone is required");
    auto milestone = codec<holder::contract::NewMilestone>::decode(arguments.at("milestone"));
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::add_card_milestone(
             context,
             std::move(project_id),
             std::move(card_id),
             milestone
         ))}
    };
  }
  if (name == "add_card_tag") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "tag")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("tag")) throw std::invalid_argument("tag is required");
    auto tag = check_text(
        decode_field<std::string>(arguments.at("tag"), "tag"),
        "tag",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::add_card_tag(
             context,
             std::move(project_id),
             std::move(card_id),
             std::move(tag)
         ))}
    };
  }
  if (name == "create_card") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("card")) throw std::invalid_argument("card is required");
    auto card = codec<holder::contract::NewCard>::decode(arguments.at("card"));
    authorize_project(project_id);
    return json{
        {"data", encode(holder::contract::create_card(context, std::move(project_id), card))}
    };
  }
  if (name == "get_calendar") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "first_day" && it.key() != "last_day" &&
          it.key() != "time_zone")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("first_day")) throw std::invalid_argument("first_day is required");
    auto first_day = check_text(
        decode_field<std::string>(arguments.at("first_day"), "first_day"),
        "first_day",
        10,
        10
    );
    if (!arguments.contains("last_day")) throw std::invalid_argument("last_day is required");
    auto last_day = check_text(
        decode_field<std::string>(arguments.at("last_day"), "last_day"),
        "last_day",
        10,
        10
    );
    std::optional<std::string> time_zone;
    if (const auto value = arguments.find("time_zone");
        value != arguments.end() && !value->is_null())
      time_zone = check_text(
          decode_field<std::string>(*value, "time_zone"),
          "time_zone",
          1,
          std::numeric_limits<std::size_t>::max()
      );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::get_calendar(
             context,
             std::move(project_id),
             std::move(first_day),
             std::move(last_day),
             std::move(time_zone)
         ))}
    };
  }
  if (name == "get_card") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
  if (name == "list_card_resources") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
         encode(holder::contract::list_card_resources(
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data", encode(holder::contract::list_project_tags(context, std::move(project_id)))}
    };
  }
  if (name == "list_trash") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id") throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
        "project_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data", encode(holder::contract::list_trash(context, std::move(project_id), "card"))}
    };
  }
  if (name == "move_card") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "move")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("move")) throw std::invalid_argument("move is required");
    auto move = codec<holder::contract::CardMove>::decode(arguments.at("move"));
    authorize_project(project_id);
    return json{
        {"data",
         encode(
             holder::contract::move_card(context, std::move(project_id), std::move(card_id), move)
         )}
    };
  }
  if (name == "remove_card_link") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "target_type" &&
          it.key() != "target_id" && it.key() != "kind" && it.key() != "target_project_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("target_type")) throw std::invalid_argument("target_type is required");
    auto target_type = decode_field<holder::contract::RemoveCardLinkTargetType>(
        arguments.at("target_type"),
        "target_type"
    );
    if (!arguments.contains("target_id")) throw std::invalid_argument("target_id is required");
    auto target_id = check_text(
        decode_field<std::string>(arguments.at("target_id"), "target_id"),
        "target_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    auto kind = check_text(
        decode_field<std::string>(arguments.value("kind", json("ref")), "kind"),
        "kind",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    std::optional<std::string> target_project_id;
    if (const auto value = arguments.find("target_project_id");
        value != arguments.end() && !value->is_null())
      target_project_id = check_text(
          decode_field<std::string>(*value, "target_project_id"),
          "target_project_id",
          1,
          std::numeric_limits<std::size_t>::max()
      );
    authorize_project(project_id);
    if (target_project_id && *target_project_id != project_id)
      throw std::invalid_argument("MCP links must stay inside the authorized project");
    auto links = holder::contract::remove_card_link(
        context,
        project_id,
        std::move(card_id),
        target_type,
        std::move(target_id),
        std::move(kind),
        std::move(target_project_id)
    );
    std::erase_if(links, [&project_id](const auto& link) {
      return link.source_project_id != project_id || link.target_project_id != project_id;
    });
    return json{{"data", encode(links)}};
  }
  if (name == "remove_card_milestone") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "milestone_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("milestone_id"))
      throw std::invalid_argument("milestone_id is required");
    auto milestone_id = check_text(
        decode_field<std::string>(arguments.at("milestone_id"), "milestone_id"),
        "milestone_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::remove_card_milestone(
             context,
             std::move(project_id),
             std::move(card_id),
             std::move(milestone_id)
         ))}
    };
  }
  if (name == "remove_card_tag") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "tag")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("tag")) throw std::invalid_argument("tag is required");
    auto tag = check_text(
        decode_field<std::string>(arguments.at("tag"), "tag"),
        "tag",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::remove_card_tag(
             context,
             std::move(project_id),
             std::move(card_id),
             std::move(tag)
         ))}
    };
  }
  if (name == "restore_card") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
         encode(holder::contract::restore_card(context, std::move(project_id), std::move(card_id)))}
    };
  }
  if (name == "search_cards") {
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "q" && it.key() != "limit" &&
          it.key() != "offset")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
  if (name == "trash_card") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
         encode(holder::contract::trash_card(
             context,
             std::move(project_id),
             std::move(card_id),
             holder::contract::TrashCardChildren::Promote
         ))}
    };
  }
  if (name == "update_card") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "update")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("update")) throw std::invalid_argument("update is required");
    auto update = codec<holder::contract::CardUpdate>::decode(arguments.at("update"));
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::update_card(
             context,
             std::move(project_id),
             std::move(card_id),
             update
         ))}
    };
  }
  if (name == "update_card_milestone") {
    if (!allow_writes) throw std::invalid_argument("MCP write access was not granted");
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      if (it.key() != "project_id" && it.key() != "card_id" && it.key() != "milestone_id" &&
          it.key() != "update")
        throw std::invalid_argument("unknown tool argument");
    }
    auto project_id = check_text(
        decode_field<std::string>(
            arguments.value("project_id", json(default_project)),
            "project_id"
        ),
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
    if (!arguments.contains("milestone_id"))
      throw std::invalid_argument("milestone_id is required");
    auto milestone_id = check_text(
        decode_field<std::string>(arguments.at("milestone_id"), "milestone_id"),
        "milestone_id",
        1,
        std::numeric_limits<std::size_t>::max()
    );
    if (!arguments.contains("update")) throw std::invalid_argument("update is required");
    auto update = codec<holder::contract::MilestoneUpdate>::decode(arguments.at("update"));
    authorize_project(project_id);
    return json{
        {"data",
         encode(holder::contract::update_card_milestone(
             context,
             std::move(project_id),
             std::move(card_id),
             std::move(milestone_id),
             update
         ))}
    };
  }
  throw std::invalid_argument("unknown MCP tool");
}
} // namespace holder::api::mcp
