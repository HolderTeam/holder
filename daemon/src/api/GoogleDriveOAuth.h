#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <exception>
#include <optional>
#include <string>

namespace holder::git { class GitOps; }
namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct RIG_OPEN_RECORD RIG_INLINE_RECORD GoogleDriveAuthorizeData {
  std::string authorization_url;
};

struct RIG_OPEN_RECORD GoogleDriveAuthorizeResponse {
  bool ok RIG_BOOL_VALUE(true);
  GoogleDriveAuthorizeData data;
};

GoogleDriveAuthorizeResponse post_locations_location_id_oauth_google_drive_authorize(
    holder::platform::Db& db RIG_CONTEXT,
    const std::optional<std::string>& host RIG_CONTEXT,
    std::string location_id);

rig::host_value get_locations_location_id_oauth_google_drive_callback(
    holder::platform::Db& db RIG_CONTEXT,
    holder::privacy::SecretStore* secret_store RIG_CONTEXT,
    holder::git::GitOps* git_ops RIG_CONTEXT,
    const std::optional<std::string>& host RIG_CONTEXT,
    std::string location_id,
    std::optional<std::string> code RIG_QUERY,
    std::optional<std::string> state RIG_QUERY,
    std::optional<std::string> error RIG_QUERY RIG_PARAMETER_DESCRIPTION("Set by Google when the user declined or authorization failed."));

} // namespace holder::api
