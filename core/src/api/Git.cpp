// The contract's Git sync operations (contract/interface/git.py). The generated glue checks their
// arguments first; these do the work.
#include "api/Contract.h"

#include "api/CardAccess.h"
#include "api/CoreErrors.h"

#include "capi/Context.h"
#include "git/GitOps.h"
#include "git/PushResult.h"
#include "git/RemoteProbe.h"
#include "model/ProjectSyncState.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "project/ProjectSyncRepo.h"
#include "sync/ProjectSyncOperation.h"
#include "sync/ProjectSyncPolicy.h"

#include <optional>
#include <string>
#include <utility>

namespace holder::contract {
namespace {

holder::model::Project find(holder_context& context, const std::string& project_id) {
  auto project = holder::project::ProjectRepo(context.db).get(project_id);
  if (!project) throw NotFound("project not found: " + project_id);
  return std::move(*project);
}

// The host's Git, or one of core's own with the context's credentials.
holder::git::GitOps& git_for(holder_context& context, std::optional<holder::git::RealGitOps>& own) {
  if (context.git != nullptr) return *context.git;
  auto& git = own.emplace();
  if (context.credential_provider) git.set_credential_provider(context.credential_provider);
  return git;
}

std::optional<std::string> nonempty(const std::optional<std::string>& value) {
  return value && !value->empty() ? value : std::nullopt;
}

PullPhaseStatus pull_status(holder::sync::PullPhaseStatus status) {
  switch (status) {
  case holder::sync::PullPhaseStatus::NotAttempted: return PullPhaseStatus::NotAttempted;
  case holder::sync::PullPhaseStatus::Succeeded: return PullPhaseStatus::Succeeded;
  case holder::sync::PullPhaseStatus::RemoteUnset: return PullPhaseStatus::RemoteUnset;
  case holder::sync::PullPhaseStatus::Failed: break;
  }
  return PullPhaseStatus::Failed;
}

PushPhaseStatus push_status(holder::git::PushStatus status) {
  using Status = holder::git::PushStatus;
  switch (status) {
  case Status::Pushed: return PushPhaseStatus::Pushed;
  case Status::UpToDate: return PushPhaseStatus::UpToDate;
  case Status::AuthFailed: return PushPhaseStatus::AuthFailed;
  case Status::NotFound: return PushPhaseStatus::NotFound;
  case Status::NetworkError: return PushPhaseStatus::NetworkError;
  case Status::NonFastForward: return PushPhaseStatus::NonFastForward;
  case Status::RemoteUnset: return PushPhaseStatus::RemoteUnset;
  case Status::UnknownError: break;
  }
  return PushPhaseStatus::UnknownError;
}

RemoteTestResultStatus probe_status(holder::git::RemoteProbeStatus status) {
  using Status = holder::git::RemoteProbeStatus;
  switch (status) {
  case Status::Reachable: return RemoteTestResultStatus::Reachable;
  case Status::AuthFailed: return RemoteTestResultStatus::AuthFailed;
  case Status::NotFound: return RemoteTestResultStatus::NotFound;
  case Status::NetworkError: return RemoteTestResultStatus::NetworkError;
  case Status::InvalidRemoteUrl: return RemoteTestResultStatus::InvalidRemoteUrl;
  case Status::RemoteUnset: return RemoteTestResultStatus::RemoteUnset;
  case Status::UnknownError: break;
  }
  return RemoteTestResultStatus::UnknownError;
}

SyncState state(holder_context& context, const std::string& project_id) {
  find(context, project_id);
  const auto sync = holder::project::ProjectSyncRepo(context.db).get(project_id).value_or(holder::model::ProjectSyncState{});
  return {project_id,
          sync.last_commit_at,
          sync.last_push_at,
          sync.last_pull_at,
          sync.uncommitted_changes_count,
          sync.unpushed_commits_count,
          sync.last_push_status,
          sync.last_pull_status,
          sync.last_sync_error,
          sync.last_sync_error_at,
          sync.retry_count,
          sync.next_retry_at,
          sync.pull_retry_count,
          sync.next_pull_retry_at,
          sync.updated_at > 0 ? std::optional<long long>(sync.updated_at) : std::nullopt};
}

SyncResult convert_sync_result(const holder::sync::ProjectSyncResult& result) {
  return {result.project_id,
          result.succeeded() ? SyncResultStatus::Succeeded : SyncResultStatus::Failed,
          nonempty(result.remote_url),
          {result.pull.attempted, pull_status(result.pull.status), result.pull.conflicts_resolved,
           result.pull.hidden_cards_repaired, result.pull.error_message},
          {result.push.attempted,
           result.push.attempted ? push_status(result.push.status) : PushPhaseStatus::NotAttempted,
           result.push.ahead_count, result.push.behind_count, nonempty(result.push.local_head_commit),
           result.push.error_message}};
}

DueSyncResult convert_due_result(const holder::sync::ProjectSyncResult& result) {
  auto converted = convert_sync_result(result);
  return {std::move(converted.project_id), std::move(converted.remote_url),
          std::move(converted.pull), std::move(converted.push)};
}

// Pulls, pushes or both through core's one sync operation, which records the outcome.
SyncResult run(holder_context& context, const std::string& project_id, bool pull, bool push,
               const PushOptions& options) {
  const auto project = find(context, project_id);
  // An encrypted project with plaintext in it is refused before any Git work. The sync operation
  // checks again before it pushes, and would only record a failure.
  if (push && project.privacy_mode == "encrypted_git")
    holder::privacy::assert_encryption_push_safe(project.root_path);
  std::optional<holder::git::RealGitOps> own;
  const auto result = holder::sync::run_project_sync(
      context.db, &context.fts, git_for(context, own), project_id,
      {.pull = pull,
       .push = push,
       .push_after_failed_pull = false,
       .branch = options.branch.value_or(""),
       .set_upstream = options.set_upstream,
       .now = now_epoch_seconds()});
  return convert_sync_result(result);
}

DueSyncResult run_if_due(holder_context& context, const std::string& project_id,
                         std::int32_t push_interval_seconds, std::int32_t pull_interval_seconds) {
  const auto project = find(context, project_id);
  holder::sync::ProjectSyncResult skipped;
  skipped.project_id = project_id;
  skipped.remote_url = project.git_remote_url;
  if (!project.git_remote_url || project.git_remote_url->empty()) return convert_due_result(skipped);

  const auto now = now_epoch_seconds();
  const auto state = holder::project::ProjectSyncRepo(context.db).get(project_id);
  holder::sync::PullDecisionInput pull_input{
      .last_pull_at = state ? state->last_pull_at : std::optional<long long>{},
      .next_pull_retry_at = state ? state->next_pull_retry_at : std::optional<long long>{},
      .now = now,
  };
  if (pull_interval_seconds > 0) pull_input.pull_interval_seconds = pull_interval_seconds;
  const bool pull_due = holder::sync::should_attempt_pull(pull_input);
  holder::sync::PushDecisionInput push_input{
      .last_push_at = state ? state->last_push_at : std::optional<long long>{},
      .next_retry_at = state ? state->next_retry_at : std::optional<long long>{},
      .now = now,
  };
  if (push_interval_seconds > 0) push_input.push_interval_seconds = push_interval_seconds;
  const bool push_due = holder::sync::should_attempt_push(push_input);
  if (!pull_due && !push_due) return convert_due_result(skipped);

  std::optional<holder::git::RealGitOps> own;
  const auto result = holder::sync::run_project_sync(
      context.db, &context.fts, git_for(context, own), project_id,
      {.pull = pull_due,
       .push = push_due,
       .push_after_failed_pull = true,
       .branch = "",
       .set_upstream = true,
       .now = now});
  return convert_due_result(result);
}

RemoteTestResult test_remote(holder_context& context, const std::string& project_id, const RemoteTest& request) {
  std::optional<holder::git::RealGitOps> own;
  auto& git = git_for(context, own);
  auto operation = git.lock_operation(find(context, project_id).root_path);
  // Read again under the lock, so the remote is the one stored now.
  const auto remote_url = request.remote_url ? request.remote_url : nonempty(find(context, project_id).git_remote_url);
  holder::git::RemoteProbeResult probe{holder::git::RemoteProbeStatus::RemoteUnset, false,
                                       "The project has no Git remote."};
  if (remote_url) probe = git.probe_remote_url(*remote_url);
  return {project_id, probe_status(probe.status), remote_url, probe.remote_has_head,
          probe.status == holder::git::RemoteProbeStatus::Reachable ? std::nullopt
                                                                     : nonempty(probe.error_message)};
}

} // namespace

SyncState get_project_sync_state(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return state(context, project_id); });
}

SyncResult pull_project(holder_context& context, std::string project_id) {
  return with_contract_errors([&] { return run(context, project_id, true, false, PushOptions{}); });
}

SyncResult push_project(holder_context& context, std::string project_id, const PushOptions& options) {
  return with_contract_errors([&] { return run(context, project_id, false, true, options); });
}

SyncResult sync_project(holder_context& context, std::string project_id, const PushOptions& options) {
  return with_contract_errors([&] { return run(context, project_id, true, true, options); });
}

DueSyncResult sync_project_if_due(holder_context& context, std::string project_id,
                                  std::int32_t push_interval_seconds, std::int32_t pull_interval_seconds) {
  return with_contract_errors([&] {
    return run_if_due(context, project_id, push_interval_seconds, pull_interval_seconds);
  });
}

RemoteTestResult test_project_remote(holder_context& context, std::string project_id, const RemoteTest& remote) {
  return with_contract_errors([&] { return test_remote(context, project_id, remote); });
}

} // namespace holder::contract
