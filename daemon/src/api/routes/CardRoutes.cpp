#include "api/routes/CardRoutes.h"
#include "api/CardLinks.h"
#include "api/support/HttpResponses.h"
#include "api/support/Time.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardPaths.h"
#include "card/CardPlacementResolver.h"
#include "card/CardRepo.h"
#include "card/LinkMutation.h"
#include "card/LinkRepo.h"
#include "card/TagExtractor.h"
#include "card/TagRepo.h"
#include "identity/Uuid.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"
#include "resource/ResourceRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

bool should_include_link_target(
    holder::card::CardRepo& card_repo,
    holder::ai::AiMessageRepo& msg_repo,
    const holder::model::CardLink& link,
    bool include_deleted
) {
  if (include_deleted) return true;
  if (link.to_type == "card") {
    // The source owns external links even when their target cannot be loaded.
    if (link.target_project_id.has_value() && *link.target_project_id != link.project_id)
      return true;
    const auto target = card_repo.get(link.to_card_id);
    return target.has_value() && !target->deleted_at.has_value();
  }
  if (link.to_type == "ai_message") {
    const auto target = msg_repo.get(link.to_card_id);
    return target.has_value() && !target->deleted_at.has_value();
  }
  return true;
}

bool should_include_backlink_source(
    holder::card::CardRepo& card_repo,
    holder::ai::AiMessageRepo& msg_repo,
    const holder::model::CardLink& link,
    bool include_deleted
) {
  if (include_deleted) return true;
  const auto as_card = card_repo.get(link.from_card_id);
  if (as_card.has_value()) {
    return !as_card->deleted_at.has_value();
  }
  const auto as_msg = msg_repo.get(link.from_card_id);
  if (as_msg.has_value()) {
    return !as_msg->deleted_at.has_value();
  }
  return false;
}

bool should_include_external_backlink_source(
    holder::card::CardRepo& card_repo,
    holder::ai::AiMessageRepo& msg_repo,
    holder::ai::AiThreadRepo& thread_repo,
    const holder::model::CardLink& link,
    bool include_deleted
) {
  const auto as_card = card_repo.get(link.from_card_id);
  if (as_card.has_value() && as_card->project_id == link.project_id) {
    return include_deleted || !as_card->deleted_at.has_value();
  }
  const auto as_msg = msg_repo.get(link.from_card_id);
  if (!as_msg.has_value()) return false;
  const auto thread = thread_repo.get(as_msg->thread_id);
  return thread.has_value() && thread->project_id == link.project_id &&
         (include_deleted || !as_msg->deleted_at.has_value());
}

bool project_is_available(const holder::model::Project& project) {
  // The bearer token covers registered projects; only projects with local roots join this view.
  std::error_code error;
  return std::filesystem::is_directory(project.root_path, error);
}

bool validate_link_target(
    holder::platform::Db& db,
    const std::string& project_id,
    const std::string& to_id,
    const std::string& to_type_raw,
    const std::optional<std::string>& target_project_id,
    std::string& error
) {
  try {
    holder::card::validate_link_target(
        db,
        project_id,
        to_id,
        to_type_raw.empty() ? "card" : to_type_raw,
        target_project_id
    );
    return true;
  } catch (const std::invalid_argument& ex) {
    error = ex.what();
    return false;
  }
}

// LCOV_EXCL_START
http::response<http::string_body> privacy_error_response(const holder::privacy::PrivacyError& ex) {
  http::status status = http::status::bad_request;
  if (ex.code() == holder::privacy::PrivacyErrorCode::KeyringUnavailable) {
    status = http::status::service_unavailable;
  }
  return support::error_response(
      status,
      holder::privacy::privacy_error_code_name(ex.code()),
      ex.what()
  );
}
// LCOV_EXCL_STOP

std::optional<std::string> normalize_parent_id(const std::optional<std::string>& parent_card_id) {
  if (!parent_card_id.has_value()) {
    return std::nullopt;
  }
  const std::string& raw = parent_card_id.value();
  const auto start = raw.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::nullopt;
  }
  const auto end = raw.find_last_not_of(" \t\r\n");
  return raw.substr(start, end - start + 1);
}

// The move route's placement algorithm (target/parent lookups, cycle detection, sort keys and
// re-spacing) lives in holder-core (CardStore::move_to), shared with holder-android; this route
// just parses the request, calls it, and maps its
// std::runtime_error vocabulary onto this route's existing HTTP responses. See the "/move"
// handler below.
holder::card::CardPlacementIntent card_placement_intent_from_string(const std::string& intent) {
  if (intent == "into") return holder::card::CardPlacementIntent::Into;
  if (intent == "before") return holder::card::CardPlacementIntent::Before;
  if (intent == "after") return holder::card::CardPlacementIntent::After;
  if (intent == "to_start") return holder::card::CardPlacementIntent::ToStart;
  if (intent == "to_end") return holder::card::CardPlacementIntent::ToEnd;
  if (intent == "left") return holder::card::CardPlacementIntent::Left;
  if (intent == "right") return holder::card::CardPlacementIntent::Right;
  if (intent == "up_level") return holder::card::CardPlacementIntent::UpLevel;
  throw std::runtime_error("invalid_move_intent");
}

bool card_placement_intent_needs_target(holder::card::CardPlacementIntent intent) {
  return intent == holder::card::CardPlacementIntent::Into ||
         intent == holder::card::CardPlacementIntent::Before ||
         intent == holder::card::CardPlacementIntent::After;
}

holder::card::CardPlacementRequest card_placement_request_from_json(const nlohmann::json& body) {
  holder::card::CardPlacementRequest request;
  request.intent = card_placement_intent_from_string(body.at("intent").get<std::string>());
  if (body.contains("target_card_id") && !body.at("target_card_id").is_null()) {
    request.target_card_id = body.at("target_card_id").get<std::string>();
  }
  // An omitted parent_card_id keeps the card's current parent; an explicit null is the
  // project's top level.
  if (body.contains("parent_card_id")) {
    request.has_parent_card_id = true;
    if (!body.at("parent_card_id").is_null()) {
      request.parent_card_id = body.at("parent_card_id").get<std::string>();
    }
  }
  return request;
}

