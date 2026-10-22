#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <exception>
#include <optional>
#include <string>

namespace holder::platform { class Db; }

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

} // namespace holder::api
