#pragma once

#include "api/ProjectRead.h"
#include "privacy/PrivacyError.h"

#include <exception>
#include <functional>
#include <optional>
#include <string>

namespace holder::git { class GitOps; }

namespace holder::api {

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

struct RIG_OPEN_RECORD ProjectCreateResponse {
  bool ok RIG_BOOL_VALUE(true);
  Project data;
};

struct ProjectCreateUnavailable : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct ProjectCreateInternal : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};
struct ProjectCreateBadRequest : holder::privacy::PrivacyError {
  using PrivacyError::PrivacyError;
};

RIG_ROUTE(POST, "/projects")
RIG_SUMMARY("Create a project")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable", ProjectCreateUnavailable)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Project encryption setup failed", ProjectCreateInternal)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", ProjectCreateBadRequest, std::exception)
ProjectCreateResponse create_project(holder::platform::Db& db RIG_CONTEXT,
                                     holder::git::GitOps* git_ops RIG_CONTEXT,
                                     const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                     ProjectCreateRequest request);

} // namespace holder::api
