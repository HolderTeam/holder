#include "api/Router.h"
#include "api/generated/ByeRoutes.h"

#include <utility>

namespace holder::api {

std::size_t Router::KeyHash::operator()(const Key& key) const {
  const auto h1 = std::hash<int>()(static_cast<int>(key.method));
  const auto h2 = std::hash<std::string>()(key.target);
  return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

void Router::add(boost::beast::http::verb method, std::string target, Handler handler) {
  Key key{method, std::move(target)};
  routes_[std::move(key)] = std::move(handler);
}

bool Router::dispatch(const Request& req, Response& res) const {
  const auto target = req.target();
  const std::string target_str(target.data(), target.size());
  Key key{req.method(), target_str};
  const auto it = routes_.find(key);
  if (it != routes_.end()) {
    it->second(req, res);
    return true;
  }
  // Checked-in RIG output handles the operations migrated from handwritten routes.
  const auto generated = rig_http::dispatch(std::string(req.method_string()), target_str, req.body());
  if (!generated) return false;
  Response response{static_cast<boost::beast::http::status>(generated->status), req.version()};
  response.set(boost::beast::http::field::content_type, "application/json");
  response.keep_alive(false);
  response.body() = generated->body.dump();
  response.prepare_payload();
  res = std::move(response);
  return true;
}

} // namespace holder::api
