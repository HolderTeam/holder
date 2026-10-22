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
  std::string pin RIG_FIELD_DESCRIPTION("User-provided PIN used to wrap the project key.") RIG_MIN_LENGTH(1);
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

struct RIG_OPEN_RECORD RIG_INLINE_RECORD RecoveryTokenImportAutoResponseData {
  std::string project_id;
  bool project_created RIG_FIELD_DESCRIPTION("True when no local project had the token's project_id, so one was created from the token's metadata.");
  bool remote_hint_present;
  bool remote_configured;
  std::optional<std::optional<std::string>> remote_error RIG_OMIT_EMPTY RIG_NULLABLE;
  std::string pull_status RIG_STRING_VALUES("not_attempted", "succeeded", "failed");
  std::optional<std::optional<std::string>> pull_error RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD RecoveryTokenImportAutoResponse {
  bool ok RIG_BOOL_VALUE(true);
  RecoveryTokenImportAutoResponseData data;
};

struct RecoveryTokenProjectNotFound : std::runtime_error {
  RecoveryTokenProjectNotFound() : std::runtime_error("Project not found.") {}
};

RIG_ROUTE(POST, "/recovery-token/import")
RIG_SUMMARY("Import recovery token and auto-match/create project")
RIG_DESCRIPTION("Matches the token's project_id to a local project, or creates one from the token's metadata; project_created says which. When the token names a remote, it is configured and pulled; failures there are reported in the result, not as HTTP errors.")
RIG_RESPONSE_DESCRIPTION("Imported into a matched or newly created local project")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or an invalid token or PIN (recovery_token_invalid)", support::PrivacyRejected)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error, including missing key material or crypt metadata", support::PrivacyBroken)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable", support::KeyringUnavailable)
RecoveryTokenImportAutoResponse post_recovery_token_import(
    holder::platform::Db& db RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    ProjectRecoveryTokenImportRequest request);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/export")
RIG_SUMMARY("Export project recovery token")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, a project with no key material, or a privacy error", std::invalid_argument, support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", RecoveryTokenProjectNotFound)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error, including missing key material or crypt metadata", support::PrivacyBroken)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable", support::KeyringUnavailable)
ProjectRecoveryTokenExportResponse post_projects_project_id_recovery_token_export(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    ProjectRecoveryTokenExportRequest request);

RIG_ROUTE(POST, "/projects/{project_id}/recovery-token/import")
RIG_SUMMARY("Import project recovery token")
RIG_DESCRIPTION("The token must be for this project.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Malformed request, or an invalid token or PIN, including one for another project (recovery_token_invalid)", support::PrivacyRejected)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", RecoveryTokenProjectNotFound)
RIG_ERROR_RESPONSE(500, ErrorResponse, "Internal server error, including missing key material or crypt metadata", support::PrivacyBroken)
RIG_ERROR_RESPONSE(503, ErrorResponse, "Keyring unavailable", support::KeyringUnavailable)
ProjectRecoveryTokenImportResponse post_projects_project_id_recovery_token_import(
    holder::platform::Db& db RIG_CONTEXT,
    std::string project_id,
    ProjectRecoveryTokenImportRequest request);

} // namespace holder::api
