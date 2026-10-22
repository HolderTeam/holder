#include "api/GoogleDriveOAuth.h"

#include "api/routes/GoogleDriveOAuthRoutes.h"
#include "api/support/StreamServer.h"

namespace holder::api {

GoogleDriveAuthorizeResponse post_locations_location_id_oauth_google_drive_authorize(
    holder::platform::Db& db,
    const std::optional<std::string>& host,
    std::string location_id) {
  return {true, {routes::start_google_drive_authorization(db, location_id, host)}};
}

rig::host_value get_locations_location_id_oauth_google_drive_callback(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::optional<std::string>& host,
    std::string location_id,
    std::optional<std::string> code,
    std::optional<std::string> state,
    std::optional<std::string> error
) {
  auto page = routes::finish_google_drive_authorization(
      db,
      secret_store,
      git_ops,
      host,
      location_id,
      code.value_or(""),
      state.value_or(""),
      error.value_or("")
  );
  // The page, success or failure, is the whole response.
  return {
      200,
      support::StreamServer([page = std::move(page)](
                                boost::asio::ip::tcp::socket&,
                                boost::beast::http::response<boost::beast::http::string_body>& res
                            ) {
        res = page;
        return false;
      })
  };
}

} // namespace holder::api
