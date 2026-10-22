#include "api/routes/ai/AiRunnerRoutes.h"

#include "api/routes/ai/runner/AiRunnerPullEventRoutes.h"

namespace holder::api::routes {

// The runner routes are generated, except the pull event stream.
RunnerRouteDispatchResult handle_ai_runner_routes(
    const std::string& path,
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    boost::beast::http::response<boost::beast::http::string_body>& res,
    boost::asio::ip::tcp::socket& socket,
    holder::llm::RunnerRegistry* runner_registry,
    const std::function<std::string(const std::string&)>& param_get,
    std::shared_ptr<holder::api::support::SseRegistry> streams
) {
  return ai::runner::handle_ai_runner_pull_event_routes(
      path, req, res, socket, runner_registry, param_get, streams);
}

} // namespace holder::api::routes
