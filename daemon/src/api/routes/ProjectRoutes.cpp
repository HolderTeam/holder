#include "api/routes/ProjectRoutes.h"
#include "api/ProjectTags.h"
#include "api/support/HttpResponses.h"
#include "api/support/Time.h"

#include "card/TagRepo.h"

#include "card/CardRepo.h"
#include "git/RepoSyncMetrics.h"
#include "index/FtsIndexer.h"
#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectManifest.h"
#include "project/ProjectPaths.h"
#include "project/ProjectRepo.h"
#include "project/ProjectStore.h"
#include "project/ProjectSyncRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace holder::api {

ProjectTagListResponse list_project_tags(holder::platform::Db& db, std::string project_id) {
  holder::project::ProjectRepo project_repo(db);
  if (!project_repo.get(project_id).has_value()) throw ProjectNotFound{};

  holder::card::TagRepo tag_repo(db);
  ProjectTagListResponse response{true, {}};
  for (const auto& [tag, card_count] : tag_repo.list_project_tags(project_id)) {
    response.data.push_back({tag, card_count});
  }
  return response;
}

} // namespace holder::api

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

holder::git::GitOps& resolve_git(holder::git::GitOps* git) {
  static holder::git::RealGitOps real_git;
  return git ? *git : real_git;
}

// LCOV_EXCL_START
http::response<http::string_body> privacy_error_response(const holder::privacy::PrivacyError& ex) {
  http::status status = http::status::bad_request;
  switch (ex.code()) {
  case holder::privacy::PrivacyErrorCode::KeyringUnavailable:
    status = http::status::service_unavailable;
    break;
  case holder::privacy::PrivacyErrorCode::KeyMaterialMissing:
  case holder::privacy::PrivacyErrorCode::CryptMetadataMissing:
    status = http::status::internal_server_error;
    break;
  case holder::privacy::PrivacyErrorCode::RecoveryTokenInvalid:
  case holder::privacy::PrivacyErrorCode::EnvelopeInvalid:
  case holder::privacy::PrivacyErrorCode::EnvelopeMetadataMismatch:
  case holder::privacy::PrivacyErrorCode::EncryptionSafetyCheckFailed:
  case holder::privacy::PrivacyErrorCode::PrivacyCryptoFailed:
    status = http::status::bad_request;
    break;
  }
  return support::error_response(
      status,
      holder::privacy::privacy_error_code_name(ex.code()),
      ex.what()
  );
}
// LCOV_EXCL_STOP

} // namespace

