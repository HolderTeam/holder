#include "api/routes/ProjectCardRoutes.h"
#include "api/support/HttpResponses.h"
#include "api/support/Time.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>

namespace holder::api::routes {
namespace {
namespace http = boost::beast::http;

http::response<http::string_body> privacy_error(const holder::privacy::PrivacyError& ex) {
  return support::error_response(
      ex.code() == holder::privacy::PrivacyErrorCode::KeyringUnavailable
          ? http::status::service_unavailable
          : http::status::bad_request,
      holder::privacy::privacy_error_code_name(ex.code()),
      ex.what()
  );
}

// POST /projects/{project_id}/repair-hierarchy {"apply": bool}: finds live cards that browsing
// from the roots cannot reach (their parent missing or trashed) and, with apply, moves each
// hidden branch to its nearest live, reachable ancestor in one commit. The plan is core's.
void repair_hierarchy_route(
    const std::string& project_id,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store
) {
  if (req.method() != http::verb::post) {
    res = support::error_response(
        http::status::method_not_allowed,
        "method_not_allowed",
        "Use POST."
    );
    return;
  }
  try {
    bool apply = false;
    if (!req.body().empty()) {
      const auto body = nlohmann::json::parse(req.body());
      if (!body.is_object()) throw std::invalid_argument("Body must be a JSON object.");
      if (body.contains("apply")) {
        if (!body.at("apply").is_boolean())
          throw std::invalid_argument("apply must be true or false.");
        apply = body.at("apply").get<bool>();
      }
    }
    if (!holder::project::ProjectRepo(db).get(project_id).has_value()) {
      res = support::error_response(http::status::not_found, "not_found", "Project not found.");
      return;
    }
    if (!card_store) {
      res = support::error_response(
          http::status::not_implemented,
          "not_implemented",
          "Card store unavailable."
      );
      return;
    }
    const auto plan = apply ? card_store->repair_hierarchy(project_id, support::now_epoch_seconds())
                            : card_store->plan_hierarchy_repair(project_id);
    auto moved = nlohmann::json::array();
    for (const auto& card_id : plan.moved_card_ids) {
      nlohmann::json parent = nullptr;
      for (const auto& card : plan.placements) {
        if (card.card_id == card_id && card.parent_card_id) parent = *card.parent_card_id;
      }
      moved.push_back({{"card_id", card_id}, {"parent_card_id", parent}});
    }
    res = support::json_response(
        http::status::ok,
        {{"ok", true},
         {"data",
          {{"applied", apply && !plan.placements.empty()},
           {"moved", std::move(moved)},
           {"changed_card_count", plan.placements.size()},
           {"cycle_card_ids", plan.cycle_card_ids}}}}
    );
  } catch (const holder::privacy::PrivacyError& ex) {
    res = privacy_error(ex);
  } catch (const std::invalid_argument& ex) {
    res = support::error_response(http::status::bad_request, "bad_request", ex.what());
  } catch (const nlohmann::json::exception&) {
    res = support::error_response(http::status::bad_request, "bad_request", "Invalid JSON body.");
  } catch (const std::exception& ex) {
    res = support::error_response(http::status::internal_server_error, "error", ex.what());
  }
}

} // namespace

bool handle_project_card_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store
) {
  constexpr std::string_view prefix = "/projects/";
  constexpr std::string_view repair_suffix = "/repair-hierarchy";
  if (path.starts_with(prefix) && path.ends_with(repair_suffix) &&
      path.size() > prefix.size() + repair_suffix.size()) {
    const auto project_id =
        path.substr(prefix.size(), path.size() - prefix.size() - repair_suffix.size());
    if (project_id.find('/') != std::string::npos) return false;
    repair_hierarchy_route(project_id, req, res, db, card_store);
    return true;
  }
  // The card page and breadcrumb GETs are generated; other methods on their paths end here.
  if (!path.starts_with(prefix)) return false;
  const auto rest = std::string_view(path).substr(prefix.size());
  const auto cards_at = rest.find("/cards");
  if (cards_at == 0 || cards_at == std::string_view::npos) return false;
  const auto after = rest.substr(cards_at + 6);
  constexpr std::string_view breadcrumbs_suffix = "/breadcrumbs";
  const bool page = after.empty();
  const bool breadcrumbs = after.size() > breadcrumbs_suffix.size() + 1 && after[0] == '/' &&
                           after.ends_with(breadcrumbs_suffix) &&
                           after.substr(1, after.size() - 1 - breadcrumbs_suffix.size())
                                   .find('/') == std::string_view::npos;
  if ((!page && !breadcrumbs) || rest.substr(0, cards_at).find('/') != std::string_view::npos)
    return false;
  res = support::error_response(http::status::method_not_allowed, "method_not_allowed", "Use GET.");
  return true;
}
} // namespace holder::api::routes
