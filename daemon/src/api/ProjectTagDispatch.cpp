#include "api/ProjectTagDispatch.h"

#include "api/ProjectTags.h"
#include "api/generated/ProjectTagsRoutes.h"
#include "api/support/HttpResponses.h"

#include <boost/beast/http.hpp>

#include <exception>
#include <string>
#include <utility>

namespace holder::api {
namespace {

namespace http = boost::beast::http;

struct Context {
  holder::platform::Db& db;
};

Router::Response error_response(http::status status, std::string code, std::string message) {
  return support::json_response(status, rig_json::encode(ErrorResponse{
      false, {std::move(code), std::move(message)}}));
}

} // namespace

bool dispatch_project_tags(const Router::Request& req, Router::Response& res,
                           holder::platform::Db& db) {
  const auto target = req.target();
  Context context{db};
  try {
    const auto generated = rig_http::dispatch(
        std::string(req.method_string()), std::string(target.data(), target.size()),
        req.body(), context);
    // Keep malformed project IDs on Holder's existing not-found path.
    if (!generated || generated->status == 400) return false;
    res = support::json_response(static_cast<http::status>(generated->status), generated->body);
  } catch (const ProjectNotFound& error) {
    res = error_response(http::status::not_found, "not_found", error.what());
  } catch (const std::exception& error) {
    res = error_response(http::status::internal_server_error, "error", error.what());
  }
  return true;
}

} // namespace holder::api
