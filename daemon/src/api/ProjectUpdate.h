#pragma once

#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "privacy/PrivacyError.h"

#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::git { class GitOps; }
namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ProjectUpdateResponseData {
  std::string project_id;
  std::optional<bool> git_remote_changed RIG_FIELD_DESCRIPTION("Present when git_remote_url is supplied. Reports whether the stored remote URL changed, including disconnecting it. Reapplying the same value returns false. The URL is persisted only after Git accepts the remote operation.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ProjectUpdateResponse {
  bool ok RIG_BOOL_VALUE(true);
  ProjectUpdateResponseData data;
};

enum class ProjectUpdateRequestPrivacyMode {
  encrypted_git,
  plain,
};

struct RIG_OPEN_RECORD ProjectUpdateRequest {
  std::optional<std::optional<std::string>> name RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> root_path RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_remote_url RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> git_provider RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<ProjectUpdateRequestPrivacyMode>> privacy_mode RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> project_key_id RIG_OMIT_EMPTY RIG_NULLABLE;
  long long updated_at;
};

struct ProjectUpdateNotFound : std::runtime_error {
  ProjectUpdateNotFound() : std::runtime_error("Project not found.") {}
};
struct ProjectUpdateUnavailable : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct ProjectUpdateInternal : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct ProjectUpdateBadRequest : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

RIG_ROUTE(PATCH, "/projects/{project_id}")
RIG_SUMMARY("Update a project")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ProjectUpdateNotFound)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable", ProjectUpdateUnavailable)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Project encryption setup failed", ProjectUpdateInternal)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", ProjectUpdateBadRequest, std::exception)
ProjectUpdateResponse update_project(holder::platform::Db& db RIG_CONTEXT,
                                     holder::git::GitOps* git_ops RIG_CONTEXT,
                                     const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                     std::string project_id,
                                     ProjectUpdateRequest request);

} // namespace holder::api
