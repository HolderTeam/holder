#pragma once

#include "git/GitOps.h"
#include "platform/Db.h"
#include "privacy/SecretStore.h"

#include <boost/beast/http.hpp>

#include <optional>
#include <string>

namespace holder::api::routes {

// Starts connecting a Google Drive location for POST /locations/{id}/oauth/google-drive/
// authorize: generates a fresh PKCE challenge and state, stashes them keyed by location_id,
// and returns the Google authorization URL for the caller to open in the system browser.
// Google redirects back to host, the address the caller reached this daemon on.
std::string start_google_drive_authorization(
    holder::platform::Db& db,
    const std::string& location_id,
    const std::optional<std::string>& host,
    std::optional<long long> now = std::nullopt
);

// Finishes connecting a Google Drive location for GET /locations/{id}/oauth/google-drive/
// callback, where Google redirects the user's browser: checks state against what the
// authorize call stored, exchanges the code for tokens, finds or creates the
// "Holder/Resources" Drive folder, writes its ID into the location and binds the refresh
// token. Returns the HTML page the browser shows, since the browser, not the app, is
// showing it.
boost::beast::http::response<boost::beast::http::string_body> finish_google_drive_authorization(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::optional<std::string>& host,
    const std::string& location_id,
    const std::string& code,
    const std::string& state,
    const std::string& oauth_error,
    std::optional<long long> now = std::nullopt
);

} // namespace holder::api::routes
