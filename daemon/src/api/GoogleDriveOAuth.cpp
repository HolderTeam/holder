#include "api/GoogleDriveOAuth.h"

#include "api/routes/GoogleDriveOAuthRoutes.h"

namespace holder::api {

GoogleDriveAuthorizeResponse post_locations_location_id_oauth_google_drive_authorize(
    holder::platform::Db& db,
    const std::optional<std::string>& host,
    std::string location_id) {
  return {true, {routes::start_google_drive_authorization(db, location_id, host)}};
}

} // namespace holder::api
