// The contract's recovery-token operations (contract/interface/recovery.py).
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"
#include "capi/Context.h"
#include "git/GitOps.h"
#include "git/RepoSyncMetrics.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectManifest.h"
#include "project/ProjectPaths.h"
#include "project/ProjectRegistry.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"

#include <optional>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

holder::model::Project find_project(holder::platform::Db& db, const std::string& id) {
  auto project = holder::project::ProjectRepo(db).get(id);
  if (!project) throw NotFound("project not found: " + id);
  return std::move(*project);
}

template <class Body> decltype(auto) with_recovery_errors(Body&& body) {
  try {
    return with_contract_errors(std::forward<Body>(body));
  } catch (const holder::privacy::PrivacyError& error) {
    using Code = holder::privacy::PrivacyErrorCode;
    if (error.code() == Code::RecoveryTokenInvalid)
      throw RecoveryTokenInvalid("invalid recovery token or PIN");
    if (error.code() == Code::PrivacyCryptoFailed)
      throw DataCorrupted("recovery token cryptography failed");
    throw;
  }
}

RecoveryTokenGlobalImportResult import_global(
    holder_context& context,
    const RecoveryTokenImportRequest& request
) {
  const auto metadata =
      holder::privacy::inspect_recovery_token(request.pin, request.recovery_token);
  holder::project::ProjectRepo repo(context.db);
  holder::project::ProjectSyncRepo sync_repo(context.db);
  const auto now = now_epoch_seconds();
  auto project = repo.get(metadata.project_id);
  const bool created = !project;
  if (created) {
    holder::model::Project recovered;
    recovered.project_id = metadata.project_id;
    recovered.name = metadata.project_name && !metadata.project_name->empty()
                         ? *metadata.project_name
                         : "Recovered Project";
    recovered.privacy_mode = "encrypted_git";
    recovered.created_at = now;
    recovered.updated_at = now;
    recovered.root_path = holder::core::unique_project_root(
        context.projects_root,
        holder::core::slugify(recovered.name),
        repo.list()
    );
    project = std::move(recovered);
  }

  std::optional<holder::git::RealGitOps> own_git;
  auto& git = context.git ? *context.git : own_git.emplace();
  auto operation = git.lock_operation(project->root_path);
  if (created) repo.create(*project);
  holder::privacy::import_recovery_token(
      repo,
      metadata.project_id,
      request.pin,
      request.recovery_token,
      now
  );

  const bool has_remote = metadata.git_remote_url && !metadata.git_remote_url->empty();
  bool configured = false;
  auto pull_status = RecoveryTokenGlobalImportResultPullStatus::NotAttempted;
  std::optional<std::string> remote_error;
  std::optional<std::string> pull_error;
  if (has_remote) {
    const auto refreshed = find_project(context.db, metadata.project_id);
    git.open_or_init(refreshed.root_path);
    try {
      git.set_remote("origin", *metadata.git_remote_url);
      configured = true;
    } catch (const std::exception& error) {
      remote_error = error.what();
    }
    if (configured) {
      try {
        git.pull_remote_ff_only("origin");
        pull_status = RecoveryTokenGlobalImportResultPullStatus::Succeeded;
        sync_repo.record_pull_result(metadata.project_id, "succeeded", true, std::nullopt, now);
      } catch (const std::exception& error) {
        pull_status = RecoveryTokenGlobalImportResultPullStatus::Failed;
        pull_error = error.what();
        sync_repo.record_pull_result(metadata.project_id, "failed", false, pull_error, now);
      }
    }
    try {
      const auto metrics = holder::git::inspect_repo_sync_metrics(refreshed.root_path, "origin");
      sync_repo.update_activity_counts(
          metadata.project_id,
          {.uncommitted_changes_count = metrics.uncommitted_changes_count,
           .unpushed_commits_count = metrics.unpushed_commits_count,
           .updated_at = now}
      );
    } catch (const std::exception&) {
      // Metrics are best effort; the token has already restored the key.
    }
  }

  const auto refreshed = find_project(context.db, metadata.project_id);
  holder::project::write_project_manifest(git, refreshed);
  git.commit("Restore encrypted project metadata");
  if (!context.registry_path.empty())
    holder::core::ProjectRegistry(context.registry_path).restore(refreshed);
  return {
      {metadata.project_id},
      created,
      has_remote,
      configured,
      remote_error,
      pull_status,
      pull_error
  };
}

} // namespace

ProjectRecoveryTokenExportResult export_project_recovery_token(
    holder_context& context,
    std::string project_id,
    const RecoveryTokenExportRequest& request
) {
  return with_recovery_errors([&] {
    const auto project = find_project(context.db, project_id);
    if (!project.project_key_id) throw InvalidArgument("project has no key material configured");
    auto token = holder::privacy::export_recovery_token(
        project_id,
        *project.project_key_id,
        request.pin,
        project.name,
        project.git_remote_url
    );
    return ProjectRecoveryTokenExportResult{
        std::move(project_id),
        *project.project_key_id,
        std::move(token)
    };
  });
}

ProjectRecoveryTokenImportResult import_project_recovery_token(
    holder_context& context,
    std::string project_id,
    const RecoveryTokenImportRequest& request
) {
  return with_recovery_errors([&] {
    find_project(context.db, project_id);
    holder::project::ProjectRepo repo(context.db);
    holder::privacy::import_recovery_token(
        repo,
        project_id,
        request.pin,
        request.recovery_token,
        now_epoch_seconds()
    );
    return ProjectRecoveryTokenImportResult{std::move(project_id)};
  });
}

RecoveryTokenGlobalImportResult import_recovery_token_global(
    holder_context& context,
    const RecoveryTokenImportRequest& request
) {
  return with_recovery_errors([&] {
    return import_global(context, request);
  });
}

} // namespace holder::contract
