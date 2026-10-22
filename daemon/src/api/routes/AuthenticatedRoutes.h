#pragma once

#include "git/GitOps.h"
#include "platform/Db.h"
#include "privacy/SecretStore.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <functional>
#include <string>

namespace holder::api::routes {

struct AuthenticatedDispatchResult {
  bool streamed = false;
};

// The authenticated routes the daemon writes itself; the session calls it only for routes
// the contract declares that way.
AuthenticatedDispatchResult dispatch_authenticated_routes(
    const std::string& path,
    const std::string& query_string,
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    boost::beast::http::response<boost::beast::http::string_body>& res,
    boost::asio::ip::tcp::socket& socket,
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::function<std::string()>& uuid_v4
);

} // namespace holder::api::routes
