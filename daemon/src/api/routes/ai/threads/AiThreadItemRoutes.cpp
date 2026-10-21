#include "api/routes/ai/threads/AiThreadItemRoutes.h"

#include "ai/AiThreadDurability.h"
#include "ai/AiThreadRepo.h"
#include "api/support/HttpResponses.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace holder::api::routes::ai::threads {
namespace {

namespace http = boost::beast::http;

} // namespace

bool handle_ai_thread_item_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db
) {
  if (path.rfind("/ai/threads/", 0) != 0) {
    return false;
  }

  const std::string thread_id = path.substr(std::string("/ai/threads/").size());
  if (thread_id.empty()) {
    res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    return true;
  }

  if (req.method() == http::verb::patch) {
    try {
      const auto body = nlohmann::json::parse(req.body());
      if (!body.contains("updated_at")) {
        res = support::error_response(
            http::status::bad_request,
            "bad_request",
            "Missing updated_at."
        );
      } else {
        std::optional<std::string> title;
        if (body.contains("title") && !body.at("title").is_null()) {
          title = body.at("title").get<std::string>();
        }
        std::optional<std::string> card_id;
        if (body.contains("card_id") && !body.at("card_id").is_null()) {
          card_id = body.at("card_id").get<std::string>();
        }
        const long long updated_at = body.at("updated_at").get<long long>();

        holder::ai::AiThreadRepo repo(db);
        if (title.has_value()) {
          repo.update_title(thread_id, title.value(), updated_at);
        } else {
          repo.touch_updated(thread_id, updated_at);
        }
        if (body.contains("card_id")) {
          repo.update_card_id(thread_id, card_id);
        }
        holder::ai::persist_ai_thread(db, repo.get(thread_id).value());

        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = {{"thread_id", thread_id}};
        res = support::json_response(http::status::ok, payload);
      }
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    }
    return true;
  }

  if (req.method() == http::verb::delete_) {
    try {
      holder::ai::AiThreadRepo repo(db);
      const auto thread = repo.get(thread_id);
      if (!thread.has_value()) {
        res = support::error_response(http::status::not_found, "not_found", "AI thread not found.");
        return true;
      }
      holder::ai::remove_ai_thread_manifest(db, *thread);
      repo.remove(thread_id);
      nlohmann::json payload;
      payload["ok"] = true;
      payload["data"] = {{"thread_id", thread_id}};
      res = support::json_response(http::status::ok, payload);
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    }
    return true;
  }

  res = support::error_response(http::status::not_found, "not_found", "Route not found.");
  return true;
}

} // namespace holder::api::routes::ai::threads
