#include "api/routes/AuthenticatedRoutes.h"

#include "api/routes/AiResourceRoutes.h"
#include "api/support/HttpQuery.h"
#include "api/support/HttpResponses.h"

#include <boost/beast/http.hpp>

#include <string>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

} // namespace

AuthenticatedDispatchResult dispatch_authenticated_routes(
    const std::string& path,
    const std::string& query_string,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    boost::asio::ip::tcp::socket& socket,
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    const std::function<std::string()>& uuid_v4
) {
  auto param = [&](const std::string& key) -> std::string {
    return support::query_param_value(query_string, key);
  };
  bool streamed = false;
  if (handle_ai_resource_routes(
          path,
          req,
          res,
          db,
          uuid_v4,
          param,
          secret_store,
          git_ops,
          &socket,
          &streamed
      ))
    return {.streamed = streamed};

  res = support::error_response(http::status::not_found, "not_found", "Route not found.");
  return {};
}

} // namespace holder::api::routes
