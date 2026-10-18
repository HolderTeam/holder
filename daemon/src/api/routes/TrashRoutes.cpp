#include "api/routes/TrashRoutes.h"
#include "api/support/HttpResponses.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

std::optional<std::string> scoped_trash_item(const std::string& path, std::string& rest) {
  static const std::string prefix = "/projects/";
  if (path.rfind(prefix, 0) != 0) return std::nullopt;
  const auto project_end = path.find('/', prefix.size());
  if (project_end == std::string::npos || project_end == prefix.size() ||
      path.compare(project_end, 7, "/trash/") != 0) {
    return std::nullopt;
  }
  rest = path.substr(project_end + 7);
  return path.substr(prefix.size(), project_end - prefix.size());
}

http::response<http::string_body> delete_trashed_item(
    const std::string& type,
    const std::string& id,
    const std::optional<std::string>& expected_project_id,
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts
) {
  if (type == "card") {
    if (!card_store) {
      return support::error_response(
          http::status::not_implemented,
          "not_implemented",
          "Card store unavailable."
      );
    }
    const auto card = holder::card::CardRepo(db).get(id);
    if (!card || (expected_project_id && card->project_id != *expected_project_id)) {
      return support::error_response(http::status::not_found, "not_found", "Card not found.");
    }
    if (!card->deleted_at) {
      return support::error_response(
          http::status::conflict,
          "not_trashed",
          "Card is not in Trash."
      );
    }
    card_store->hard_delete(id, expected_project_id);
    return support::json_response(http::status::ok, {{"ok", true}, {"data", {{"card_id", id}}}});
  }
  if (type == "ai_message") {
    holder::ai::AiMessageRepo messages(db, fts);
    const auto message = messages.get(id);
    if (!message) {
      return support::error_response(http::status::not_found, "not_found", "AI message not found.");
    }
    const auto thread = holder::ai::AiThreadRepo(db).get(message->thread_id);
    if (!thread || (expected_project_id && thread->project_id != *expected_project_id)) {
      return support::error_response(http::status::not_found, "not_found", "AI message not found.");
    }
    if (!message->deleted_at) {
      return support::error_response(
          http::status::conflict,
          "not_trashed",
          "AI message is not in Trash."
      );
    }
    messages.remove_trashed(thread->project_id, id);
    return support::json_response(http::status::ok, {{"ok", true}, {"data", {{"message_id", id}}}});
  }
  return support::error_response(http::status::bad_request, "bad_request", "Unknown trash type.");
}

} // namespace

bool handle_trash_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts,
    const std::function<std::string(const std::string&)>& param_get
) {
  if (path == "/trash" && req.method() == http::verb::get) {
    const std::string project_id = param_get("project_id");
    const std::string type = param_get("type");
    if (project_id.empty()) {
      res =
          support::error_response(http::status::bad_request, "bad_request", "Missing project_id.");
    } else {
      try {
        nlohmann::json data = nlohmann::json::array();
        if (type.empty() || type == "card" || type == "all") {
          holder::card::CardRepo card_repo(db);
          const auto cards = card_repo.list_all(project_id);
          for (const auto& card : cards) {
            if (!card.deleted_at.has_value()) continue;
            nlohmann::json item;
            item["type"] = "card";
            item["card_id"] = card.card_id;
            item["project_id"] = card.project_id;
            item["title"] = card.title;
            item["deleted_at"] = card.deleted_at.value();
            item["rel_path"] = card.rel_path;
            data.push_back(std::move(item));
          }
        }
        if (type.empty() || type == "ai_message" || type == "all") {
          holder::ai::AiMessageRepo msg_repo(db, fts);
          const auto msgs = msg_repo.list_deleted_by_project(project_id);
          for (const auto& msg : msgs) {
            if (!msg.deleted_at.has_value()) continue;
            nlohmann::json item;
            item["type"] = "ai_message";
            item["message_id"] = msg.message_id;
            item["thread_id"] = msg.thread_id;
            item["project_id"] = project_id;
            item["role"] = msg.role;
            item["deleted_at"] = msg.deleted_at.value();
            item["created_at"] = msg.created_at;
            data.push_back(std::move(item));
          }
        }
        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = data;
        res = support::json_response(http::status::ok, payload);
      } catch (const std::exception& ex) {
        res = support::error_response(http::status::bad_request, "bad_request", ex.what());
      }
    }
    return true;
  }

  if (path == "/trash" && req.method() == http::verb::delete_) {
    const std::string project_id = param_get("project_id");
    const std::string type = param_get("type");
    if (project_id.empty()) {
      res =
          support::error_response(http::status::bad_request, "bad_request", "Missing project_id.");
    } else {
      try {
        if ((type.empty() || type == "card" || type == "all") && card_store) {
          holder::card::CardRepo card_repo(db);
          const auto cards = card_repo.list_all(project_id);
          for (const auto& card : cards) {
            if (!card.deleted_at.has_value()) continue;
            card_store->hard_delete(card.card_id, project_id);
          }
        }
        if (type.empty() || type == "ai_message" || type == "all") {
          holder::ai::AiMessageRepo msg_repo(db, fts);
          const auto msgs = msg_repo.list_deleted_by_project(project_id);
          for (const auto& msg : msgs) {
            msg_repo.remove_trashed(project_id, msg.message_id);
          }
        }
        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = {{"project_id", project_id}};
        res = support::json_response(http::status::ok, payload);
      } catch (const std::exception& ex) {
        res = support::error_response(http::status::bad_request, "bad_request", ex.what());
      }
    }
    return true;
  }

  std::string rest;
  auto expected_project_id = scoped_trash_item(path, rest);
  const bool legacy_item = path.rfind("/trash/", 0) == 0;
  if (legacy_item) {
    rest = path.substr(std::string("/trash/").size());
  }
  if ((legacy_item || expected_project_id) && req.method() == http::verb::delete_) {
    const auto slash = rest.find('/');
    if (slash == std::string::npos) {
      res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    } else {
      const std::string type = rest.substr(0, slash);
      const std::string id = rest.substr(slash + 1);
      if (id.empty()) {
        res = support::error_response(http::status::bad_request, "bad_request", "Missing id.");
      } else {
        try {
          res = delete_trashed_item(type, id, expected_project_id, db, card_store, fts);
        } catch (const std::exception& ex) {
          res = support::error_response(http::status::bad_request, "bad_request", ex.what());
        }
      }
    }
    return true;
  }

  return false;
}

} // namespace holder::api::routes
