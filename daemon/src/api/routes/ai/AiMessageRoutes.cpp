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
  return false;
}

} // namespace holder::api::routes
