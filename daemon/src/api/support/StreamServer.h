#pragma once

#include "api/support/SseStream.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <functional>
#include <memory>

namespace holder::api::support {

// What a generated route hands the session for a response the daemon writes itself: it
// serves the body on the socket and returns true, or leaves the response to send instead in
// res and returns false. Generated dispatch has already decoded and checked the request.
using StreamServer = std::function<bool(
    boost::asio::ip::tcp::socket& socket,
    boost::beast::http::response<boost::beast::http::string_body>& res
)>;

// The request's connection, for a route that streams as it runs. Declaration headers
// name it without including Boost in the API declarations.
struct RequestSocket {
  boost::asio::ip::tcp::socket& socket;
};

// Serves an event stream polled from source, or answers 503 when too many are open.
StreamServer serve_events(std::shared_ptr<SseRegistry> streams, SseStream::Source source);

} // namespace holder::api::support
