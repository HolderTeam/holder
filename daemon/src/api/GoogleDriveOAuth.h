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

RIG_ROUTE(POST, "/locations/{location_id}/oauth/google-drive/authorize")
RIG_SUMMARY("Start connecting a Google Drive location")
RIG_DESCRIPTION("Returns the Google authorization URL for the user to open in a browser. Google then redirects the browser to this location's callback, on the host and port this request used.")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "The location does not exist or is not a Google Drive location, or the daemon has no Google OAuth client", std::exception)
GoogleDriveAuthorizeResponse post_locations_location_id_oauth_google_drive_authorize(
    holder::platform::Db& db RIG_CONTEXT,
    const std::optional<std::string>& host RIG_CONTEXT,
    std::string location_id);

RIG_ROUTE(GET, "/locations/{location_id}/oauth/google-drive/callback")
RIG_PUBLIC
RIG_SUMMARY("Finish connecting a Google Drive location")
RIG_DESCRIPTION("Google redirects the user's browser here after authorization, so the request carries no token; the state from the authorize call authenticates it instead. The daemon exchanges the code, creates the location's Drive folder if needed, and binds the location. A failure is answered with 400 and an HTML page that explains it.")
RIG_RESPONSE_DESCRIPTION("An HTML page telling the user the location is connected")
RIG_HOST_RESPONSE("text/html")
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
