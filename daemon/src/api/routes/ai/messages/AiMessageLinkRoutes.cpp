#include "api/routes/ai/messages/AiMessageLinkRoutes.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "api/support/HttpResponses.h"
#include "api/support/Time.h"
#include "card/LinkMutation.h"
#include "card/LinkRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace holder::api::routes::ai::messages {
namespace {

namespace http = boost::beast::http;

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

} // namespace

bool handle_ai_message_link_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts
) {
  if (path.rfind("/ai/messages/", 0) != 0) {
    return false;
  }
  const std::string rest = path.substr(std::string("/ai/messages/").size());
  const auto slash = rest.find('/');
  if (slash == std::string::npos) {
    return false;
  }

  const std::string message_id = rest.substr(0, slash);
  const std::string tail = rest.substr(slash);
  if (message_id.empty()) {
    res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    return true;
  }

  if (tail == "/links") {
    try {
      holder::ai::AiMessageRepo message_repo(db, fts);
      const auto msg_opt = message_repo.get(message_id);
      if (!msg_opt.has_value()) {
        res =
            support::error_response(http::status::not_found, "not_found", "AI message not found.");
        return true;
      }

      holder::ai::AiThreadRepo thread_repo(db);
      const auto thread_opt = thread_repo.get(msg_opt->thread_id);
      if (!thread_opt.has_value()) {
        res =
            support::error_response(http::status::bad_request, "bad_request", "Thread not found.");
        return true;
      }

      const std::string project_id = thread_opt->project_id;
      holder::card::LinkRepo link_repo(db);
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
          link.project_id = project_id;
          link.from_card_id = message_id;
          link.to_card_id = body.at("to_card_id").get<std::string>();
          if (body.contains("to_type") && !body.at("to_type").is_null()) {
            link.to_type = body.at("to_type").get<std::string>();
          }
          if (link.to_type.empty()) {
            link.to_type = "card";
          }
          if (body.contains("kind") && !body.at("kind").is_null()) {
            link.kind = body.at("kind").get<std::string>();
          }
          if (link.kind.empty()) {
            link.kind = "ref";
          }
          if (body.contains("label") && !body.at("label").is_null()) {
            link.label = body.at("label").get<std::string>();
          }
          if (body.contains("target_project_id") && !body.at("target_project_id").is_null()) {
            link.target_project_id = body.at("target_project_id").get<std::string>();
            if (link.target_project_id == project_id) link.target_project_id.reset();
          }
          if (body.contains("created_at") && !body.at("created_at").is_null()) {
            link.created_at = body.at("created_at").get<long long>();
          } else {
            link.created_at = support::now_epoch_seconds();
          }
          std::string validation_error;
          if (!validate_link_target(
                  db,
                  project_id,
                  link.to_card_id,
                  link.to_type,
                  link.target_project_id,
                  validation_error
              )) {
            res =
                support::error_response(http::status::bad_request, "bad_request", validation_error);
          } else {
            link_repo.upsert_links(project_id, message_id, {link});
            message_repo.update_links(message_id);

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
        if (target_project_id.has_value()) {
          if (target_project_id->empty())
            throw std::invalid_argument("target_project_id must not be empty.");
          link_repo.delete_link_exact(
              project_id, message_id, *to_card_id, *to_type, *kind, *target_project_id
          );
        } else if (to_card_id.has_value() && to_type.has_value() && kind.has_value()) {
          std::optional<std::string> matched_project_id;
          for (const auto& link : link_repo.list_outgoing(project_id, message_id)) {
            if (link.to_card_id != *to_card_id || link.to_type != *to_type ||
                link.kind != *kind) continue;
            if (matched_project_id.has_value())
              throw std::invalid_argument("target_project_id is required to disambiguate links.");
            matched_project_id = link.target_project_id.value_or(project_id);
          }
          if (matched_project_id.has_value())
            link_repo.delete_link_exact(
                project_id, message_id, *to_card_id, *to_type, *kind, *matched_project_id
            );
        } else if (to_card_id.has_value()) {
          link_repo.delete_link(project_id, message_id, to_card_id.value(), to_type, kind);
        } else {
          link_repo.delete_links_from(project_id, message_id);
        }
        message_repo.update_links(message_id);

        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = {{"message_id", message_id}};
        res = support::json_response(http::status::ok, payload);
      } else {
        res = support::error_response(
            http::status::method_not_allowed,
            "method_not_allowed",
            "Method not allowed."
        );
      }
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    }
    return true;
  }

  // GET is generated; any other method on backlinks is not allowed.
  if (tail == "/backlinks") {
    res = support::error_response(
        http::status::method_not_allowed,
        "method_not_allowed",
        "Method not allowed."
    );
    return true;
  }

  return false;
}

} // namespace holder::api::routes::ai::messages
