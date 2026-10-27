#pragma once

#include "api/ApiService.h"
#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::platform { class Db; }

namespace holder::api {

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