std::optional<int> parse_limit_param(
    const std::string& limit_raw,
    http::response<http::string_body>& res
) {
  int limit = 200;
  if (!limit_raw.empty()) {
    try {
      const long parsed = std::stol(limit_raw);
      if (parsed <= 0 || parsed > 5000) {
        res = support::error_response(
            http::status::bad_request,
            "bad_request",
            "limit must be between 1 and 5000."
        );
        return std::nullopt;
      }
      limit = static_cast<int>(parsed);
    } catch (const std::exception&) {
      res = support::error_response(
          http::status::bad_request,
          "bad_request",
          "Invalid limit. Expected integer."
      );
      return std::nullopt;
    }
  }
  return limit;
}

std::optional<bool> parse_count_param(
    const std::string& count_raw,
    http::response<http::string_body>& res
) {
  if (count_raw.empty()) {
    return false;
  }
  if (count_raw == "1" || count_raw == "true") {
    return true;
  }
  if (count_raw == "0" || count_raw == "false") {
    return false;
  }
  res = support::error_response(
      http::status::bad_request,
      "bad_request",
      "Invalid count. Expected true/false or 1/0."
  );
  return std::nullopt;
}

enum class CardListOrder : std::uint8_t {
  TreeDefault,
  UpdatedDesc,
  TitleAsc,
};

std::optional<CardListOrder> parse_card_order_param(
    const std::string& order_raw,
    CardListOrder default_order,
    http::response<http::string_body>& res
) {
  if (order_raw.empty()) {
    return default_order;
  }
  if (order_raw == "tree_default") {
    return CardListOrder::TreeDefault;
  }
  if (order_raw == "updated_desc") {
    return CardListOrder::UpdatedDesc;
  }
  if (order_raw == "title_asc") {
    return CardListOrder::TitleAsc;
  }
  res = support::error_response(
      http::status::bad_request,
      "bad_request",
      "Invalid order. Expected tree_default, updated_desc, or title_asc."
  );
  return std::nullopt;
}

int compare_title_ci_then_id(const holder::model::Card& a, const holder::model::Card& b) {
  std::string a_lower = a.title;
  std::string b_lower = b.title;
  std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  std::transform(b_lower.begin(), b_lower.end(), b_lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (a_lower < b_lower) return -1;
  if (a_lower > b_lower) return 1;
  if (a.card_id < b.card_id) return -1;
  if (a.card_id > b.card_id) return 1;
  return 0; // LCOV_EXCL_LINE
}

void sort_cards_by_order(std::vector<holder::model::Card>& cards, CardListOrder order) {
  std::sort(cards.begin(), cards.end(), [order](const auto& a, const auto& b) {
    if (order == CardListOrder::TreeDefault) {
      if (a.sort_key < b.sort_key) return true;
      if (a.sort_key > b.sort_key) return false;
      if (a.updated_at > b.updated_at) return true;
      if (a.updated_at < b.updated_at) return false;
      return compare_title_ci_then_id(a, b) < 0;
    }
    if (order == CardListOrder::UpdatedDesc) {
      if (a.updated_at > b.updated_at) return true;
      if (a.updated_at < b.updated_at) return false;
      if (a.created_at > b.created_at) return true;
      if (a.created_at < b.created_at) return false;
      return compare_title_ci_then_id(a, b) < 0;
    }
    return compare_title_ci_then_id(a, b) < 0;
  });
}

http::response<http::string_body> recent_cards_response(
    holder::platform::Db& db,
    const std::string& project_id,
    int limit,
    CardListOrder order,
    bool include_count
) {
  holder::card::CardRepo repo(db);
  auto cards = repo.list_all(project_id);
  std::vector<holder::model::Card> visible;
  visible.reserve(cards.size());
  for (const auto& c : cards) {
    if (!c.deleted_at.has_value()) {
      visible.push_back(c);
    }
  }
  sort_cards_by_order(visible, order);

  nlohmann::json data = nlohmann::json::array();
  const size_t out_count = std::min(static_cast<size_t>(limit), visible.size());
  for (size_t i = 0; i < out_count; ++i) {
    const auto& card = visible[i];
    nlohmann::json item;
    item["card_id"] = card.card_id;
    item["project_id"] = card.project_id;
    item["title"] = card.title;
    item["rel_path"] = card.rel_path;
    item["sort_key"] = card.sort_key;
    item["created_at"] = card.created_at;
    item["updated_at"] = card.updated_at;
    item["parent_card_id"] = card.parent_card_id.has_value()
                                 ? nlohmann::json(card.parent_card_id.value())
                                 : nlohmann::json(nullptr);
    item["deleted_at"] = nlohmann::json(nullptr);
    if (include_count) {
      item["child_count"] = repo.count_children_not_deleted(project_id, card.card_id);
    }
    data.push_back(std::move(item));
  }

  nlohmann::json payload;
  payload["ok"] = true;
  payload["data"] = std::move(data);
  return support::json_response(http::status::ok, payload);
}

} // namespace

CardLinkListResponse list_card_links(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string card_id,
    std::optional<int> include_deleted
) {
  holder::card::LinkRepo links(db);
  holder::card::CardRepo card_repo(db);
  const auto source = card_repo.get(card_id);
  if (!source.has_value()) throw CardLinkSourceNotFound{};
  holder::ai::AiMessageRepo msg_repo(db, fts);
  CardLinkListResponse response{true, {}};
  for (const auto& link : links.list_outgoing(source->project_id, card_id)) {
    if (!should_include_link_target(card_repo, msg_repo, link, include_deleted.value_or(0) == 1)) {
      continue;
    }
    response.data.push_back({link.from_card_id,
                             link.to_card_id,
                             link.target_project_id,
                             std::nullopt,
                             link.to_type,
                             link.kind,
                             std::optional<std::optional<std::string>>{std::in_place, link.label},
                             link.created_at});
  }
  return response;
}

