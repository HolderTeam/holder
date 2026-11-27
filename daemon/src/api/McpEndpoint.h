#pragma once

#include "api/GeneratedRoutes.h"

#include <string>

namespace holder::api::mcp {

struct Access {
  std::string token;
  std::string project_id;
  std::string server_version;
};

GeneratedResponse handle(
    const GeneratedRequest& request,
    boost::asio::ip::tcp::socket& socket,
    const Access& access,
    holder::platform::Db& db,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops
);

} // namespace holder::api::mcp
