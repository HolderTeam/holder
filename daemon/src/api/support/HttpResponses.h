#pragma once

#include "api/HttpError.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace holder::api::support {

inline boost::beast::http::response<boost::beast::http::string_body> json_response(
    boost::beast::http::status status,
    const nlohmann::json& payload
) {
  namespace http = boost::beast::http;
  http::response<http::string_body> res{status, 11};
  res.set(http::field::content_type, "application/json");
  res.keep_alive(false);
  res.body() = payload.dump();
  res.prepare_payload();
  return res;
} // LCOV_EXCL_LINE

inline boost::beast::http::response<boost::beast::http::string_body> error_response(
    boost::beast::http::status status,
    std::string code,
    std::string message
) {
  const holder::api::ErrorResponse payload{false, {std::move(code), std::move(message)}};
  return json_response(
      status,
      {{"ok", payload.ok}, {"error", {{"code", payload.error.code}, {"message", payload.error.message}}}}
  );
}

// The event-stream standard's answer to a reconnect once a stream has finished: stop reconnecting.
inline boost::beast::http::response<boost::beast::http::string_body> stream_finished_response() {
  namespace http = boost::beast::http;
  http::response<http::string_body> res{http::status::no_content, 11};
  res.keep_alive(false);
  res.prepare_payload();
  return res;
}

} // namespace holder::api::support