CardBacklinkListResponse list_card_backlinks(
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    std::string card_id,
    std::optional<int> include_deleted_value,
    std::string scope
) {
  holder::card::CardRepo card_repo(db);
  const auto card = card_repo.get(card_id);
  if (!card.has_value()) throw CardLinkSourceNotFound{};
  holder::card::LinkRepo repo(db);
  holder::ai::AiMessageRepo msg_repo(db, fts);
  const bool include_deleted = include_deleted_value.value_or(0) == 1;
  const bool include_local = scope != "external";
  const bool include_external = scope != "local";
  std::vector<holder::model::CardLink> links;
  if (include_local) {
    links = repo.list_backlinks_typed(card->project_id, card->card_id, "card");
  }
  if (include_external) {
    holder::project::ProjectRepo projects(db);
    const auto target_project = projects.get(card->project_id);
    if (!target_project.has_value() || !project_is_available(*target_project)) {
      throw CardLinkSourceNotFound{};
    }
    std::vector<std::string> available_sources;
    for (const auto& project : projects.list()) {
      if (project.project_id != card->project_id && project_is_available(project)) {
        available_sources.push_back(project.project_id);
      }
    }
    auto external =
        repo.list_external_backlinks_typed(card->project_id, card->card_id, "card", available_sources);
    links.insert(links.end(), external.begin(), external.end());
  }
  std::sort(links.begin(), links.end(), [](const auto& left, const auto& right) {
    if (left.created_at != right.created_at) return left.created_at > right.created_at;
    if (left.project_id != right.project_id) return left.project_id < right.project_id;
    if (left.from_card_id != right.from_card_id) return left.from_card_id < right.from_card_id;
    return left.kind < right.kind;
  });

  CardBacklinkListResponse response{
      true, {}, scope, include_external ? "available_projects" : "target_project"};
  holder::ai::AiThreadRepo thread_repo(db);
  for (const auto& link : links) {
    const bool external = link.project_id != card->project_id;
    const bool include_source =
        external ? should_include_external_backlink_source(
                       card_repo, msg_repo, thread_repo, link, include_deleted
                   )
                 : should_include_backlink_source(card_repo, msg_repo, link, include_deleted);
    if (!include_source) continue;
    response.data.push_back({link.from_card_id,
                             link.to_card_id,
                             external ? link.target_project_id : std::nullopt,
                             external ? std::optional<std::string>(link.project_id) : std::nullopt,
                             link.to_type,
                             link.kind,
                             std::optional<std::optional<std::string>>{std::in_place, link.label},
                             link.created_at});
  }
  return response;
}

http::response<http::string_body> card_create_error_response(const std::exception& ex) {
  const std::string msg = ex.what();
  if (msg.rfind("conflict:", 0) == 0) {
    return support::error_response(http::status::conflict, "conflict", msg);
  }
  return support::error_response(http::status::bad_request, "bad_request", msg);
}

