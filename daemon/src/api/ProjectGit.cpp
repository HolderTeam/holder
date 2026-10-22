#include "api/ProjectGit.h"

#include "api/support/Time.h"
#include "git/GitOps.h"
#include "git/RepoSyncMetrics.h"
#include "index/FtsIndexer.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"
#include "sync/ProjectSyncOperation.h"

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
  if (!project) throw ProjectGitNotFound{};
  return std::move(*project);
}

// An absent or empty value is reported as null.
OptionalText text(const std::optional<std::string>& value) {
  if (!value || value->empty()) return OptionalText(std::in_place, std::nullopt);
  return OptionalText(std::in_place, *value);
}

std::string branch_name(const std::string& branch) {
  return branch.empty() ? "local_default" : branch;
}

bool pushed(holder::git::PushStatus status) {
  return status == holder::git::PushStatus::Pushed || status == holder::git::PushStatus::UpToDate;
}

std::optional<std::string> next_action(holder::git::PushStatus status) {
  switch (status) {
  case holder::git::PushStatus::NonFastForward: return "pull_then_retry";
  case holder::git::PushStatus::AuthFailed: return "fix_auth_then_retry";
  case holder::git::PushStatus::NotFound: return "fix_remote_url_then_retry";
  case holder::git::PushStatus::NetworkError:
  case holder::git::PushStatus::UnknownError: return "retry";
  default: return std::nullopt;
  }
}

ProjectGitSyncOperationResponseDataPushStatus sync_push_status(holder::git::PushStatus status) {
  using Status = ProjectGitSyncOperationResponseDataPushStatus;
  switch (status) {
  case holder::git::PushStatus::Pushed: return Status::pushed;
  case holder::git::PushStatus::UpToDate: return Status::up_to_date;
  case holder::git::PushStatus::AuthFailed: return Status::auth_failed;
  case holder::git::PushStatus::NotFound: return Status::not_found;
  case holder::git::PushStatus::NetworkError: return Status::network_error;
  case holder::git::PushStatus::NonFastForward: return Status::non_fast_forward;
  case holder::git::PushStatus::RemoteUnset: return Status::remote_unset;
  case holder::git::PushStatus::UnknownError: break;
  }
  return Status::unknown_error;
}

ProjectGitPushResponse push_response(const std::string& project_id,
                                     const std::optional<std::string>& remote_url,
                                     const std::string& branch,
                                     const holder::git::PushResult& push) {
  const bool ok = pushed(push.status);
  return {true,
          {project_id, OptionalText(std::in_place, remote_url), branch_name(branch),
           holder::git::push_status_name(push.status), push.ahead_count, push.behind_count,
           text(push.local_head_commit),
           text(ok ? std::nullopt
                   : std::optional<std::string>(holder::git::push_status_name(push.status))),
           text(push.error_message), OptionalText(std::in_place, next_action(push.status))}};
}

ProjectGitSyncOperationResponse sync_response(const holder::sync::ProjectSyncResult& result,
                                              const std::string& branch) {
  std::optional<std::string> error_code;
  std::optional<std::string> error_message;
  if (result.pull.attempted && result.pull.status != holder::sync::PullPhaseStatus::Succeeded) {
    error_code = result.pull.status == holder::sync::PullPhaseStatus::RemoteUnset ? "remote_unset"
                                                                                  : "pull_failed";
    error_message = result.pull.error_message;
  } else if (result.push.attempted && !pushed(result.push.status)) {
    error_code = holder::git::push_status_name(result.push.status);
    error_message = result.push.error_message;
  }
  ProjectGitSyncOperationResponseDataPush push{
      result.push.attempted, std::nullopt, result.push.ahead_count, result.push.behind_count,
      result.push.local_head_commit, result.push.error_message};
  if (result.push.attempted) push.status = sync_push_status(result.push.status);
  return {true,
          {result.project_id, OptionalText(std::in_place, result.remote_url), branch_name(branch),
           result.succeeded() ? "succeeded" : "failed", std::move(error_code),
           std::move(error_message),
           {result.pull.attempted, holder::sync::pull_phase_status_name(result.pull.status),
            result.pull.conflicts_resolved, result.pull.error_message},
           std::move(push)}};
}

