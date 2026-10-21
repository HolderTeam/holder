#include "api/routes/ai/AiStatusRoutes.h"

#include "api/routes/ai/status/AiLocalModelConfigRoutes.h"

namespace holder::api::routes {

bool handle_ai_status_routes(
    const std::string& path,
    const boost::beast::http::request<boost::beast::http::string_body>& req,
    boost::beast::http::response<boost::beast::http::string_body>& res,
    holder::platform::Db& db,
    holder::llm::RunnerRegistry* /*runner_registry*/,
    const std::function<std::string(const std::string&)>& /*param_get*/
) {
  if (ai::status::handle_ai_local_model_config_routes(path, req, res, db)) {
    return true;
  }
  return false;
}

} // namespace holder::api::routes
