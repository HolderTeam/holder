#pragma once

#include "api/Router.h"

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }
namespace holder::index { class FtsIndexer; }
namespace holder::privacy { class SecretStore; }

namespace holder::api {

// Returns false only when no generated operation matches the request.
bool dispatch_generated_routes(const Router::Request& req, Router::Response& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry = nullptr,
                               holder::index::FtsIndexer* fts = nullptr,
                               holder::privacy::SecretStore* secret_store = nullptr);

} // namespace holder::api
