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
#include "card/LinkVisibility.h"
#include "identity/Uuid.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

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
    if (include_deleted.value_or(0) != 1 && !holder::card::link_target_visible(db, link)) {
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
                 : include_deleted || holder::card::backlink_source_visible(db, link);
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
