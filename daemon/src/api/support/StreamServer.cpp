#include "api/support/StreamServer.h"

#include "api/support/HttpResponses.h"

namespace holder::api::support {

StreamServer serve_events(std::shared_ptr<SseRegistry> streams, SseStream::Source source) {
  return [streams = std::move(streams), source = std::move(source)](
             boost::asio::ip::tcp::socket& socket,
             boost::beast::http::response<boost::beast::http::string_body>& res
         ) mutable {
    if (SseStream::start(socket, std::move(streams), std::move(source))) return true;
    res = error_response(
        boost::beast::http::status::service_unavailable,
        "server_busy",
        "Too many event streams."
    );
    return false;
  };
}

} // namespace holder::api::support
