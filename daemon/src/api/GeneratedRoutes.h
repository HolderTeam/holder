#pragma once

#include "api/Router.h"

#include <chrono>
#include <functional>

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }
namespace holder::index { class FtsIndexer; }
namespace holder::privacy { class SecretStore; }
namespace holder::git { class GitOps; }
namespace holder::card { class CardStore; }

namespace holder::api {

// Whether the generated operation for this request is declared RIG_PUBLIC.
bool is_public_generated_route(const Router::Request& req);

// Returns false only when no generated operation matches the request.
bool dispatch_generated_routes(const Router::Request& req, Router::Response& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry = nullptr,
                               holder::index::FtsIndexer* fts = nullptr,
                               holder::privacy::SecretStore* secret_store = nullptr,
                               holder::git::GitOps* git_ops = nullptr,
                               std::function<std::string()> uuid_v4 = {},
                               std::chrono::steady_clock::time_point started_at = {},
                               holder::card::CardStore* card_store = nullptr);

} // namespace holder::api
