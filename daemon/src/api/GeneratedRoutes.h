#pragma once

#include "api/support/StreamServer.h"

#include <boost/beast/http.hpp>

#include <chrono>
#include <functional>
#include <string>

namespace holder::platform { class Db; }
namespace holder::llm { class RunnerRegistry; }
namespace holder::index { class FtsIndexer; }
namespace holder::privacy { class SecretStore; }
namespace holder::git { class GitOps; }
namespace holder::card { class CardStore; }
namespace holder::api::support { class EventService; }

namespace holder::api {

using GeneratedRequest = boost::beast::http::request<boost::beast::http::string_body>;
using GeneratedResponse = boost::beast::http::response<boost::beast::http::string_body>;

// Whether the generated operation for this request is declared RIG_PUBLIC.
bool is_public_generated_route(const GeneratedRequest& req);

// Whether this request names an operation declared RIG_HOST_RESPONSE, whose response the
// daemon's own handlers write.
bool is_host_served_route(const GeneratedRequest& req);

// Returns false only when no generated operation matches the request. A route the daemon
// writes itself, such as an event stream, leaves res alone and its server in *stream.
bool dispatch_generated_routes(const GeneratedRequest& req, GeneratedResponse& res,
                               holder::platform::Db& db,
                               holder::llm::RunnerRegistry* registry = nullptr,
                               holder::index::FtsIndexer* fts = nullptr,
                               holder::privacy::SecretStore* secret_store = nullptr,
                               holder::git::GitOps* git_ops = nullptr,
                               std::function<std::string()> uuid_v4 = {},
                               std::chrono::steady_clock::time_point started_at = {},
                               holder::card::CardStore* card_store = nullptr,
                               holder::api::support::EventService* events = nullptr,
                               support::StreamServer* stream = nullptr);

} // namespace holder::api
