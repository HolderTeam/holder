#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <optional>
#include <string>

namespace holder::api {

// Declared for the contract; routes::GoogleDriveOAuthRoutes writes this response.

RIG_ROUTE(GET, "/locations/{location_id}/oauth/google-drive/callback")
RIG_PUBLIC
RIG_SUMMARY("Finish connecting a Google Drive location")
RIG_DESCRIPTION("Google redirects the user's browser here after authorization, so the request carries no token; the state from the authorize call authenticates it instead. The daemon exchanges the code, creates the location's Drive folder if needed, and binds the location. A failure is answered with 400 and an HTML page that explains it.")
RIG_RESPONSE_DESCRIPTION("An HTML page telling the user the location is connected")
RIG_HOST_RESPONSE("text/html")
std::string get_locations_location_id_oauth_google_drive_callback(
    std::string location_id,
    std::optional<std::string> code RIG_QUERY,
    std::optional<std::string> state RIG_QUERY,
    std::optional<std::string> error RIG_QUERY RIG_PARAMETER_DESCRIPTION("Set by Google when the user declined or authorization failed."));

} // namespace holder::api
