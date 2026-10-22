#include "api/routes/ai/AiDispatch.h"

#include "api/routes/ai/AiRunRoutes.h"
#include "api/routes/ai/AiRunnerRoutes.h"

#include <boost/beast/http.hpp>

#include <string>

namespace holder::api::routes::ai {
namespace {

namespace http = boost::beast::http;

std::string segment_at(const std::string& path, std::size_t index) {
  if (path.empty() || path[0] != '/') return {};
  std::size_t cursor = 1;
  std::size_t current = 1;
  while (cursor <= path.size()) {
    const auto end = path.find('/', cursor);
    const auto seg_end = (end == std::string::npos) ? path.size() : end;
    if (current == index) {
      if (cursor >= seg_end) return {};
      return path.substr(cursor, seg_end - cursor);
    }
    if (end == std::string::npos) break;
    cursor = end + 1;
    ++current;
  }
  return {};
}

} // namespace

DispatchResult dispatch_ai_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    boost::asio::ip::tcp::socket& socket,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::llm::RunnerRegistry* runner_registry,
    const std::function<std::string()>& uuid_v4,
    const std::function<std::string(const std::string&)>& param,
    std::shared_ptr<holder::api::support::SseRegistry> streams
) {
  const std::string ai_resource = segment_at(path, 2);

  if (ai_resource == "runs") {
    if (const auto route_result = handle_ai_run_routes(
            path,
            req,
            res,
            socket,
            db,
            fts,
            secret_store,
            runner_registry,
            uuid_v4,
            param,
            streams
        );
        route_result.handled) {
      return {.handled = true, .streamed = route_result.streamed};
    }
    return {.handled = false, .streamed = false};
  }
  if (ai_resource == "runner" || ai_resource == "runners") {
    if (const auto route_result = handle_ai_runner_routes(
            path,
            req,
            res,
            socket,
            runner_registry,
            param,
            streams
        );
        route_result.handled) {
      return {.handled = true, .streamed = route_result.streamed};
    }
    return {.handled = false, .streamed = false};
  }
  return {.handled = false, .streamed = false};
}

} // namespace holder::api::routes::ai
