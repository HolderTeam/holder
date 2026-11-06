#pragma once

#include "api/HttpError.h"
#include "api/AiRuntimeTypes.h"

#include <exception>
#include <optional>
#include <string>

namespace holder::git { class GitOps; }
namespace holder::platform { class Db; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

struct GoogleDriveAuthorizeData {
  std::string authorization_url;
};

struct GoogleDriveAuthorizeResponse {
  bool ok;
  GoogleDriveAuthorizeData data;
};

GoogleDriveAuthorizeResponse post_locations_location_id_oauth_google_drive_authorize(
    holder::platform::Db& db,
    const std::optional<std::string>& host,
    std::string location_id);

api_runtime::host_value get_locations_location_id_oauth_google_drive_callback(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::optional<std::string>& host,
    std::string location_id,
    std::optional<std::string> code,
    std::optional<std::string> state,
    std::optional<std::string> error);

} // namespace holder::api
