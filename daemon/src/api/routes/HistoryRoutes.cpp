#include "api/routes/HistoryRoutes.h"

#include "api/History.h"

#include "api/support/HttpResponses.h"
#include "api/support/Time.h"

#include "card/CardStore.h"
#include "git/GitRepo.h"
#include "git/RepoLocks.h"
#include "git/RevisionReferenceResolver.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

struct HistoryPath {
  enum class Scope { Project, Card };
  std::string project_id;
  std::string card_id;
  Scope scope = Scope::Card;
  bool compare = false;
  bool restore = false;
  bool snapshot = false;
};

std::optional<HistoryPath> parse_history_path(const std::string& path) {
  static const std::string projects = "/projects/";
  static const std::string project_history = "/history";
  static const std::string card_history = "/history/cards/";
  if (path.rfind(projects, 0) != 0) return std::nullopt;
  const auto project_end = path.find('/', projects.size());
  if (project_end == std::string::npos) return std::nullopt;
  HistoryPath parsed;
  parsed.project_id = path.substr(projects.size(), project_end - projects.size());
  if (parsed.project_id.empty()) return std::nullopt;
  if (path.substr(project_end) == project_history) {
    parsed.scope = HistoryPath::Scope::Project;
    return parsed;
  }
  if (path.compare(project_end, card_history.size(), card_history) != 0) return std::nullopt;
  const auto card_start = project_end + card_history.size();
  const auto suffix = path.find('/', card_start);
  parsed.card_id = path.substr(card_start, suffix - card_start);
  if (parsed.card_id.empty()) return std::nullopt;
  if (suffix == std::string::npos) return parsed;
  if (path.substr(suffix) == "/compare")
    parsed.compare = true;
  else if (path.substr(suffix) == "/restore")
    parsed.restore = true;
  else if (path.substr(suffix) == "/snapshot")
    parsed.snapshot = true;
  else
    return std::nullopt;
  return parsed;
}

bool map_revision_reference_result(
    const holder::git::RevisionReferenceResult& result,
    const std::string& reference,
    const std::string& parameter_name,
    http::response<http::string_body>& res
) {
  switch (result.status) {
  case holder::git::RevisionReferenceStatus::Resolved:
    return true;
  case holder::git::RevisionReferenceStatus::Ambiguous:
    res = support::error_response(
        http::status::conflict,
        "revision_ambiguous",
        parameter_name + " revision reference is ambiguous: " + reference
    );
    return false;
  case holder::git::RevisionReferenceStatus::NotFound:
    res = support::error_response(
        http::status::not_found,
        "revision_not_found",
        parameter_name + " revision was not found: " + reference
    );
    return false;
  }
  throw std::runtime_error("unknown revision reference status"); // LCOV_EXCL_LINE
}

bool resolve_revision_reference(
    const holder::model::Project& project,
    const std::string& reference,
    const std::string& parameter_name,
    http::response<http::string_body>& res,
    std::string& resolved_oid
) {
  if (reference.empty()) {
    res = support::error_response(
        http::status::bad_request,
        "bad_request",
        parameter_name + " revision reference is required."
    );
    return false;
  }

  holder::git::GitRepo repo;
  repo.open_existing(project.root_path);
  const auto result = holder::git::RevisionReferenceResolver(repo).resolve(reference);
  if (!map_revision_reference_result(result, reference, parameter_name, res)) return false;
  resolved_oid = result.oid.value();
  return true;
}

} // namespace

bool handle_history_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    const std::function<std::string(const std::string&)>& param_get,
    holder::card::CardStore* card_store
) {
  const auto parsed = parse_history_path(path);
  if (!parsed.has_value()) return false;
  // The generated routes serve every history read.
  if (!parsed->restore && req.method() == http::verb::get) return false;

  try {
    const auto project = holder::project::ProjectRepo(db).get(parsed->project_id);
    if (!project.has_value()) {
      res = support::error_response(http::status::not_found, "not_found", "Project not found.");
      return true;
    }

    if (parsed->restore) {
      if (req.method() != http::verb::post) {
        res = support::error_response(
            http::status::method_not_allowed,
            "method_not_allowed",
            "Method not allowed."
        );
        return true;
      }
      if (card_store == nullptr) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
        return true;
      }
      const auto repo_mutex = holder::git::repo_mutex_for(project->root_path);
      const std::lock_guard<std::recursive_mutex> repo_guard(*repo_mutex);
      std::string oid;
      if (!resolve_revision_reference(*project, param_get("oid"), "oid", res, oid)) return true;
      const auto card = card_store->get(parsed->card_id);
      if (!card.has_value() || card->project_id != project->project_id) {
        res = support::error_response(http::status::not_found, "not_found", "Card not found.");
        return true;
      }
      card_store->restore_version(parsed->card_id, oid, support::now_epoch_seconds());
      const auto restored = card_store->get(parsed->card_id);
      if (!restored.has_value()) {
        throw std::runtime_error("restored card is missing"); // LCOV_EXCL_LINE
      }
      holder::git::GitRepo repo;
      repo.open_existing(project->root_path);
      const auto result_oid = repo.head_oid();
      if (!result_oid.has_value()) {
        throw std::runtime_error("restored revision is missing"); // LCOV_EXCL_LINE
      }
      res = support::json_response(
          http::status::ok,
          {{"ok", true},
           {"data",
            {{"card_id", parsed->card_id},
             {"restored_from_oid", oid},
             {"result_oid", *result_oid},
             {"title", restored->title},
             {"updated_at", restored->updated_at},
             {"deleted_at",
              restored->deleted_at.has_value() ? nlohmann::json(*restored->deleted_at)
                                               : nlohmann::json(nullptr)}}}}
      );
      return true;
    }
    res = support::error_response(
        http::status::method_not_allowed,
        "method_not_allowed",
        "Method not allowed."
    );
  } catch (const std::invalid_argument& ex) {
    res = support::error_response(http::status::bad_request, "bad_request", ex.what());
  } catch (const holder::privacy::PrivacyError& ex) {
    res = history_privacy_error_response(ex);
  } catch (const std::exception& ex) {
    res = support::error_response(
        http::status::service_unavailable,
        "history_unavailable",
        ex.what()
    );
  }
  return true;
}

http::response<http::string_body> history_privacy_error_response(
    const holder::privacy::PrivacyError& error
) {
  if (holder::api::history_key_unavailable(error)) {
    return support::error_response(http::status::conflict, "history_key_unavailable", error.what());
  }
  return support::error_response(
      http::status::service_unavailable,
      "history_unavailable",
      error.what()
  );
}

} // namespace holder::api::routes
