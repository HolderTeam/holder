#pragma once

#include "api/Router.h"

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }

namespace holder::api {

// Returns false only when no generated operation matches the request.
bool dispatch_generated_routes(const Router::Request& req, Router::Response& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry = nullptr);

} // namespace holder::api
