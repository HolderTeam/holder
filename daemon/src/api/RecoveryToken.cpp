#include "api/RecoveryToken.h"

#include "api/support/Time.h"
#include "git/GitOps.h"
#include "git/RepoSyncMetrics.h"
#include "platform/Paths.h"
#include "platform/ProjectRegistry.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectManifest.h"
#include "project/ProjectPaths.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"

#include <utility>

namespace holder::api {
namespace {

using OptionalText = std::optional<std::optional<std::string>>;

holder::git::GitOps& resolve_git(holder::git::GitOps* git) {
  static holder::git::RealGitOps real_git;
  return git ? *git : real_git;
}

holder::model::Project find_project(holder::project::ProjectRepo& repo,
                                    const std::string& project_id) {
  auto project = repo.get(project_id);
  if (!project) throw RecoveryTokenProjectNotFound{};
  return std::move(*project);
}

// A new encrypted project for a token whose project this Home does not have.
holder::model::Project recovered_project(holder::project::ProjectRepo& repo,
                                         const holder::privacy::RecoveryTokenMetadata& metadata,
                                         long long now) {
  holder::model::Project project;
  project.project_id = metadata.project_id;
  project.name = metadata.project_name && !metadata.project_name->empty() ? *metadata.project_name
                                                                          : "Recovered Project";
  project.privacy_mode = "encrypted_git";
  project.created_at = now;
  project.updated_at = now;
  project.root_path = holder::core::unique_project_root(
      holder::core::default_projects_root(), holder::core::slugify(project.name), repo.list());
  return project;
}

// An empty message is reported as null.
OptionalText text(const std::string& value) {
  if (value.empty()) return OptionalText(std::in_place, std::nullopt);
  return OptionalText(std::in_place, value);
}

} // namespace

RecoveryTokenImportAutoResponse post_recovery_token_import(holder::platform::Db& db,
                                                           holder::git::GitOps* git_ops,
                                                           ProjectRecoveryTokenImportRequest request) {
  try {
    const auto metadata = holder::privacy::inspect_recovery_token(request.pin,
                                                                  request.recovery_token);
    holder::project::ProjectRepo repo(db);
    holder::project::ProjectSyncRepo sync_repo(db);
    const long long now = support::now_epoch_seconds();
    auto project = repo.get(metadata.project_id);
    const bool project_created = !project;
    if (project_created) project = recovered_project(repo, metadata, now);

    auto& git = resolve_git(git_ops);
    auto operation = git.lock_operation(project->root_path);
    if (project_created) repo.create(*project);
    holder::privacy::import_recovery_token(repo, metadata.project_id, request.pin,
                                           request.recovery_token, now);

    // A remote named by the token is configured and pulled; failures are reported, not thrown.
    const bool remote_hint_present = metadata.git_remote_url && !metadata.git_remote_url->empty();
    bool remote_configured = false;
    std::string pull_status = "not_attempted";
    std::string remote_error;
    std::string pull_error;
    if (remote_hint_present) {
      const auto refreshed = find_project(repo, metadata.project_id);
      git.open_or_init(refreshed.root_path);
      try {
        git.set_remote("origin", *metadata.git_remote_url);
        remote_configured = true;
      } catch (const std::exception& error) {
        remote_error = error.what();
      }
      if (remote_configured) {
        try {
          git.pull_remote_ff_only("origin");
          pull_status = "succeeded";
          sync_repo.record_pull_result(metadata.project_id, pull_status, true, std::nullopt, now);
        } catch (const std::exception& error) {
          pull_status = "failed";
          pull_error = error.what();
          sync_repo.record_pull_result(metadata.project_id, pull_status, false, pull_error, now);
        }
      }
      // Best effort: a metrics refresh failure does not fail the import.
      try {
        const auto metrics = holder::git::inspect_repo_sync_metrics(refreshed.root_path, "origin");
        sync_repo.update_activity_counts(
            metadata.project_id,
            {.uncommitted_changes_count = metrics.uncommitted_changes_count,
             .unpushed_commits_count = metrics.unpushed_commits_count,
             .updated_at = now});
      } catch (const std::exception&) { // LCOV_EXCL_LINE
      }
    }

    const auto refreshed = find_project(repo, metadata.project_id);
    holder::project::write_project_manifest(git, refreshed);
    git.commit("Restore encrypted project metadata");
    holder::core::ProjectRegistry(holder::core::Paths::resolve("holder").project_registry_path())
        .restore(refreshed);

    return {true,
            {metadata.project_id, project_created, remote_hint_present, remote_configured,
             text(remote_error), std::move(pull_status), text(pull_error)}};
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
}

ProjectRecoveryTokenExportResponse post_projects_project_id_recovery_token_export(
    holder::platform::Db& db,
    std::string project_id,
    ProjectRecoveryTokenExportRequest request) {
  holder::project::ProjectRepo repo(db);
  const auto project = find_project(repo, project_id);
  if (!project.project_key_id)
    throw std::invalid_argument("Project has no key material configured.");
  try {
    auto token = holder::privacy::export_recovery_token(project_id, *project.project_key_id,
                                                        request.pin, project.name,
                                                        project.git_remote_url);
    return {true, {std::move(project_id), *project.project_key_id, std::move(token)}};
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
}

ProjectRecoveryTokenImportResponse post_projects_project_id_recovery_token_import(
    holder::platform::Db& db,
    std::string project_id,
    ProjectRecoveryTokenImportRequest request) {
  holder::project::ProjectRepo repo(db);
  find_project(repo, project_id);
  try {
    holder::privacy::import_recovery_token(repo, project_id, request.pin, request.recovery_token,
                                           support::now_epoch_seconds());
  } catch (const holder::privacy::PrivacyError& error) {
    support::throw_privacy_failure(error);
  }
  return {true, {std::move(project_id)}};
}

} // namespace holder::api
