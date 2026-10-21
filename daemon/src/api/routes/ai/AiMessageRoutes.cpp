#include "api/routes/ai/AiMessageRoutes.h"

#include "api/routes/ai/messages/AiMessageCaptureRoutes.h"
#include "api/support/HttpResponses.h"

namespace holder::api::routes {

namespace http = boost::beast::http;

bool handle_ai_message_routes(
    const std::string& path,
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    boost::beast::http::response<boost::beast::http::string_body>& res,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    const std::function<std::string()>& uuid_v4
) {
  if (ai::messages::handle_ai_message_capture_routes(path, req, res, db, fts, uuid_v4)) {
    return true;
  }
  // The other message routes are generated; these are their paths' unsupported methods.
  constexpr std::string_view prefix = "/ai/messages/";
  if (!path.starts_with(prefix)) return false;
  const auto rest = std::string_view(path).substr(prefix.size());
  const auto slash = rest.find('/');
  if (slash == 0 || slash == std::string_view::npos) return false;
  const auto tail = rest.substr(slash);
  if (tail != "/restore" && tail != "/links" && tail != "/backlinks") return false;
  res = support::error_response(
      http::status::method_not_allowed,
      "method_not_allowed",
      "Method not allowed."
  );
  return true;
}

} // namespace holder::api::routes
