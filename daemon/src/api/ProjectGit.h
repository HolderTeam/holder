#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"
#include "api/support/PrivacyErrors.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::git { class GitOps; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitTestRemoteResponseData {
  std::string project_id;
  std::optional<std::optional<std::string>> remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string branch;
  std::string status RIG_STRING_VALUES("reachable", "auth_failed", "not_found", "network_error", "invalid_remote_url", "remote_unset", "unknown_error");
  bool remote_has_head;
  std::optional<std::optional<std::string>> error_code RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_message RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD ProjectGitTestRemoteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitTestRemoteResponseData data;
};

struct RIG_OPEN_RECORD ProjectGitTestRemoteRequest {
  std::optional<std::optional<std::string>> remote_url RIG_FIELD_DESCRIPTION("Optional remote URL override for this read-only probe. Stored project configuration and Git configuration are unchanged. If omitted, uses the configured URL; null probes no remote.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> branch RIG_FIELD_DESCRIPTION("Optional branch hint, echoed in the result. If omitted, reports local_default. The probe lists remote refs without resolving or fetching a branch.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitPushResponseData {
  std::string project_id;
  std::optional<std::optional<std::string>> remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string branch;
  std::string status RIG_STRING_VALUES("pushed", "up_to_date", "auth_failed", "not_found", "network_error", "non_fast_forward", "remote_unset", "unknown_error");
  int ahead_count;
  int behind_count;
  std::optional<std::optional<std::string>> local_head_commit RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_code RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> error_message RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> next_action RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD ProjectGitPushResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitPushResponseData data;
};

struct RIG_OPEN_RECORD ProjectGitPushRequest {
  std::optional<std::optional<std::string>> branch RIG_FIELD_DESCRIPTION("Optional branch to push. If omitted, server resolves local default branch from HEAD/init.defaultBranch.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<bool>> set_upstream RIG_FIELD_DESCRIPTION("Optional; defaults to true.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncOperationResponseDataPull {
  bool attempted;
  std::string status RIG_STRING_VALUES("not_attempted", "succeeded", "remote_unset", "failed");
  int conflicts_resolved;
  std::optional<std::string> error_message RIG_NULLABLE;
};

enum class ProjectGitSyncOperationResponseDataPushStatus {
  pushed,
  up_to_date,
  auth_failed,
  not_found,
  network_error,
  non_fast_forward,
  remote_unset,
  unknown_error,
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncOperationResponseDataPush {
  bool attempted;
  std::optional<ProjectGitSyncOperationResponseDataPushStatus> status;
  int ahead_count;
  int behind_count;
  std::optional<std::string> local_head_commit RIG_NULLABLE;
  std::optional<std::string> error_message RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncOperationResponseData {
  std::string project_id;
  std::optional<std::optional<std::string>> remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string branch;
  std::string status RIG_STRING_VALUES("succeeded", "failed");
  std::optional<std::string> error_code RIG_FIELD_DESCRIPTION("Stable operation failure code. Git failures are returned in a successful HTTP envelope.") RIG_NULLABLE;
  std::optional<std::string> error_message RIG_NULLABLE;
  ProjectGitSyncOperationResponseDataPull pull;
  ProjectGitSyncOperationResponseDataPush push;
};

struct RIG_OPEN_RECORD ProjectGitSyncOperationResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitSyncOperationResponseData data;
};

struct RIG_OPEN_RECORD ProjectGitSyncRequest {
  std::optional<std::optional<std::string>> branch RIG_FIELD_DESCRIPTION("Optional branch to push for a full forced sync. Pull uses the checked-out branch.") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<bool>> set_upstream RIG_FIELD_DESCRIPTION("Optional push behavior; defaults to true.") RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct ProjectGitNotFound : std::runtime_error {
  ProjectGitNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(POST, "/projects/{project_id}/git/test-remote")
RIG_SUMMARY("Test project remote reachability/auth")
RIG_DESCRIPTION("Connects and lists remote refs using daemon credentials. Does not fetch, create a project repository, or change stored project configuration or Git configuration. Git failures are reported in the result, not as HTTP errors.")
RIG_RESPONSE_DESCRIPTION("Probe completed with structured status.")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectGitNotFound)
ProjectGitTestRemoteResponse post_projects_project_id_git_test_remote(
    holder::platform::Db& db RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    std::string project_id,
    std::optional<ProjectGitTestRemoteRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/push")
RIG_SUMMARY("Push project repository to remote")
RIG_DESCRIPTION("Git failures are reported in the result, not as HTTP errors. An encrypted project is checked before pushing, so no plaintext card leaves the device.")
RIG_RESPONSE_DESCRIPTION("Push attempted with structured status.")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or an encrypted project whose cards are not safe to push", support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectGitNotFound)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error, including missing key material or crypt metadata for an encrypted project", support::PrivacyBroken)
RIG_ERROR_RESPONSE(503, ErrorResponse, "The keyring needed to check an encrypted project is unavailable", support::KeyringUnavailable)
ProjectGitPushResponse post_projects_project_id_git_push(
    holder::platform::Db& db RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    std::string project_id,
    std::optional<ProjectGitPushRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/pull")
RIG_SUMMARY("Pull and reconcile a project immediately")
RIG_DESCRIPTION("Fetches the configured remote, fast-forwards or applies Holder's card-level conflict resolution, then reconciles the SQLite index before returning. Concurrent Git operations for the same project are serialized. Git failures are reported in the result, not as HTTP errors.")
RIG_RESPONSE_DESCRIPTION("Pull completed with structured status.")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectGitNotFound)
ProjectGitSyncOperationResponse post_projects_project_id_git_pull(
    holder::platform::Db& db RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request);

RIG_ROUTE(POST, "/projects/{project_id}/git/sync")
RIG_SUMMARY("Pull, reconcile, and push a project immediately")
RIG_DESCRIPTION("Pulls and reconciles first. Push is attempted only when pull succeeds. Concurrent Git operations for the same project are serialized. Git failures are reported in the result, not as HTTP errors.")
RIG_RESPONSE_DESCRIPTION("Forced sync completed with structured phase statuses.")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectGitNotFound)
ProjectGitSyncOperationResponse post_projects_project_id_git_sync(
    holder::platform::Db& db RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    std::string project_id,
    std::optional<ProjectGitSyncRequest> request);

} // namespace holder::api
