#pragma once

#include "api/support/SseStream.h"

#include "index/FtsIndexer.h"
#include "llm/RunnerRegistry.h"
#include "platform/Db.h"
#include "privacy/SecretStore.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>

#include <functional>
#include <string>

namespace holder::api::routes {

struct RouteDispatchResult {
  bool handled = false;
  bool streamed = false;
};

} // namespace holder::api::routes

namespace holder::api::routes::ai::runs {

// Starts a run from a JSON request body and streams its events on the socket. Until the
// stream starts, a failure throws: std::invalid_argument for a bad request, or
// AiRunUnavailable. The generated POST /ai/runs route calls it with the decoded body.
RouteDispatchResult execute_ai_run(
    const std::string& request_body,
    boost::asio::ip::tcp::socket& socket,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::llm::RunnerRegistry* runner_registry,
    const std::function<std::string()>& uuid_v4,
    std::shared_ptr<holder::api::support::SseRegistry> streams = {}
);



} // namespace holder::api::routes::ai::runs
