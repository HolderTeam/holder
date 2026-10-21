#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD ProjectListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<Project> data;
};

struct RIG_OPEN_RECORD ProjectGetResponse {
  bool ok RIG_BOOL_VALUE(true);
  Project data;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectGitSyncStatusResponseData {
  std::string project_id;
  ProjectSync sync;
};

struct RIG_OPEN_RECORD ProjectGitSyncStatusResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectGitSyncStatusResponseData data;
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

struct ProjectReadNotFound : std::runtime_error {
  ProjectReadNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(GET, "/projects")
RIG_SUMMARY("List projects")
RIG_RESPONSE_DESCRIPTION("OK")
ProjectListResponse get_projects(
    holder::platform::Db& db RIG_CONTEXT,
    std::optional<std::string> name RIG_QUERY RIG_PARAMETER_DESCRIPTION("Substring match on project name."),
    std::optional<long long> updated_after RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at >= updated_after."),
    std::optional<long long> updated_before RIG_QUERY RIG_PARAMETER_DESCRIPTION("Return projects with updated_at <= updated_before."),
    std::optional<std::string> order RIG_QUERY RIG_STRING_VALUES("updated_at_desc", "updated_at_asc", "created_at_desc", "created_at_asc", "name_desc", "name_asc") RIG_PARAMETER_DESCRIPTION("Order by updated_at (default), created_at, or name."),
    std::optional<int> limit RIG_QUERY RIG_MINIMUM(0) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Max results (default 100, capped at 1000)."),
    std::optional<int> offset RIG_QUERY RIG_MINIMUM(0) RIG_FORMAT("int32") RIG_PARAMETER_DESCRIPTION("Offset into ordered results (default 0)."),
    bool count RIG_QUERY RIG_DEFAULT(false) RIG_PARAMETER_DESCRIPTION("Include count fields (card_count/root_card_count) per project."));

RIG_ROUTE(GET, "/projects/{project_id}")
RIG_SUMMARY("Get a project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ProjectReadNotFound)
ProjectGetResponse get_project(holder::platform::Db& db RIG_CONTEXT,
                               std::string project_id);

RIG_ROUTE(GET, "/projects/{project_id}/git/sync-status")
RIG_SUMMARY("Get persisted project Git sync status")
RIG_RESPONSE_DESCRIPTION("Current persisted sync state")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", ProjectReadNotFound)
ProjectGitSyncStatusResponse get_project_git_sync_status(holder::platform::Db& db RIG_CONTEXT,
                                                         std::string project_id);

RIG_ROUTE(GET, "/projects/{project_id}/encryption-check")
RIG_SUMMARY("Check encrypted project card blobs before push/sync")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ProjectReadNotFound)
ProjectEncryptionCheckResponse check_project_encryption(holder::platform::Db& db RIG_CONTEXT,
                                                        std::string project_id);

} // namespace holder::api
