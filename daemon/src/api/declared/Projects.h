#pragma once

#include "api/ApiService.h"
#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::api {

// Declared for the contract; the existing route handlers still write these responses.

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectCreateResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectCreateResponseData data;
};

struct RIG_OPEN_RECORD ProjectCreateRequest {
  std::optional<std::string> project_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string name;
  std::optional<std::string> root_path RIG_FIELD_DESCRIPTION("Optional; server chooses a default if omitted.") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> git_remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::string> privacy_mode RIG_FIELD_DESCRIPTION("Optional; defaults to encrypted_git.") RIG_STRING_VALUES("encrypted_git", "plain") RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> project_key_id RIG_FIELD_DESCRIPTION("Optional keyring identifier (metadata only).") RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted or 0.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted or 0.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ProjectSync {
  std::optional<long long> last_commit_at RIG_NULLABLE;
  std::optional<long long> last_push_at RIG_NULLABLE;
  std::optional<long long> last_pull_at RIG_NULLABLE;
  int uncommitted_changes_count;
  int unpushed_commits_count;
  std::optional<std::string> last_push_status RIG_NULLABLE;
  std::optional<std::string> last_pull_status RIG_NULLABLE;
  std::optional<std::string> last_sync_error RIG_NULLABLE;
  std::optional<long long> last_sync_error_at RIG_NULLABLE;
  int retry_count;
  std::optional<long long> next_retry_at RIG_NULLABLE;
  int pull_retry_count;
  std::optional<long long> next_pull_retry_at RIG_NULLABLE;
  std::optional<long long> updated_at RIG_NULLABLE;
};

struct RIG_OPEN_RECORD Project {
  std::string project_id;
  std::string name;
  std::string root_path;
  std::optional<std::optional<std::string>> git_remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string privacy_mode RIG_STRING_VALUES("encrypted_git", "plain");
  std::optional<std::optional<std::string>> project_key_id RIG_OMIT_EMPTY RIG_NULLABLE;
  long long created_at;
  long long updated_at;
  std::optional<int> card_count RIG_FIELD_DESCRIPTION("Present when count=true.") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
  std::optional<int> root_card_count RIG_FIELD_DESCRIPTION("Present when count=true.") RIG_MINIMUM(0) RIG_OMIT_EMPTY;
  ProjectSync sync;
};

struct RIG_OPEN_RECORD ProjectGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  Project data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectDeleteResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectDeleteResponseData data;
};

struct RIG_OPEN_RECORD CardBreadcrumb {
  std::string kind RIG_STRING_VALUES("project", "card");
  std::string id;
  std::string title;
};

struct RIG_OPEN_RECORD CardBreadcrumbsResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<CardBreadcrumb> data RIG_FIELD_DESCRIPTION("Project first, then live ancestor cards from root to the requested card.");
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncStatusResponseData {
  std::string project_id;
  ProjectSync sync;
};

struct RIG_OPEN_RECORD ProjectGitSyncStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitSyncStatusResponseData data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectRecoveryTokenExportResponseData {
  std::string project_id;
  std::string key_id;
  std::string recovery_token RIG_FIELD_DESCRIPTION("Serialized `.hrk` recovery token JSON payload.");
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenExportResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectRecoveryTokenExportResponseData data;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenExportRequest {
  std::string pin RIG_FIELD_DESCRIPTION("User-provided PIN used to wrap the project key.");
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectRecoveryTokenImportResponseData {
  std::string project_id;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenImportResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectRecoveryTokenImportResponseData data;
};

struct RIG_OPEN_RECORD ProjectRecoveryTokenImportRequest {
  std::string pin;
  std::string recovery_token RIG_FIELD_DESCRIPTION("Serialized `.hrk` recovery token JSON payload.");
};

struct RIG_OPEN_RECORD ProjectEncryptionCheck {
  bool ok;
  int checked_files;
  int unsafe_files;
  std::vector<std::string> unsafe_paths;
  std::string message;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectEncryptionCheckResponseData {
  std::string project_id;
  std::string privacy_mode RIG_STRING_VALUES("encrypted_git", "plain");
  ProjectEncryptionCheck check;
};

struct RIG_OPEN_RECORD ProjectEncryptionCheckResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectEncryptionCheckResponseData data;
};

RIG_ROUTE(POST, "/projects")
RIG_SUMMARY("Create a project")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_HOST_RESPONSE("application/json")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
ProjectCreateResponse post_projects(ProjectCreateRequest request);

RIG_ROUTE(GET, "/projects/{project_id}")
RIG_SUMMARY("Get a project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectGetResponse get_projects_project_id(std::string project_id);

RIG_ROUTE(DELETE, "/projects/{project_id}")
RIG_SUMMARY("Delete a project")
RIG_DESCRIPTION("Removes the project from this device's database and records its removal so startup discovery and database reconstruction do not automatically import it again. Project files and encryption keys are retained for explicit recovery. Creating or importing the project explicitly registers it again.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Could not persist the project removal; the database deletion is rolled back.")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectDeleteResponse delete_projects_project_id(std::string project_id);

RIG_ROUTE(GET, "/projects/{project_id}/cards/{card_id}/breadcrumbs")
RIG_SUMMARY("Read a live card's project-to-card breadcrumb trail")
RIG_DESCRIPTION("Traverses only the requested card's live ancestors. A missing or trashed card returns 404; a broken or cyclic ancestor chain returns 409 rather than a partial trail.")
RIG_RESPONSE_DESCRIPTION("Project and live card path")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project or card not found")
RIG_ERROR_RESPONSE(405, ErrorResponse, "Method not allowed")
RIG_ERROR_RESPONSE(409, ErrorResponse, "Card's ancestor chain is broken or cyclic")
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error")
CardBreadcrumbsResponse get_projects_project_id_cards_card_id_breadcrumbs(
    std::string project_id,
    std::string card_id);

RIG_ROUTE(GET, "/projects/{project_id}/git/sync-status")
RIG_SUMMARY("Get persisted project Git sync status")
RIG_RESPONSE_DESCRIPTION("Current persisted sync state")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Sync status is unavailable")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found")
ProjectGitSyncStatusResponse get_projects_project_id_git_sync_status(std::string project_id);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/export")
RIG_SUMMARY("Export project recovery token")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectRecoveryTokenExportResponse post_projects_project_id_recovery_token_export(
    std::string project_id,
    ProjectRecoveryTokenExportRequest request);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/import")
RIG_SUMMARY("Import project recovery token")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectRecoveryTokenImportResponse post_projects_project_id_recovery_token_import(
    std::string project_id,
    ProjectRecoveryTokenImportRequest request);

RIG_ROUTE(GET, "/projects/{project_id}/encryption-check")
RIG_SUMMARY("Check encrypted project card blobs before push/sync")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_HOST_RESPONSE("application/json")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found")
ProjectEncryptionCheckResponse get_projects_project_id_encryption_check(std::string project_id);

} // namespace holder::api