ProjectGitSyncOperationResponse run_sync(holder::platform::Db& db,
                                         holder::git::GitOps* git_ops,
                                         const std::string& project_id,
                                         const std::optional<ProjectGitSyncRequest>& request,
                                         bool push) {
  const auto branch = request ? request->branch.value_or(std::nullopt).value_or("") : "";
  const bool set_upstream =
      request ? request->set_upstream.value_or(std::nullopt).value_or(true) : true;
  holder::project::ProjectRepo repo(db);
  find_project(repo, project_id);
  holder::index::FtsIndexer fts(db);
  const auto result = holder::sync::run_project_sync(
      db, &fts, resolve_git(git_ops), project_id,
      {.pull = true,
       .push = push,
       .push_after_failed_pull = false,
       .branch = branch,
       .set_upstream = set_upstream,
       .now = support::now_epoch_seconds()});
  return sync_response(result, branch);
}

} // namespace

ProjectGitTestRemoteResponse post_projects_project_id_git_test_remote(
    holder::platform::Db& db,
    holder::git::GitOps* git_ops,
    std::string project_id,
    std::optional<ProjectGitTestRemoteRequest> request) {
  const auto branch = request ? request->branch.value_or(std::nullopt).value_or("") : "";
  holder::project::ProjectRepo repo(db);
  auto& git = resolve_git(git_ops);
  auto operation = git.lock_operation(find_project(repo, project_id).root_path);
  // Reread under the lock, so the configured URL is the current one.
  auto remote_url = find_project(repo, project_id).git_remote_url;
  if (request && request->remote_url) remote_url = *request->remote_url;

  holder::git::RemoteProbeResult probe{holder::git::RemoteProbeStatus::RemoteUnset, false,
                                       "Remote URL is not configured."};
  if (remote_url && !remote_url->empty()) probe = git.probe_remote_url(*remote_url);
  const bool reachable = probe.status == holder::git::RemoteProbeStatus::Reachable;
  return {true,
          {std::move(project_id), OptionalText(std::in_place, std::move(remote_url)),
           branch_name(branch), holder::git::remote_probe_status_name(probe.status),
           probe.remote_has_head,
           text(reachable ? std::nullopt
                          : std::optional<std::string>(
                                holder::git::remote_probe_status_name(probe.status))),
           text(probe.error_message)}};
}

ProjectGitPushResponse post_projects_project_id_git_push(
    holder::platform::Db& db,
    holder::git::GitOps* git_ops,
    std::string project_id,
    std::optional<ProjectGitPushRequest> request) {
  const auto branch = request ? request->branch.value_or(std::nullopt).value_or("") : "";
  const bool set_upstream =
      request ? request->set_upstream.value_or(std::nullopt).value_or(true) : true;
  holder::project::ProjectRepo repo(db);
  holder::project::ProjectSyncRepo sync_repo(db);
  auto& git = resolve_git(git_ops);
  auto operation = git.lock_operation(find_project(repo, project_id).root_path);
  const auto project = find_project(repo, project_id);

  holder::git::PushResult push;
  if (!project.git_remote_url || project.git_remote_url->empty()) {
    push.status = holder::git::PushStatus::RemoteUnset;
    push.error_message = "Remote URL is not configured.";
  } else {
    git.open_or_init(project.root_path);
    git.set_remote("origin", *project.git_remote_url);
    try {
      if (project.privacy_mode == "encrypted_git")
        holder::privacy::assert_encryption_push_safe(project.root_path);
    } catch (const holder::privacy::PrivacyError& error) {
      support::throw_privacy_failure(error);
    }
    push = git.push_branch("origin", branch, set_upstream);
  }
  sync_repo.record_push_result(
      project_id, holder::git::push_status_name(push.status), pushed(push.status),
      push.error_message.empty() ? std::nullopt : std::optional<std::string>(push.error_message),
      support::now_epoch_seconds());
  if (push.status != holder::git::PushStatus::RemoteUnset) {
    // Best effort: a metrics refresh failure does not fail the push.
    try {
      const auto metrics = holder::git::inspect_repo_sync_metrics(project.root_path, "origin");
      sync_repo.update_activity_counts(
          project_id,
          {.uncommitted_changes_count = metrics.uncommitted_changes_count,
           .unpushed_commits_count = metrics.unpushed_commits_count,
           .updated_at = support::now_epoch_seconds()});
    } catch (const std::exception&) { // LCOV_EXCL_LINE
    }
  }
  return push_response(project_id, project.git_remote_url, branch, push);
}

ProjectGitSyncOperationResponse post_projects_project_id_git_pull(
    holder::platform::Db& db,
    holder::git::GitOps* git_ops,
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request) {
  return run_sync(db, git_ops, project_id, request, false);
}

ProjectGitSyncOperationResponse post_projects_project_id_git_sync(
    holder::platform::Db& db,
    holder::git::GitOps* git_ops,
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request) {
  return run_sync(db, git_ops, project_id, request, true);
}

} // namespace holder::api