bool handle_project_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::git::GitOps* git_ops
) {
  if (path == "/recovery-token/import" && req.method() == http::verb::post) {
    try {
      const auto body = nlohmann::json::parse(req.body());
      if (!body.contains("pin") || body.at("pin").is_null() || !body.contains("recovery_token") ||
          body.at("recovery_token").is_null()) {
        res = support::error_response(
            http::status::bad_request,
            "bad_request",
            "Missing required fields."
        );
        return true;
      }

      const std::string pin = body.at("pin").get<std::string>();
      const std::string recovery_token = body.at("recovery_token").get<std::string>();
      const auto metadata = holder::privacy::inspect_recovery_token(pin, recovery_token);

      holder::project::ProjectRepo repo(db);
      holder::project::ProjectSyncRepo sync_repo(db);
      const long long now = support::now_epoch_seconds();
      bool project_created = false;
      auto project_opt = repo.get(metadata.project_id);
      if (!project_opt.has_value()) {
        holder::model::Project project;
        project.project_id = metadata.project_id;
        project.name = metadata.project_name.has_value() && !metadata.project_name->empty()
                           ? metadata.project_name.value()
                           : "Recovered Project";
        project.privacy_mode = "encrypted_git";
        project.created_at = now;
        project.updated_at = now;
        const auto base_root = holder::core::default_projects_root();
        const auto slug = holder::core::slugify(project.name);
        project.root_path = holder::core::unique_project_root(base_root, slug, repo.list());
        project_created = true;
        project_opt = project;
      }

      auto& git = resolve_git(git_ops);
      auto operation = git.lock_operation(project_opt->root_path);
      if (project_created) repo.create(*project_opt);
      holder::privacy::import_recovery_token(repo, metadata.project_id, pin, recovery_token, now);

      const bool remote_hint_present = metadata.git_remote_url.has_value() &&
                                       !metadata.git_remote_url->empty();
      bool remote_configured = false;
      std::string pull_status = "not_attempted";
      std::string remote_error;
      std::string pull_error;
      if (remote_hint_present) {
        const auto refreshed = repo.get(metadata.project_id);
        if (refreshed.has_value()) {
          git.open_or_init(refreshed->root_path);
          try {
            git.set_remote("origin", metadata.git_remote_url.value());
            remote_configured = true;
          } catch (const std::exception& ex) {
            remote_error = ex.what();
          }

          if (remote_configured) {
            try {
              git.pull_remote_ff_only("origin");
              pull_status = "succeeded";
              sync_repo
                  .record_pull_result(metadata.project_id, pull_status, true, std::nullopt, now);
            } catch (const std::exception& ex) {
              pull_status = "failed";
              pull_error = ex.what();
              sync_repo
                  .record_pull_result(metadata.project_id, pull_status, false, pull_error, now);
            }
          }
          try {
            const auto metrics =
                holder::git::inspect_repo_sync_metrics(refreshed->root_path, "origin");
            sync_repo.update_activity_counts(
                metadata.project_id,
                {.uncommitted_changes_count = metrics.uncommitted_changes_count,
                 .unpushed_commits_count = metrics.unpushed_commits_count,
                 .updated_at = now}
            );
            // Best-effort only; metrics refresh failure does not fail import.
            // LCOV_EXCL_START
          } catch (const std::exception& ex) {
            (void)ex;
            // Best-effort only.
          }
          // LCOV_EXCL_STOP
        }
      }

      if (const auto refreshed = repo.get(metadata.project_id); refreshed.has_value()) {
        holder::project::write_project_manifest(git, *refreshed);
        git.commit("Restore encrypted project metadata");
        holder::core::ProjectRegistry(holder::core::Paths::resolve("holder").project_registry_path()
        )
            .restore(*refreshed);
      }

      nlohmann::json payload;
      payload["ok"] = true;
      payload["data"] = {
          {"project_id", metadata.project_id},
          {"project_created", project_created},
          {"remote_hint_present", remote_hint_present},
          {"remote_configured", remote_configured},
          {"remote_error",
           remote_error.empty() ? nlohmann::json(nullptr) : nlohmann::json(remote_error)},
          {"pull_status", pull_status},
          {"pull_error", pull_error.empty() ? nlohmann::json(nullptr) : nlohmann::json(pull_error)},
      };
      res = support::json_response(
          project_created ? http::status::created : http::status::ok,
          payload
      );
    } catch (const holder::privacy::PrivacyError& ex) {
      res = privacy_error_response(ex); // LCOV_EXCL_LINE
    } catch (const std::exception& ex) {
      res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    }
    return true;
  }

  if (path.rfind("/projects/", 0) == 0) {
    const std::string suffix = path.substr(std::string("/projects/").size());
    const auto slash_pos = suffix.find('/');
    const std::string project_id = slash_pos == std::string::npos ? suffix
                                                                  : suffix.substr(0, slash_pos);
    const std::string subpath = slash_pos == std::string::npos ? "" : suffix.substr(slash_pos);
    if (project_id.empty()) {
      res = support::error_response(http::status::not_found, "not_found", "Route not found.");
      return true;
    }
    if (subpath == "/recovery-token/export" && req.method() == http::verb::post) {
      try {
        const auto body = nlohmann::json::parse(req.body());
        if (!body.contains("pin") || body.at("pin").is_null()) {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "Missing required fields."
          );
          return true;
        }
        holder::project::ProjectRepo repo(db);
        const auto project_opt = repo.get(project_id);
        if (!project_opt.has_value()) {
          res = support::error_response(http::status::not_found, "not_found", "Project not found.");
          return true;
        }
        if (!project_opt->project_key_id.has_value()) {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "Project has no key material configured."
          );
          return true;
        }
        const std::string token = holder::privacy::export_recovery_token(
            project_id,
            project_opt->project_key_id.value(),
            body.at("pin").get<std::string>(),
            project_opt->name,
            project_opt->git_remote_url
        );

        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = {
            {"project_id", project_id},
            {"key_id", project_opt->project_key_id.value()},
            {"recovery_token", token},
        };
        res = support::json_response(http::status::ok, payload);
      } catch (const holder::privacy::PrivacyError& ex) {
        res = privacy_error_response(ex); // LCOV_EXCL_LINE
      } catch (const std::exception& ex) {
        res = support::error_response(http::status::bad_request, "bad_request", ex.what());
      }
    } else if (subpath == "/recovery-token/import" && req.method() == http::verb::post) {
      try {
        const auto body = nlohmann::json::parse(req.body());
        if (!body.contains("pin") || body.at("pin").is_null() || !body.contains("recovery_token") ||
            body.at("recovery_token").is_null()) {
          res = support::error_response(
              http::status::bad_request,
              "bad_request",
              "Missing required fields."
          );
          return true;
        }
        holder::project::ProjectRepo repo(db);
        const auto project_opt = repo.get(project_id);
        if (!project_opt.has_value()) {
          res = support::error_response(http::status::not_found, "not_found", "Project not found.");
          return true;
        }

        holder::privacy::import_recovery_token(
            repo,
            project_id,
            body.at("pin").get<std::string>(),
            body.at("recovery_token").get<std::string>(),
            support::now_epoch_seconds()
        );

        nlohmann::json payload;
        payload["ok"] = true;
        payload["data"] = {{"project_id", project_id}};
        res = support::json_response(http::status::ok, payload);
      } catch (const holder::privacy::PrivacyError& ex) {
        res = privacy_error_response(ex); // LCOV_EXCL_LINE
      } catch (const std::exception& ex) {
        res = support::error_response(http::status::bad_request, "bad_request", ex.what());
      }
    } else if (subpath.empty() &&
               (req.method() == http::verb::get || req.method() == http::verb::delete_ ||
                req.method() == http::verb::patch)) {
      return false;
    } else {
      res = support::error_response(http::status::not_found, "not_found", "Route not found.");
    }
    return true;
  }

  return false;
}

} // namespace holder::api::routes