bool handle_card_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts,
    const std::function<std::string()>& /*uuid_v4*/,
    const std::function<std::string(const std::string&)>& param_get
) {
  if (path == "/cards/context" && req.method() == http::verb::get) {
    const std::string project_id = param_get("project_id");
    const std::string parent_raw = param_get("parent_card_id");
    const std::string order_raw = param_get("order");
    const std::string count_raw = param_get("count");
    if (project_id.empty()) {
      res =
          support::error_response(http::status::bad_request, "bad_request", "Missing project_id.");
      return true;
    }

    try {
      holder::project::ProjectRepo project_repo(db);
      const auto project_opt = project_repo.get(project_id);
      if (!project_opt.has_value()) {
        res = support::error_response(http::status::not_found, "not_found", "Project not found.");
        return true;
      }

      holder::card::CardRepo card_repo(db);
      const auto all_cards = card_repo.list_all(project_id);
      std::unordered_map<std::string, holder::model::Card> cards_by_id;
      cards_by_id.reserve(all_cards.size());
      for (const auto& c : all_cards) {
        if (!c.deleted_at.has_value()) {
          cards_by_id[c.card_id] = c;
        }
      }

      std::optional<std::string> parent_card_id = std::nullopt;
      if (!parent_raw.empty()) {
        parent_card_id = normalize_parent_id(std::optional<std::string>(parent_raw));
      }

      if (parent_card_id.has_value()) {
        const auto parent_it = cards_by_id.find(parent_card_id.value());
        if (parent_it == cards_by_id.end()) {
          res = support::error_response(
              http::status::not_found,
              "not_found",
              "Parent card not found."
          );
          return true;
        }
      }

      std::unordered_map<std::string, int> child_count_by_parent;
      for (const auto& [card_id, card] : cards_by_id) {
        const auto parent = normalize_parent_id(card.parent_card_id);
        if (parent.has_value()) {
          const auto& parent_id = *parent;
          if (cards_by_id.find(parent_id) != cards_by_id.end()) {
            child_count_by_parent[parent_id] += 1;
          }
        }
      }

      std::vector<holder::model::Card> level_cards;
      const std::string parent_card_id_value = parent_card_id.value_or(std::string());
      if (!parent_card_id.has_value()) {
        level_cards = card_repo.list_roots(project_id);
      } else {
        level_cards = card_repo.list_children(project_id, parent_card_id_value);
      }
      const auto order = parse_card_order_param(order_raw, CardListOrder::TreeDefault, res);
      if (!order.has_value()) {
        return true;
      }
      const auto include_count = parse_count_param(count_raw, res);
      if (!include_count.has_value()) {
        return true;
      }
      const auto selected_order = *order;
      const bool should_include_count = *include_count;
      sort_cards_by_order(level_cards, selected_order);

      nlohmann::json cards_json = nlohmann::json::array();
      for (const auto& card : level_cards) {
        if (card.deleted_at.has_value()) {
          continue; // LCOV_EXCL_LINE
        }
        nlohmann::json item;
        item["card_id"] = card.card_id;
        item["project_id"] = card.project_id;
        item["title"] = card.title;
        item["rel_path"] = card.rel_path;
        item["sort_key"] = card.sort_key;
        item["created_at"] = card.created_at;
        item["updated_at"] = card.updated_at;
        item["parent_card_id"] = card.parent_card_id.has_value()
                                     ? nlohmann::json(*card.parent_card_id)
                                     : nlohmann::json(nullptr);
        item["deleted_at"] = nlohmann::json(nullptr);
        if (should_include_count) {
          const auto child_it = child_count_by_parent.find(card.card_id);
          item["child_count"] = child_it == child_count_by_parent.end() ? 0 : child_it->second;
        }
        cards_json.push_back(std::move(item));
      }

      nlohmann::json breadcrumbs = nlohmann::json::array();
      breadcrumbs.push_back({
          {"type", "project"},
          {"project_id", project_opt->project_id},
          {"title", project_opt->name},
      });

      if (parent_card_id.has_value()) {
        std::vector<holder::model::Card> chain;
        std::optional<std::string> cursor = parent_card_id;
        int guard = 0;
        while (cursor.has_value() && guard < 1024) {
          const std::string cursor_id = cursor.value_or(std::string());
          const auto it = cards_by_id.find(cursor_id);
          if (it == cards_by_id.end()) {
            break;
          }
          chain.push_back(it->second);
          cursor = normalize_parent_id(it->second.parent_card_id);
          guard++;
        }
        std::reverse(chain.begin(), chain.end());
        for (const auto& card : chain) {
          breadcrumbs.push_back({
              {"type", "card"},
              {"card_id", card.card_id},
              {"title", card.title},
          });
        }
      }

      nlohmann::json data;
      data["project"] = {
          {"project_id", project_opt->project_id},
          {"name", project_opt->name},
      };
      data["current_parent_card_id"] = parent_card_id.has_value()
                                           ? nlohmann::json(parent_card_id_value)
                                           : nlohmann::json(nullptr);
      data["breadcrumbs"] = std::move(breadcrumbs);
      data["cards"] = std::move(cards_json);

      nlohmann::json payload;
      payload["ok"] = true;
      payload["data"] = std::move(data);
      res = support::json_response(http::status::ok, payload);
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    }
    return true;
  }

  if (path == "/cards" && req.method() == http::verb::get) {
    const std::string project_id = param_get("project_id");
    const std::string tag_raw = param_get("tag");
    const std::string parent_raw = param_get("parent_card_id");
    const std::string view_raw = param_get("view");
    const std::string order_raw = param_get("order");
    const std::string count_raw = param_get("count");
    const std::string limit_raw = param_get("limit");
    const std::string include_deleted_raw = param_get("include_deleted");
    if (project_id.empty()) {
      res =
          support::error_response(http::status::bad_request, "bad_request", "Missing project_id.");
    } else {
      try {
        holder::card::CardRepo repo(db);
        if (!tag_raw.empty()) {
          holder::card::TagRepo tag_repo(db);
          nlohmann::json data = nlohmann::json::array();
          for (const auto& card_id : tag_repo.list_card_ids_with_tag(project_id, tag_raw)) {
            // list_card_ids_with_tag itself now guarantees a trashed card's id never comes back
            // (see TagRepo), so this only guards against the card being genuinely missing.
            const auto card_opt = repo.get(card_id);
            // LCOV_EXCL_START: TagRepo's foreign key and live-card filter guarantee this row.
            if (!card_opt.has_value()) {
              continue;
            }
            // LCOV_EXCL_STOP
            const auto& card = card_opt.value();
            nlohmann::json item;
            item["card_id"] = card.card_id;
            item["project_id"] = card.project_id;
            item["title"] = card.title;
            item["rel_path"] = card.rel_path;
            item["sort_key"] = card.sort_key;
            item["created_at"] = card.created_at;
            item["updated_at"] = card.updated_at;
            item["parent_card_id"] = card.parent_card_id.has_value()
                                         ? nlohmann::json(card.parent_card_id.value())
                                         : nlohmann::json(nullptr);
            item["deleted_at"] = nlohmann::json(nullptr);
            data.push_back(std::move(item));
          }
          nlohmann::json payload;
          payload["ok"] = true;
          payload["data"] = std::move(data);
          res = support::json_response(http::status::ok, payload);
          return true;
        }
        const std::string view = view_raw.empty() ? "tree" : view_raw;
        const auto include_count = parse_count_param(count_raw, res);
        if (!include_count.has_value()) return true;

        std::vector<holder::model::Card> cards;
        if (view == "tree") {
          const auto order = parse_card_order_param(order_raw, CardListOrder::TreeDefault, res);
          if (!order.has_value()) return true;
          cards = parent_raw.empty() ? repo.list_roots(project_id)
                                     : repo.list_children(project_id, parent_raw);
          sort_cards_by_order(cards, order.value());
        } else if (view == "recent") {
          const auto order = parse_card_order_param(order_raw, CardListOrder::UpdatedDesc, res);
          if (!order.has_value()) return true;
          const auto limit = parse_limit_param(limit_raw, res);
          if (!limit.has_value()) return true;
          res = recent_cards_response(
              db,
              project_id,
              limit.value(),
              order.value(),
              include_count.value()
          );
          return true;
        } else {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "Invalid view. Expected tree or recent."
          );
          return true;
        }

        nlohmann::json data = nlohmann::json::array();
        for (const auto& card : cards) {
          if (!include_deleted_raw.empty() && include_deleted_raw != "0") {
            // include deleted
          } else if (card.deleted_at.has_value()) {
            continue;
          }
          nlohmann::json item;
          item["card_id"] = card.card_id;
          item["project_id"] = card.project_id;
          item["title"] = card.title;
          item["rel_path"] = card.rel_path;
          item["sort_key"] = card.sort_key;
          item["created_at"] = card.created_at;
          item["updated_at"] = card.updated_at;
          item["parent_card_id"] = card.parent_card_id.has_value()
                                       ? nlohmann::json(card.parent_card_id.value())
                                       : nlohmann::json(nullptr);
          item["deleted_at"] = card.deleted_at.has_value() ? nlohmann::json(card.deleted_at.value())
                                                           : nlohmann::json(nullptr);
          if (include_count.value()) {
            item["child_count"] = repo.count_children_not_deleted(project_id, card.card_id);
          }
          data.push_back(std::move(item));
        }
        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = data;
        res = support::json_response(http::status::ok, payload);
      } catch (const std::exception& ex) {
        res = support::error_response(http::status::internal_server_error, "error", ex.what());
      }
    }
    return true;
  }

  if (path == "/cards" && req.method() == http::verb::post) {
    if (!card_store) {
      res = support::error_response(
          http::status::not_implemented,
          "not_implemented",
          "Card store unavailable."
      );
    } else {
      try {
        const auto body = nlohmann::json::parse(req.body());
        if (body.contains("card_id")) {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "card_id is generated by Holder and must not be supplied."
          );
        } else if (!body.contains("project_id") || !body.contains("title") ||
                   !body.contains("content")) {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "Missing required fields."
          );
        } else {
          holder::model::Card card;
          card.project_id = body.at("project_id").get<std::string>();
          holder::project::ProjectRepo project_repo(db);
          const auto project = project_repo.get(card.project_id);
          if (!project.has_value()) {
            res =
                support::error_response(http::status::not_found, "not_found", "Project not found.");
            return true;
          }
          card.card_id = holder::identity::generate_id(project->id_scheme);
          card.title = body.at("title").get<std::string>();
          if (body.contains("created_at") && !body.at("created_at").is_null()) {
            card.created_at = body.at("created_at").get<long long>();
          }
          if (body.contains("updated_at") && !body.at("updated_at").is_null()) {
            card.updated_at = body.at("updated_at").get<long long>();
          }
          if (card.created_at <= 0) {
            card.created_at = support::now_epoch_seconds();
          }
          if (card.updated_at <= 0) {
            card.updated_at = card.created_at;
          }
          if (body.contains("parent_card_id") && !body.at("parent_card_id").is_null()) {
            card.parent_card_id = body.at("parent_card_id").get<std::string>();
          }
          const bool has_sort_key = body.contains("sort_key");
          if (has_sort_key) {
            card.sort_key = body.at("sort_key").get<double>();
          }
          if (body.contains("rel_path") && !body.at("rel_path").is_null()) {
            card.rel_path = body.at("rel_path").get<std::string>();
          }

          const std::string content = body.at("content").get<std::string>();
          if (card.rel_path.empty()) {
            card.rel_path = holder::core::card_rel_path(card.card_id);
          }
          std::optional<double> explicit_sort_key;
          if (has_sort_key) {
            explicit_sort_key = card.sort_key;
          }
          card_store->create(card, content, explicit_sort_key);

          nlohmann::json data;
          data["card_id"] = card.card_id;
          data["rel_path"] = card.rel_path;
          nlohmann::json payload;
          payload["ok"] = true;
          payload["data"] = data;
          res = support::json_response(http::status::created, payload);
        }
      } catch (const holder::privacy::PrivacyError& ex) {
        res = privacy_error_response(ex); // LCOV_EXCL_LINE
      } catch (const std::exception& ex) {
        res = card_create_error_response(ex);
      }
    }
    return true;
  }

  if (path.rfind("/cards/", 0) == 0) {
    const std::string rest = path.substr(std::string("/cards/").size());
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
      const std::string card_id = rest.substr(0, slash);
      const std::string tail = rest.substr(slash);
      if (card_id.empty()) {
        res = support::error_response(http::status::not_found, "not_found", "Route not found.");
        return true;
      }
      if (!card_store) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
      } else if (tail == "/move") {
        if (req.method() != http::verb::post) {
          res = support::error_response(
              http::status::method_not_allowed,
              "method_not_allowed",
              "Method not allowed."
          );
        } else {
          std::optional<holder::card::CardPlacementRequest> request;
          try {
            const auto body = nlohmann::json::parse(req.body());
            if (!body.contains("project_id") || !body.contains("intent")) {
              res = support::error_response(
                  http::status::bad_request,
                  "bad_request",
                  "Missing required fields."
              );
              return true;
            }

            const std::string project_id = body.at("project_id").get<std::string>();
            // Throws (caught below) for a non-string "intent" -- a plain nlohmann type_error,
            // mapped to bad_request by the generic catch -- or an unrecognized intent string,
            // a std::runtime_error("invalid_move_intent") mapped explicitly below.
            request = card_placement_request_from_json(body);

            // Resolves under the project lock and writes the card with any siblings re-spaced to
            // make room for it, in one commit.
            const auto result = card_store->move_to(
                project_id,
                card_id,
                request.value(),
                support::now_epoch_seconds()
            );
            const auto& moved_card = result.card;

            nlohmann::json data;
            data["card_id"] = moved_card.card_id;
            data["parent_card_id"] = moved_card.parent_card_id.has_value()
                                         ? nlohmann::json(moved_card.parent_card_id.value())
                                         : nlohmann::json(nullptr);
            data["sort_key"] = moved_card.sort_key;
            data["revision"] = moved_card.updated_at;
            data["moved_into_title"] = result.moved_into_title.has_value()
                                           ? nlohmann::json(result.moved_into_title.value())
                                           : nlohmann::json(nullptr);

            nlohmann::json payload;
            payload["ok"] = true;
            payload["data"] = std::move(data);
            res = support::json_response(http::status::ok, payload);
          } catch (const holder::privacy::PrivacyError& ex) {
            res = privacy_error_response(ex); // LCOV_EXCL_LINE
          } catch (const std::runtime_error& ex) {
            const std::string msg = ex.what();
            if (msg == "card_not_found") {
              res =
                  support::error_response(http::status::not_found, "not_found", "Card not found.");
            } else if (msg == "cross_project_move_forbidden") {
              res = support::error_response(
                  http::status::unprocessable_entity,
                  "cross_project_move_forbidden",
                  "Source card is in a different project."
              );
            } else if (msg == "target_not_found") {
              const bool target_style = request.has_value() &&
                                        card_placement_intent_needs_target(request->intent);
              res = support::error_response(
                  http::status::not_found,
                  "target_not_found",
                  target_style ? "Target card not found." : "Parent card not found."
              );
            } else if (msg == "move_would_create_cycle") {
              res = support::error_response(
                  http::status::unprocessable_entity,
                  "move_would_create_cycle",
                  "Move would create a cycle."
              );
            } else if (msg == "invalid_target") {
              res = support::error_response(
                  http::status::unprocessable_entity,
                  "invalid_target",
                  "Target card is invalid for requested move."
              );
            } else if (msg == "missing_target_card_id") {
              res = support::error_response(
                  http::status::bad_request,
                  "missing_target_card_id",
                  "target_card_id is required for this intent."
              );
            } else if (msg == "already_at_project_root") {
              res = support::error_response(
                  http::status::unprocessable_entity,
                  "invalid_move_intent",
                  "Card is already at project root."
              );
            } else if (msg == "invalid_move_intent") {
              res = support::error_response(
                  http::status::bad_request,
                  "invalid_move_intent",
                  "Unknown move intent."
              );
            } else {
              // LCOV_EXCL_START
              res = support::error_response(http::status::bad_request, "bad_request", msg);
              // LCOV_EXCL_STOP
            }
          } catch (const std::exception& ex) {
            res = support::error_response(http::status::bad_request, "bad_request", ex.what());
          }
        }
      } else if (tail == "/tags") {
        if (req.method() != http::verb::post && req.method() != http::verb::delete_) {
          res = support::error_response(
              http::status::method_not_allowed,
              "method_not_allowed",
              "Method not allowed."
          );
        } else {
          try {
            const auto body = nlohmann::json::parse(req.body());
            if (!body.contains("project_id") || !body.contains("tag") ||
                body.at("project_id").is_null() || body.at("tag").is_null()) {
              res = support::error_response(
                  http::status::bad_request,
                  "bad_request",
                  "project_id and tag are required."
              );
              return true;
            }

            const auto project_id = body.at("project_id").get<std::string>();
            const auto tag = body.at("tag").get<std::string>();
            if (project_id.empty()) {
              res = support::error_response(
                  http::status::bad_request,
                  "bad_request",
                  "project_id must not be empty."
              );
              return true;
            }
            const auto card_opt = card_store->get(card_id);
            if (!card_opt.has_value() || card_opt->deleted_at.has_value()) {
              res =
                  support::error_response(http::status::not_found, "not_found", "Card not found.");
              return true;
            }
            if (card_opt->project_id != project_id) {
              res = support::error_response(
                  http::status::unprocessable_entity,
                  "cross_project_tag_forbidden",
                  "Card is in a different project."
              );
              return true;
            }

            std::string outcome;
            bool changed = false;
            if (req.method() == http::verb::post) {
              switch (card_store->add_tag(card_id, tag, support::now_epoch_seconds())) {
              case holder::card::AddTagResult::Added:
                outcome = "added";
                changed = true;
                break;
              case holder::card::AddTagResult::AlreadyPresent:
                outcome = "already_present";
                break;
              case holder::card::AddTagResult::InvalidTag:
                res = support::error_response(
                    http::status::bad_request,
                    "invalid_tag",
                    "Invalid tag: " + tag + "."
                );
                return true;
              }
            } else {
              switch (card_store->remove_tag(card_id, tag, support::now_epoch_seconds())) {
              case holder::card::RemoveTagResult::Removed:
                outcome = "removed";
                changed = true;
                break;
              case holder::card::RemoveTagResult::NotPresent:
                outcome = "not_present";
                break;
              case holder::card::RemoveTagResult::PresentOutsideEditableTagLine:
                outcome = "present_outside_editable_tag_line";
                break;
              case holder::card::RemoveTagResult::InvalidTag:
                res = support::error_response(
                    http::status::bad_request,
                    "invalid_tag",
                    "Invalid tag: " + tag + "."
                );
                return true;
              }
            }

            const nlohmann::json payload = {
                {"ok", true},
                {"data",
                 {{"card_id", card_id},
                  {"tag", holder::core::normalize_tag(tag)},
                  {"outcome", outcome},
                  {"changed", changed}}},
            };
            res = support::json_response(http::status::ok, payload);
          } catch (const holder::privacy::PrivacyError& ex) {
            res = privacy_error_response(ex); // LCOV_EXCL_LINE
          } catch (const std::exception& ex) {
            res = support::error_response(http::status::bad_request, "bad_request", ex.what());
          }
        }
      } else if (tail == "/resources") {
        if (req.method() != http::verb::post && req.method() != http::verb::delete_) {
          res = support::error_response(
              http::status::method_not_allowed,
              "method_not_allowed",
              "Use POST to attach or DELETE to detach."
          );
          return true;
        }
        try {
          const auto body = nlohmann::json::parse(req.body());
          const auto project_id = body.at("project_id").get<std::string>();
          const auto resource_id = body.at("resource_id").get<std::string>();
          if (project_id.empty() || resource_id.empty()) {
            throw std::invalid_argument("project_id and resource_id are required.");
          }
          const auto card = card_store->get(card_id);
          if (!card.has_value() || card->deleted_at.has_value()) {
            res = support::error_response(
                http::status::not_found,
                "not_found",
                "Live card not found."
            );
            return true;
          }
          const auto resource = holder::resource::ResourceRepo(db).get(resource_id);
          const bool attaching = req.method() == http::verb::post;
          if (!resource.has_value() && attaching) {
            res = support::error_response(
                http::status::not_found,
                "not_found",
                "Resource not found."
            );
            return true;
          }
          if (card->project_id != project_id ||
              (resource.has_value() && resource->project_id != project_id)) {
            res = support::error_response(
                http::status::unprocessable_entity,
                "cross_project_resource_forbidden",
                "Card and resource must belong to the selected project."
            );
            return true;
          }
          holder::card::LinkRepo links(db);
          const auto outgoing = links.list_outgoing(project_id, card_id);
          const bool present = std::any_of(outgoing.begin(), outgoing.end(), [&](const auto& link) {
            return link.to_card_id == resource_id && link.to_type == "resource" &&
                   link.kind == "attachment";
          });
          const bool changed = attaching != present;
          if (changed) {
            if (attaching) {
              holder::model::CardLink link;
              link.project_id = project_id;
              link.from_card_id = card_id;
              link.to_card_id = resource_id;
              link.to_type = "resource";
              link.kind = "attachment";
              link.created_at = support::now_epoch_seconds();
              holder::card::add_card_link(db, fts, link);
            } else {
              holder::card::remove_card_link(
                  db,
                  fts,
                  card_id,
                  resource_id,
                  "resource",
                  "attachment"
              );
            }
          }
          res = support::json_response(
              http::status::ok,
              {{"ok", true},
               {"data",
                {{"card_id", card_id},
                 {"resource_id", resource_id},
                 {"changed", changed},
                 {"outcome",
                  attaching ? (changed ? "attached" : "already_attached")
                            : (changed ? "detached" : "not_attached")}}}}
          );
        } catch (const holder::privacy::PrivacyError& ex
        ) { // LCOV_EXCL_LINE: injected crypto failure guard.
          res = privacy_error_response(ex); // LCOV_EXCL_LINE
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::bad_request, "bad_request", ex.what());
        }
      } else if (tail == "/links") {
        try {
          const auto card_opt = card_store->get(card_id);
          if (!card_opt.has_value()) {
            res = support::error_response(http::status::not_found, "not_found", "Card not found.");
          } else {
            const auto& card = card_opt.value();
            holder::card::LinkRepo repo(db);
            if (req.method() == http::verb::post) {
              const auto body = nlohmann::json::parse(req.body());
              if (!body.contains("to_card_id")) {
                res = support::error_response(
                    http::status::bad_request,
                    "bad_request",
                    "Missing to_card_id."
                );
              } else {
                holder::model::CardLink link;
                link.project_id = card.project_id;
                link.from_card_id = card.card_id;
                link.to_card_id = body.at("to_card_id").get<std::string>();
                if (body.contains("to_type") && !body.at("to_type").is_null()) {
                  link.to_type = body.at("to_type").get<std::string>();
                }
                if (body.contains("kind") && !body.at("kind").is_null()) {
                  link.kind = body.at("kind").get<std::string>();
                }
                if (link.to_type.empty()) {
                  link.to_type = "card";
                }
                if (link.kind.empty()) {
                  link.kind = "ref";
                }
                if (body.contains("label") && !body.at("label").is_null()) {
                  link.label = body.at("label").get<std::string>();
                }
                if (body.contains("target_project_id") && !body.at("target_project_id").is_null()) {
                  link.target_project_id = body.at("target_project_id").get<std::string>();
                  if (link.target_project_id == card.project_id) link.target_project_id.reset();
                }
                if (body.contains("created_at") && !body.at("created_at").is_null()) {
                  link.created_at = body.at("created_at").get<long long>();
                } else {
                  link.created_at = support::now_epoch_seconds();
                }
                std::string validation_error;
                if (!validate_link_target(
                        db,
                        card.project_id,
                        link.to_card_id,
                        link.to_type,
                        link.target_project_id,
                        validation_error
                    )) {
                  res = support::error_response(
                      http::status::bad_request,
                      "bad_request",
                      validation_error
                  );
                } else {
                  holder::card::add_card_link(db, fts, link);
                  for (const auto& saved : repo.list_outgoing(card.project_id, card.card_id)) {
                    if (saved.to_card_id == link.to_card_id && saved.to_type == link.to_type &&
                        saved.kind == link.kind &&
                        saved.target_project_id == link.target_project_id) {
                      link.created_at = saved.created_at;
                      break;
                    }
                  }

                  nlohmann::json payload;
                  payload["ok"] = true;
                  payload["data"] = {
                      {"from_card_id", link.from_card_id},
                      {"to_card_id", link.to_card_id},
                      {"to_type", link.to_type},
                      {"kind", link.kind},
                      {"created_at", link.created_at},
                      {"label",
                       link.label.has_value() ? nlohmann::json(link.label.value())
                                              : nlohmann::json(nullptr)},
                  };
                  if (link.target_project_id.has_value())
                    payload["data"]["target_project_id"] = *link.target_project_id;
                  res = support::json_response(http::status::created, payload);
                }
              }
            } else if (req.method() == http::verb::delete_) {
              std::optional<std::string> to_card_id;
              std::optional<std::string> to_type;
              std::optional<std::string> kind;
              std::optional<std::string> target_project_id;
              if (!req.body().empty()) {
                const auto body = nlohmann::json::parse(req.body());
                if (body.contains("to_card_id") && !body.at("to_card_id").is_null()) {
                  to_card_id = body.at("to_card_id").get<std::string>();
                }
                if (body.contains("to_type") && !body.at("to_type").is_null()) {
                  to_type = body.at("to_type").get<std::string>();
                }
                if (body.contains("kind") && !body.at("kind").is_null()) {
                  kind = body.at("kind").get<std::string>();
                }
                if (body.contains("target_project_id") && !body.at("target_project_id").is_null()) {
                  target_project_id = body.at("target_project_id").get<std::string>();
                }
              }
              if (target_project_id.has_value() &&
                  (!to_card_id.has_value() || !to_type.has_value() || !kind.has_value())) {
                throw std::invalid_argument(
                    "to_card_id, to_type and kind are required with target_project_id."
                );
              }
              if (to_card_id.has_value() && to_type.has_value() && kind.has_value()) {
                holder::card::remove_card_link(
                    db, fts, card.card_id, *to_card_id, *to_type, *kind, target_project_id
                );
              } else if (to_card_id.has_value()) {
                repo.delete_link(card.project_id, card.card_id, to_card_id.value(), to_type, kind);
              } else {
                repo.delete_links_from(card.project_id, card.card_id);
              }
              if (!to_card_id.has_value() || !to_type.has_value() || !kind.has_value())
                card_store->update_links(card.card_id, support::now_epoch_seconds());

              nlohmann::json payload;
              payload["ok"] = true;
              payload["data"] = {{"card_id", card.card_id}};
              res = support::json_response(http::status::ok, payload);
            } else {
              res = support::error_response(
                  http::status::method_not_allowed,
                  "method_not_allowed",
                  "Method not allowed."
              );
            }
          }
        } catch (const holder::privacy::PrivacyError& ex) {
          res = privacy_error_response(ex); // LCOV_EXCL_LINE
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::bad_request, "bad_request", ex.what());
        }
      } else if (tail == "/restore") {
        if (req.method() != http::verb::post) {
          res = support::error_response(
              http::status::method_not_allowed,
              "method_not_allowed",
              "Method not allowed."
          );
        } else {
          try {
            const long long updated_at = support::now_epoch_seconds();
            card_store->restore(card_id, updated_at);
            nlohmann::json payload;
            payload["ok"] = true;
            payload["data"] = {{"card_id", card_id}};
            res = support::json_response(http::status::ok, payload);
          } catch (const holder::privacy::PrivacyError& ex) {
            res = privacy_error_response(ex); // LCOV_EXCL_LINE
          } catch (const std::exception& ex) {
            res = support::error_response(http::status::bad_request, "bad_request", ex.what());
          }
        }
      } else {
        res = support::error_response(http::status::not_found, "not_found", "Route not found.");
      }
    } else {
      const std::string& card_id = rest;
      if (card_id.empty()) {
        res = support::error_response(http::status::not_found, "not_found", "Route not found.");
        return true;
      }
      if (!card_store) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
      } else if (req.method() == http::verb::get) {
        try {
          const auto card_opt = card_store->get(card_id);
          if (!card_opt.has_value()) {
            res = support::error_response(http::status::not_found, "not_found", "Card not found.");
          } else {
            const auto& card = card_opt.value();
            if (card.deleted_at.has_value()) {
              res =
                  support::error_response(http::status::not_found, "not_found", "Card not found.");
            } else {
              const auto content_opt = card_store->get_content(card);
              if (!content_opt.has_value()) {
                res = support::error_response(
                    http::status::not_found,
                    "not_found",
                    "Card content missing."
                );
              } else {
                nlohmann::json data;
                data["card_id"] = card.card_id;
                data["project_id"] = card.project_id;
                data["title"] = card.title;
                data["rel_path"] = card.rel_path;
                data["sort_key"] = card.sort_key;
                data["created_at"] = card.created_at;
                data["updated_at"] = card.updated_at;
                data["parent_card_id"] = card.parent_card_id.has_value()
                                             ? nlohmann::json(card.parent_card_id.value())
                                             : nlohmann::json(nullptr);
                data["deleted_at"] = card.deleted_at.has_value()
                                         ? nlohmann::json(card.deleted_at.value())
                                         : nlohmann::json(nullptr);
                data["content"] = content_opt.value();
                holder::card::TagRepo tag_repo(db);
                data["tags"] = tag_repo.list_tags_for_card(card.project_id, card.card_id);
                data["tag_occurrences"] = nlohmann::json::array();
                for (const auto& occurrence :
                     holder::core::extract_tag_occurrences(content_opt.value())) {
                  data["tag_occurrences"].push_back({
                      {"tag", occurrence.tag},
                      {"byte_start", occurrence.byte_start},
                      {"byte_end", occurrence.byte_end},
                  });
                }

                nlohmann::json payload;
                payload["ok"] = true;
                payload["data"] = data;
                res = support::json_response(http::status::ok, payload);
              }
            }
          }
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::internal_server_error, "error", ex.what());
        }
      } else if (req.method() == http::verb::patch) {
        try {
          const auto body = nlohmann::json::parse(req.body());
          if (!body.contains("updated_at")) {
            res = support::error_response(
                http::status::bad_request,
                "bad_request",
                "Missing required fields."
            );
          } else {
            const bool has_content = body.contains("content");
            const bool has_title = body.contains("title");
            const bool has_parent_card_id = body.contains("parent_card_id");
            const bool has_sort_key = body.contains("sort_key");
            if (!has_content && !has_title && !has_parent_card_id && !has_sort_key) {
              res = support::error_response(
                  http::status::bad_request,
                  "bad_request",
                  "No updatable fields supplied."
              );
              return true;
            }

            std::optional<std::string> title;
            if (has_title && !body.at("title").is_null()) {
              title = body.at("title").get<std::string>();
            }
            const long long updated_at = body.at("updated_at").get<long long>();

            if (has_content || has_title) {
              if (!has_content) {
                res = support::error_response(
                    http::status::bad_request,
                    "bad_request",
                    "content is required when updating title."
                );
                return true;
              }
              const std::string content = body.at("content").get<std::string>();
              card_store->update_content(card_id, content, title, updated_at);
            }

            if (has_parent_card_id || has_sort_key) {
              std::optional<std::string> parent_card_id;
              if (has_parent_card_id && !body.at("parent_card_id").is_null()) {
                parent_card_id = body.at("parent_card_id").get<std::string>();
              }
              std::optional<double> sort_key;
              if (has_sort_key) {
                sort_key = body.at("sort_key").get<double>();
              }
              card_store->move(card_id, has_parent_card_id, parent_card_id, sort_key, updated_at);
            }

            nlohmann::json payload;
            payload["ok"] = true;
            payload["data"] = {{"card_id", card_id}};
            res = support::json_response(http::status::ok, payload);
          }
        } catch (const holder::privacy::PrivacyError& ex) {
          res = privacy_error_response(ex); // LCOV_EXCL_LINE
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::bad_request, "bad_request", ex.what());
        }
      } else if (req.method() == http::verb::delete_) {
        try {
          // promote (the default): live children take the card's place. trash_subtree: every live
          // descendant goes to Trash with it. Checked before anything changes.
          const auto children_raw = param_get("children");
          auto children = holder::card::TrashChildren::Promote;
          if (children_raw == "trash_subtree") {
            children = holder::card::TrashChildren::TrashSubtree;
          } else if (!children_raw.empty() && children_raw != "promote") {
            res = support::error_response(
                http::status::bad_request,
                "bad_request",
                "children must be promote or trash_subtree."
            );
            return true;
          }
          const long long deleted_at = support::now_epoch_seconds();
          card_store->trash(card_id, deleted_at, children);
          nlohmann::json payload;
          payload["ok"] = true;
          payload["data"] = {{"card_id", card_id}};
          res = support::json_response(http::status::ok, payload);
        } catch (const holder::privacy::PrivacyError& ex) {
          res = privacy_error_response(ex); // LCOV_EXCL_LINE
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::bad_request, "bad_request", ex.what());
        }
      } else {
        res = support::error_response(http::status::not_found, "not_found", "Route not found.");
      }
    }
    return true;
  }

  return false;
}

} // namespace holder::api::routes
